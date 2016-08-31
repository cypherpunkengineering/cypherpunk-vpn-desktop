'use strict';

// An implementation of JSON-RPC 2.0 over websockets. It attempts to stay
// connected until the 'disconnect' method is explicitly called.

class RPC {
  constructor({ url, onerror, onopen }) {
    var self = this;
    self._queue = [];
    self._nextId = 1;
    self._callbacks = {};
    self._handlers = {};
    self.url = url;
    self.onerror = onerror;

    // Helper function to connect to the websocket
    function connect() {

      // Helper to process any queued up commands
      function _onopen(evt) {
        self._queue.forEach(fn => fn());
        self._queue = [];
        if (onopen) {
          onopen(evt);
          onopen = null;
        }
      }

      // Helper to handle incoming websocket messages
      function onmessage(evt) {
        let id = null;
        // Helper function to send a reply
        function reply(type, value) {
          let msg = '{"jsonrpc":"2.0","' + type + '":' + JSON.stringify(value) + ',"id":' + JSON.stringify(id) + '}';
          if (self.socket && self.socket.readyState == RPC.WebSocket.OPEN) {
            self.socket.send(msg);
          } else {
            self._queue.push(() => socket.send(msg));
          }
        }
        // Helper function to send a user error
        function replyError(obj) {
          if (typeof obj === 'string') {
            reply('error', { code: 0, message: obj });
          } else if (Number.isInteger(obj)) {
            reply('error', { code: obj, message: "Unknown error" });
          } else if (typeof obj === 'object' && obj.hasOwnProperty('code') && obj.hasOwnProperty('message')) {
            reply('error', { code: obj.code, message: obj.message, data: obj.data });
          } else {
            reply('error', { code: 0, message: "Unknown error", data: obj });
          }
        }
        let obj;
        if (typeof evt.data !== 'string') {
          return reply('error', { code: -32700, message: "Message did not contain text data" });
        }
        try {
          obj = JSON.parse(evt.data);
        } catch (e) {
          return reply('error', { code: -32700, message: "Failed to parse JSON" });
        }
        if (typeof obj === 'object' && typeof obj['jsonrpc'] == 'string' && obj['jsonrpc'] == "2.0") {
          id = obj.id;
          let hasId = id === null || typeof id === 'string' || typeof id === 'number';
          if (!hasId) {
            id = null;
          }
          if (obj.hasOwnProperty('method')) {
            if (typeof obj['method'] === 'string' && obj['method'] != "" &&
                !obj.hasOwnProperty('result') && !obj.hasOwnProperty('error') &&
                (/*typeof obj['params'] === 'undefined' || typeof obj['params'] === 'object' ||*/ Array.isArray(obj['params']))) {
              try {
                let result;
                if (typeof self._handlers[obj['method']] === 'function') {
                  result = self._handlers[obj['method']].apply(null, obj['params']);
                } else if (typeof self.onrequest === 'function') {
                  result = self.onrequest.call(null, obj['method'], obj['params']);
                } else {
                  return reply('error', { code: -32601, message: "Method not found" });
                }
                if (hasId) {
                  if (result instanceof Promise) {
                    // Asynchronous return
                    result.then(r => { reply('result', r); }, e => { replyError(e); })
                  } else {
                    // Synchronous return
                    reply('result', result);
                  }
                }
              } catch (ex) {
                replyError(ex);
              }
              return;
            }
          } else if (obj.hasOwnProperty('result') ^ obj.hasOwnProperty('error')) {
            if (hasId) {
              let cb = self._callbacks[id];
              if (cb) {
                delete self._callbacks[id];
                cb(obj['result'], obj['error']);
              }
              return;
            }
          }
        }
        return reply('error', { code: -32600, message: "Not a valid RPC request" });
      }

      // If there is an onerror handler, report the errors to the client, otherwise just try to reconnect
      function onerror(evt) {
        if (typeof self.onerror !== 'function' || self.onerror(evt))
          reconnect();
        else
          self.disconnect();
      }

	    console.log("Connecting to " + url + "...");
      self.socket = new RPC.WebSocket(url);
      self.socket.onerror = onerror;
      self.socket.onclose = onerror;
      self.socket.onopen = _onopen;
      self.socket.onmessage = onmessage;
    }

    // Helper to reconnect on any errors (instance-specific)
    var reconnectTimeout = null;
    function reconnect() {
      if (self.socket && self.socket.readyState >= RPC.WebSocket.CLOSING && reconnectTimeout === null) {
        reconnectTimeout = setTimeout(function() {
          reconnectTimeout = null;
          if (self.socket && self.socket.readyState >= RPC.WebSocket.CLOSING) {
            connect();
          }
        }, 500);
      }
    }

    // Make a proxy object to call remote methods, either via:
    //   rpc.call(method, [args])
    // or:
    //   rpc.call.method(arg1, arg2, ...)
    // Returns a Promise which will be fulfilled with the result.
    //
    self.call = new Proxy(Object.freeze(function call(method, args) {
      return new Promise((resolve, reject) => {
        self.send(method, args, (result, error) => {
          if (error)
            reject(error);
          else
            resolve(result);
        });
      });
    }), {
      get: function(target, name) {
        return function() {
          let args = [].slice.call(arguments);
          return new Promise((resolve, reject) => {
            self.send(name, args, (result, error) => {
              if (error)
                reject(error);
              else
                resolve(result);
            });
          });
        };
      }
    });

    // Make a proxy object to post remote notifications, either via:
    //   rpc.post(method, [args])
    // or:
    //   rpc.post.method(arg1, arg2, ...)
    // The result of posted messages is ignored (no ID sent).
    //
    self.post = new Proxy(Object.freeze(function post(method, args) {
      self.send(method, args);
    }), {
      get: function(target, name) {
        return function() {
          self.send(name, [].slice.call(arguments));
        }
      }
    });

    connect();
  }

