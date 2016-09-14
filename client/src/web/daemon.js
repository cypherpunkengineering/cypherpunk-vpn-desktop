import { ipcRenderer } from 'electron';
import EventEmitter from 'events';
import { RPC, IPCImpl } from '../rpc.js';
import Loader from './loader.js';
import './unload.js';

var ipc, rpc, daemon;
var opened = false;
var errorCount = 0;
var readyCallbacks = [];

// Set up a Daemon class which is also an EventEmitter (for notifications).
// In addition to actual RPC calls, the special events 'up' and 'down'
// notify that the connection to the daemon is up or down, respectively.

class Daemon extends EventEmitter {
  constructor() {
    super();
    this.state = {};
  }
  registerMethod(method, callback) {
    rpc.registerMethod(method, callback);
  }
  unregisterMethod(method) {
    rpc.unregisterMethod(method);
  }
  ready(callback) {
    if (opened) {
      callback();
    } else {
      readyCallbacks.push(callback);
    }
  }
}

function onpost(method, params) {
  if (daemon) {
    if (method == 'state') {
      Object.assign(daemon.state, params[0]);
    }
    daemon.emit(method, ...params);
  }
}

function up() {
  opened = true;
  errorCount = 0;
  var cbs = readyCallbacks;
  readyCallbacks = [];
  cbs.forEach(cb => cb());
  Loader.hide();
}

function down() {
  errorCount++;
  Loader.show(opened ? "Reconnecting" : "Loading");
}

ipc = new IPCImpl({
  onpost: onpost,
});
rpc = new RPC(ipc);

daemon = new Daemon();

daemon.on('state', state => {
  Object.assign(daemon.state, state);
})

daemon.post = rpc.post;
daemon.call = rpc.call;

ipcRenderer.on('daemon-up', up);
ipcRenderer.on('daemon-down', down);
if (ipcRenderer.sendSync('daemon-ping') == 'up') {
  up();
}

module.exports = daemon;
