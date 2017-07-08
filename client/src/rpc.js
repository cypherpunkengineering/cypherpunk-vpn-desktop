'use strict';

const EventEmitter = require('events');

// An implementation of JSON-RPC 2.0 over websockets. It attempts to stay
// connected until the 'disconnect' method is explicitly called.

const ERROR_METHOD_NOT_FOUND = { code: -32601, message: "Method not found" };
const ERROR_INVALID_JSON = { code: -32700, message: "Invalid JSON" };
const ERROR_INVALID_REQUEST = { code: -32600, message: "Not a valid RPC request" };
const ERROR_NOT_CONNECTED = { code: -32603, message: "WebSocket is not connected" };

exports.ERROR_METHOD_NOT_FOUND = ERROR_METHOD_NOT_FOUND;

class WebSocketImpl extends EventEmitter {
  constructor(WebSocket, { url, onerror, onopen, oncall, onpost }) {
    super();
    var self = this;
    self.WebSocket = WebSocket;
    self._queue = [];
    self._nextId = 1;
    self._callbacks = {};
    self._handlers = {};

    self.on('error', function() {}); // dummy listener to silence Node.js error detection

    // Helper function to connect to the websocket
    function connect() {

      // Helper to process any queued up commands
      function _onopen(evt) {
        if (onopen) {
          onopen(evt);
        }
        self._queue.forEach(fn => fn());
        self._queue = [];
      }

      // Helper to handle incoming websocket messages
      function _onmessage(evt) {
        let id = null;
        // Helper function to send a reply
        function reply(type, value) {
          let msg = '{"jsonrpc":"2.0","' + type + '":' + JSON.stringify(value) + ',"id":' + JSON.stringify(id) + '}';
          if (self.socket && self.socket.readyState == self.WebSocket.OPEN) {
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
          } else if (obj instanceof Error) {
            reply('error', { code: 0, message: obj.toString(), data: obj.toString().split('\n') });
          } else {
            reply('error', { code: 0, message: "Unknown error", data: obj });
          }
        }
        let obj;
        if (typeof evt.data !== 'string') {
          return reply('error', ERROR_INVALID_JSON);
        }
        if (evt.data == "") {
          // Just an empty string; ignore
          return;
        }
        try {
          obj = JSON.parse(evt.data);
        } catch (e) {
          return reply('error', ERROR_INVALID_JSON);
        }
        if (typeof obj === 'object' && obj.jsonrpc === '2.0') {
          id = obj.id;
          let hasId = id === null || typeof id === 'string' || typeof id === 'number';
          if (!hasId) {
            id = null;
          }
          let method = obj.method;
          if (method !== undefined) {
            if (typeof method === 'string' && method != "" && obj.result === undefined && obj.error === undefined && Array.isArray(obj.params)) {
              let result;
              try {
                // Check registered methods (a method can be posted if the caller doesn't care about the result)
                if (self._handlers[method]) {
                  result = self._handlers[method].apply(self, obj.params);
                } else if (hasId) { // But if the caller _does_ care about the result, we need a call handler
                  if (oncall) {
                    result = oncall(method, obj.params, id);
                  } else {
                    return reply('error', ERROR_METHOD_NOT_FOUND);
                  }
                } else { // Otherwise notify all notification listeners
                  let count = self.emit(method, ...obj.params);
                  if (onpost) {
                    onpost(method, obj.params);
                    count++;
                  }
                  if (count == 0) {
                    return reply('error', ERROR_METHOD_NOT_FOUND);
                  }
                  return;
                }
                // Not a notification; check the result
                if (result instanceof Promise) {
                  return result.then(r => { reply('result', r); }, e => { replyError(e); });
                } else {
                  return reply('result', result);
                }
              } catch (ex) {
                console.warn(ex);
                return replyError(ex);
              }
            }
            // fallthrough to error below
          } else if (obj.hasOwnProperty('result') ^ obj.hasOwnProperty('error')) {
            let cb = self._callbacks[id];
            if (cb) {
              if (id !== null) {
                delete self._callbacks[id];
              }
              cb(obj['result'], obj['error']);
              return;
            } else if (id === null) {
              console.log("RPC error:", obj.error);
              return;
            }
            // fallthrough to error below
          }
        }
        return reply('error', ERROR_INVALID_REQUEST);
      }

      // If there is an onerror handler, report the errors to the client, otherwise just try to reconnect
      function _onerror(evt) {
        if (!onerror || onerror(evt)) {
          reconnect();
        } else {
          self.disconnect();
        }
      }

	    console.log("Connecting to " + url + "...");
      self.socket = new self.WebSocket(url);
      self.socket.onerror = _onerror;
      self.socket.onclose = _onerror;
      self.socket.onopen = _onopen;
      self.socket.onmessage = _onmessage;
    }

    // Helper to reconnect on any errors (instance-specific)
    var reconnectTimeout = null;
    function reconnect() {
      if (self.socket && self.socket.readyState >= self.WebSocket.CLOSING && reconnectTimeout === null) {
        reconnectTimeout = setTimeout(function() {
          reconnectTimeout = null;
          if (self.socket && self.socket.readyState >= self.WebSocket.CLOSING) {
            self.socket.onerror = function() {};
            self.socket.onclose = function() {};
            self.socket.close();
            self.socket = null;
            connect();
          }
        }, 500);
      }
    }

    connect();
  }

