const { ipcMain } = require('electron');
const EventEmitter = require('events');
const { RPC, WebSocketImpl, ERROR_METHOD_NOT_FOUND } = require('./rpc.js');
const WebSocket = require('ws');
//const WebSocket = require('websocket').client;

var mainWindow = null;
var callbacks = {};
var handlers = {};
var ws, rpc, daemon;
var isopen = false;

function sendCurrentState() {
  if (mainWindow) {
    mainWindow.webContents.send(isopen ? 'daemon-up' : 'daemon-down');
  }
}

// Set up a Daemon class which is also an EventEmitter (for notifications).
// In addition to actual RPC calls, the special events 'up' and 'down'
// notify that the connection to the daemon is up or down, respectively.

class Daemon extends EventEmitter {
  constructor() {
    super();
    this.state = {};
  }
  registerMethod(method, callback) {
    handlers[method] = callback;
  }
  unregisterMethod(method) {
    delete handlers[method];
  }
  setMainWindow(w) {
    if (w !== mainWindow) {
      mainWindow = w;
      sendCurrentState();
    }
  }
  disconnect() {
    return ws.disconnect();
  }
}

// Listen for requests and replies from the renderer

ipcMain.on('daemon-call', (event, method, params, id) => {
  rpc.call(method, params).then(result => {
    event.sender.send('daemon-result', id, result, null);
  }, error => {
    console.log("uncaught6: ", error);
    event.sender.send('daemon-result', id, null, error);
  });
});

ipcMain.on('daemon-post', (event, method, params) => {
  rpc.post(method, params);
});

ipcMain.on('daemon-result', (event, id, result, error) => {
  var cb = callbacks[id];
  if (cb) {
    delete callbacks[id];
    cb(result, error);
  }
});

ipcMain.on('daemon-ping', sendCurrentState);

// Listen for events from the WebSocket RPC instance

function onopen() {
  isopen = true;
  daemon.emit('up');
  if (mainWindow) {
    mainWindow.webContents.send('daemon-up');
  }
}

function onerror() {
  isopen = false;
  daemon.emit('down');
  if (mainWindow) {
    mainWindow.webContents.send('daemon-down');
  }
  return true;
}

function oncall(method, params, id) {
  if (handlers[method]) {
    return handlers[method].apply(daemon, params);
  } else if (mainWindow) {
    return new Promise((resolve, reject) => {
      callbacks[id] = function(result, error) {
        if (error) {
          reject(error);
        } else {
          resolve(result);
        }
      };
      mainWindow.webContents.send('daemon-call', method, params, id);
    });
  } else {
    throw ERROR_METHOD_NOT_FOUND;
  }
}

function onpost(method, params) {
  if (method == 'state') {
    Object.assign(daemon.state, params[0]);
  }
  daemon.emit(method, ...params);
  if (mainWindow) {
    mainWindow.webContents.send('daemon-post', method, params);
  }
}

// Finally put everything together and export the daemon instance

ws = new WebSocketImpl(WebSocket, {
  url: 'ws://127.0.0.1:9337/',
  onerror: onerror,
  onopen: onopen,
  oncall: oncall,
  onpost: onpost,
});

rpc = new RPC(ws);

daemon = new Daemon();

daemon.post = rpc.post;
daemon.call = rpc.call;

module.exports = daemon;