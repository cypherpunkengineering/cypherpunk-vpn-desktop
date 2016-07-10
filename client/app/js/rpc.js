// An implementation of JSON-RPC 2.0 over websockets. It attempts to stay
// connected until the 'disconnect' method is explicitly called.

module.exports = class RPC {
  constructor(url) {
    var self = this;
    self._queue = [];
    self._nextId = 1;
    self._callbacks = {};

    // Helper function to connect to the websocket
    function connect() {

      // Helper to process any queued up commands
      function onopen() {
        self._queue.forEach(fn => fn());
        self._queue = [];
        self._nextId = 1;
      }

      // Helper to handle incoming websocket messages
      function onmessage(evt) {
        var id = null;
        // Helper function to send a reply
        function reply(type, value) {
          var msg = '{"jsonrpc":"2.0","' + type + '":' + JSON.stringify(value) + ',"id":' + JSON.stringify(id);
          if (self.socket && self.socket.readyState == WebSocket.OPEN) {
            socket.send(msg);
          } else {
            self._queue.push(() => socket.send(msg));
          }
        }
        var obj;
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
          var validId = id === null || typeof id === 'string' || typeof id === 'number'; 

          if (obj.hasOwnProperty('method')) {
            if (typeof obj['method'] === 'string' && obj['method'] != "" &&
                !obj.hasOwnProperty('result') && !obj.hasOwnProperty('error') &&
                (typeof obj['params'] === 'undefined' || typeof obj['params'] === 'object' || Array.isArray(obj['params']))) {
              if (typeof self.onrequest === 'function') {
                self.onrequest(obj['method'], obj['params'], {
                  send: function send(result) {
                    if (validId) { reply('result', result); }
                  },
                  error: function error(code, message, data) {
                    if (validId) { reply('error', { code: code, message: message, data: data }); }
                  },
                });
              }
              return;
            }
          } else if (obj.hasOwnProperty('result') || obj.hasOwnProperty('error')) {
            if (validId && !(obj.hasOwnProperty('result') && obj.hasOwnProperty('error'))) {
              let cb = self._callbacks[id]; 
              if (cb) {
                delete self._callbacks[id];
                cb(obj['result'], obj['error']);
              }
            }
            return;
          }
        }
        return reply('error', { code: -32600, message: "Not a valid RPC request" });
      }

	    console.log("Connecting to " + url + "...");
      self.socket = new WebSocket(url);
      self.socket.onerror = reconnect;
      self.socket.onclose = reconnect;
      self.socket.onopen = onopen;
      self.socket.onmessage = onmessage;
    }

    // Helper to reconnect on any errors (instance-specific)
    var reconnectTimeout = null;
    function reconnect() {
      if (self.socket && self.socket.readyState >= WebSocket.CLOSING && reconnectTimeout === null) {
        reconnectTimeout = setTimeout(function() {
          reconnectTimeout = null;
          if (self.socket && self.socket.readyState >= WebSocket.CLOSING) {
            connect();
          }
        }, 500);
      }
    }

    connect();
  }

  disconnect() {
    this.socket.onclose = null;
    this.socket.close();
    this.socket = null;
    this._queue = [];
    this._callbacks = {};
  }

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
    }
  }
};
