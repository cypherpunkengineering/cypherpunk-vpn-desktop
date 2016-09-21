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
    this.account = {};
    this.config = {};
    this.settings = {};
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
  switch (method) {
    case 'account':
    case 'config':
    case 'settings':
    case 'state':
      //params[0] = filterChanged(daemon[method], params[0]);
      daemon[method] = Object.assign(daemon[method], params[0]);
      break;
  }
  if (opened) daemon.emit(method, ...params);
}

function up(event, data) {
  [ 'config', 'account', 'settings', 'state' ].forEach(s => onpost(s, [ data[s] ]));
  opened = true;
  errorCount = 0;
  var cbs = readyCallbacks;
  readyCallbacks = [];
  cbs.forEach(cb => cb());
  Loader.hide();
}

function down(event, dummy) {
  errorCount++;
  Loader.show(/*opened ? "Reconnecting" : "Loading"*/);
}

ipc = new IPCImpl({
  onpost: onpost,
});
rpc = new RPC(ipc);

daemon = new Daemon();

daemon.post = rpc.post;
daemon.call = rpc.call;

ipcRenderer.on('daemon-up', up);
ipcRenderer.on('daemon-down', down);

(function(){
  var initialState = ipcRenderer.sendSync('daemon-open');
  if (initialState !== null) {
    up(null, initialState);
  } 
})();

module.exports = daemon;
