const { ipcMain, app } = require('electron');
const EventEmitter = require('events');
const { RPC, WebSocketImpl, ERROR_METHOD_NOT_FOUND } = require('./rpc.js');
const WebSocket = require('ws');
//const WebSocket = require('websocket').client;

if (!app.isReady()) {
  throw new Error('Initialization order error');
}

var callbacks = {};
var handlers = {};
var ws, rpc, daemon;
var isopen = false;
var connectResolve = null, connectReject = null;

function filterChanges(target, delta) {
  var count = 0;
  Object.keys(delta).forEach(d => {
    if (typeof target[d] === typeof delta[d]) {
      if (JSON.stringify(target[d]) === JSON.stringify(delta[d])) {
        delete delta[d];
        return;
      }
    }
    count++;
  });
  return count;
}

function buildStatusReply() {
  return isopen ? {
    'state': daemon.state,
    'account': daemon.account,
    'config': daemon.config,
    'settings': daemon.settings,
  } : null;
}

// Set up a Daemon class which is also an EventEmitter (for notifications).
// In addition to actual RPC calls, the special events 'up' and 'down'
// notify that the connection to the daemon is up or down, respectively.

class Daemon extends EventEmitter {
  constructor() {
    super();
    this.account = {};
    this.config = {};
    this.settings = {};
    this.state = {};
    this.setMaxListeners(0);
  }
  registerMethod(method, callback) {
    handlers[method] = callback;
  }
  unregisterMethod(method) {
    delete handlers[method];
  }
  notifyWindowCreated() {
    if (window) window.webContents.send(isopen ? 'daemon-up' : 'daemon-down', buildStatusReply());
  }
  connect() {
    if (connectResolve || connectReject) {
      throw new Error("Already connecting");
    }
    return new Promise((resolve, reject) => {
      connectResolve = resolve;
      connectReject = reject;
    });
  }
  disconnect() {
    return ws.disconnect();
  }
  get connected() {
    return isopen;
  }
}

// Listen for requests and replies from the renderer

ipcMain.on('daemon-call', (event, method, params, id) => {
  rpc.call(method, params).then(result => {
    event.sender.send('daemon-result', id, result, null);
  }, error => {
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

ipcMain.on('daemon-open', (event) => {
  event.returnValue = buildStatusReply();
});

// Listen for events from the WebSocket RPC instance

function onopen() {
  // wait for the first 'data' message
}

function onerror() {
  isopen = false;
  daemon.emit('down');
  if (window) window.webContents.send('daemon-down', buildStatusReply());
  return true;
}

function oncall(method, params, id) {
  if (handlers[method]) {
    return handlers[method].apply(daemon, params);
  } else if (window) {
    return new Promise((resolve, reject) => {
      callbacks[id] = function(result, error) {
        if (error) {
          reject(error);
        } else {
          resolve(result);
        }
      };
      window.webContents.send('daemon-call', method, params, id);
    });
  } else {
    throw ERROR_METHOD_NOT_FOUND;
  }
}

function onpost(method, params) {
  if (!isopen) {
    if (method === 'data' && typeof params[0] === 'object' && params[0].hasOwnProperty('version')) {
      if (params[0].version !== app.getVersion()) {
        if (connectReject) {
          connectReject(Object.assign(new Error("Daemon version mismatch"), { daemonVersion: params[0].version }));
        } else {
          daemon.emit('error', { message: "Daemon version mismatch", daemonVersion: params[0].version });
        }
        daemon.disconnect();
        return;
      }
      delete params[0].version;
      isopen = true;
      if (connectResolve) {
        connectResolve();
        connectResolve = null;
      }
      daemon.emit('up');
      if (window) window.webContents.send('daemon-up', buildStatusReply());
    } else {
      return;
    }
  }

  switch (method) {
    case 'data':
      ['config','account','settings','state'].forEach(type => {
        if (params[0].hasOwnProperty(type)) {
          filterChanges(daemon[type], params[0][type]);
          Object.assign(daemon[type], params[0][type]);
          try {
            daemon.emit(type, params[0][type]); // deprecated
          } catch(e) {}
        }
      });
      break;
    // deprecated
    case 'account':
    case 'config':
    case 'settings':
    case 'state':
      filterChanges(daemon[method], params[0]);
      Object.assign(daemon[method], params[0]);
      break;
  }
  try {
    if (window) window.webContents.send('daemon-post', method, params);
    daemon.emit(method, ...params);
  } catch(e) {}
}

// Finally put everything together and export the daemon instance

const DEFAULT_PORT = 9337;
let port = DEFAULT_PORT;
if (__dirname.indexOf('/client/app/') < 0) { // installed build
  let path = (process.platform === 'win32') ? require('path').resolve(app.getPath('exe'), '../daemon.lock') : '/usr/local/cypherpunk/var/daemon.lock';
  try {
    port = Number.parseInt(require('fs').readFileSync(path, 'utf8'), 10);
    port = (port > 0) ? port : DEFAULT_PORT;
  } catch(e) {}
}

ws = new WebSocketImpl(WebSocket, {
  url: `ws://127.0.0.1:${port}/`,
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