  // Disconnects from the RPC server, ignoring all future calls.
  // Once disconnected, an RPC instance cannot be reconnected.
  //
  disconnect() {
    return new Promise((resolve, reject) => {
      if (this.socket) {
        this.socket.onerror = resolve;
        this.socket.onclose = resolve;
        this.socket.close();
        if (this.socket.readyState == this.WebSocket.CLOSED) {
          resolve();
        }
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
  // callback(arg1, arg2, ...) will be invoked. Registered methods are the only way
  // to return values from a call.
  //
  registerMethod(method, callback) {
    this._handlers[method] = callback;
  }

  unregisterMethod(method, callback) {
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

      if (self.socket && self.socket.readyState == self.WebSocket.OPEN) {
        sendInternal();
      } else {
        self._queue.push(sendInternal);
      }
      return id;
    } else {
      if (callback) {
        callback(undefined, ERROR_NOT_CONNECTED);
      }
    }
  }
};
exports.WebSocketImpl = WebSocketImpl;

if (!process || process.type === 'renderer') {

  const { ipcRenderer } = require('electron');

  class IPCImpl extends EventEmitter
  {
    constructor({ oncall, onpost }) {
      super();
      this._nextId = 0;
      this._callbacks = {};
      this._handlers = {};

      this.on('error', function() {}); // dummy listener to silence Node.js error detection

      ipcRenderer.on('daemon-call', (event, method, params, id) => {
        try {
          var result;
          if (this._handlers[method]) {
            result = Promise.resolve(this._handlers[method].apply(this, params));
          } else if (oncall) {
            result = oncall(method, params, id);
          } else {
            ipcRenderer.send('daemon-result', id, null, ERROR_METHOD_NOT_FOUND);
            return;
          }
          result.then(result => {
            ipcRenderer.send('daemon-result', id, result, null);
          }, error => {
            ipcRenderer.send('daemon-result', id, null, error);
          });
        } catch (ex) {
          console.warn(ex);
          ipcRenderer.send('daemon-result', id, null, ex);
        }
      });
      ipcRenderer.on('daemon-post', (event, method, params) => {
        try {
          if (this._handlers[method]) {
            this._handlers[method].apply(this, params);
          } else if (onpost) {
            this.emit(method, ...params);
            onpost(method, params);
          }
        } catch (ex) { console.warn(ex); }
      });
      ipcRenderer.on('daemon-result', (event, id, result, error) => {
        try {
          let cb = this._callbacks[id];
          delete this._callbacks[id];
          cb(result, error);
        } catch (ex) { console.warn(ex); }
      });
    }
    send(method, params, callback) {
      var id;
      if (callback) {
        id = this._nextId++;
        this._callbacks[id] = callback;
        ipcRenderer.send('daemon-call', method, params, id);
      } else {
        ipcRenderer.send('daemon-post', method, params);
      }
    }
    registerMethod(method, callback) {
      this._handlers[method] = callback;
    }
    unregisterMethod(method) {
      delete this._handlers[method];
    }
  }
  exports.IPCImpl = IPCImpl;

}

class RPC {
  constructor(impl) {
    // Make a proxy object to call remote methods, either via:
    //   rpc.call(method, [args])
    // or:
    //   rpc.call.method(arg1, arg2, ...)
    // Returns a Promise which will be fulfilled with the result.
    //
    this.call = new Proxy(Object.freeze(function call(method, args) {
      return new Promise((resolve, reject) => {
        impl.send(method, args, (result, error) => {
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
            impl.send(name, args, (result, error) => {
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
    this.post = new Proxy(Object.freeze(function post(method, args) {
      impl.send(method, args);
    }), {
      get: function(target, name) {
        return function() {
          impl.send(name, [].slice.call(arguments));
        }
      }
    });

    this.registerMethod = impl.registerMethod.bind(impl);
    this.unregisterMethod = impl.unregisterMethod.bind(impl);
  }
}
exports.RPC = RPC;