  // Disconnects from the RPC server, ignoring all future calls.
  // Once disconnected, an RPC instance cannot be reconnected.
  //
  disconnect() {
    return new Promise((resolve, reject) => {
      if (this.socket && this.socket.readyState != WebSocket.CLOSED) {
        this.socket.onclose = resolve;
        this.socket.close();
      } else {
        resolve();
      }
      this.socket = null;
      this._queue = [];
      this._callbacks = {};
      this._handlers = {};
    });
  }

  // Register an RPC function with the name 'method'; when called by the remote side,
  // callback(arg1, arg2, ..., argN) will be invoked.
  //
  on(method, callback) {
    this._handlers[method] = callback;
  }

  // Unregisters an RPC function.
  //
  removeListener(method) {
    delete this._handlers[method];
  }

  // Manually send a remote procedure call.
  // - method: The name of the method to invoked
  // - params: An array of method parameters
  // - callback: if passed, expect a response, callback(result, error)
  // If callback is specified, the result is the ID attached to the
  // RPC request, otherwise the result is undefined.
  //
  send(method, params, callback) {
    var self = this;
    if (self.socket) {
      var id;
      var msg = '{"jsonrpc":"2.0","method":' + JSON.stringify(method) + ',"params":' + JSON.stringify(params);
      if (callback) {
        id = self._nextId++;
        msg += ',"id":' + JSON.stringify(id);
      }
      msg += '}';
      var sendInternal = function() {
        if (callback) {
          self._callbacks[id] = callback;
        }
        self.socket.send(msg);
      };

      if (self.socket && self.socket.readyState == WebSocket.OPEN) {
        sendInternal();
      } else {
        self._queue.push(sendInternal);
      }
      return id;
    } else {
      if (callback) {
        callback(undefined, { code: -32603, message: "WebSocket is not connected" });
      }
    }
  }
};

RPC.WebSocket = global.WebSocket;

module.exports = RPC;
