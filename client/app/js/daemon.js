var socket = null;
var shouldConnect = false;
var reconnectTimeout = null;
var nextId = null;
var sendQueue = [];
var messageCallbacks = {};

exports = module.exports = {
  onmethod: null,
  onresult: null,
  onerror: null,
};

var connect = exports.connect = function() {
  disconnect();
  shouldConnect = true;

  function reconnect() {
    if (shouldConnect && reconnectTimeout === null) {
      reconnectTimeout = setTimeout(() => {
        reconnectTimeout = null;
        if (shouldConnect) {
          connect();
        }
      }, 500);
    }
  }

  socket = new WebSocket("ws://127.0.0.1:9337/");
  socket.onerror = reconnect;
  socket.onclose = reconnect;
  socket.onopen = function() {
    sendQueue.forEach(fn => fn());
    sendQueue = [];
    nextId = 1;
  };
  socket.onmessage = function(evt) {
    if (evt.data instanceof DOMString) {
      var obj = JSON.parse(evt.data);
      var id = obj.id;
      if (obj && obj['jsonrpc'] == "2.0") {
        if (obj['method']) {
          if (typeof exports.onmessage === 'function') {
            exports.onmessage(obj.method, obj.params, function(result) {
              if (id !== undefined) {
                reply(id, result);
              }
            });
          }
        } else if (obj['result']) {

        } else if (obj['error']) {

        }
      }
    }
  }
};

var disconnect = exports.disconnect = function() {
  shouldConnect = false;
  if (reconnectTimeout !== null) {
    clearTimeout(reconnectTimeout);
    reconnectTimeout = null;
  }
  if (socket) {
    socket.close();
    socket = null;
  }
  nextId = null;
  sendQueue = [];
};

var reply = function(id, result) {
  var msg = '{"jsonrpc":"2.0","result":' + JSON '}'
}

var send = exports.send = function(method, params, callback) {
  var id = nextId++;
  var msg = '{"jsonrpc":"2.0","method":' + JSON.stringify(method) + ',"params":' + JSON.stringify(params) + ',"id":' + JSON.stringify(id) + '}';

  var sendInternal = function() {
    messageCallbacks[id] = callback;
    socket.send(msg);
  };

  if (socket && socket.readyState == WebSocket.OPEN) {
    sendInternal();
  } else {
    sendQueue.push(sendInternal);
  }
  return id;
};

if (shouldConnect) {
  connect();
}
