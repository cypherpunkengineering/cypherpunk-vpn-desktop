import { ipcRenderer } from 'electron';
import EventEmitter from 'events';
import { RPC, IPCImpl } from '../rpc.js';
import Loader from './loader.js';
import './unload.js';

var ipc, rpc, daemon;
var opened = false;
var errorCount = 0;
var readyCallbacks = [];

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
      filterChanges(daemon[method], params[0]);
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

export default daemon;

import React from 'react';

export const DaemonAware = (Base = React.Component) => class extends Base {
  constructor(props) {
    super(props);
    this._daemonThunks = {
      account: a => this.daemonAccountChanged(a),
      config: c => this.daemonConfigChanged(c),
      settings: s => this.daemonSettingsChanged(s),
      state: s => this.daemonStateChanged(s),
    }; 
  }
  componentDidMount() {
    if (super.componentDidMount) super.componentDidMount();
    daemon.on('account', this._daemonThunks.account);
    daemon.on('config', this._daemonThunks.config);
    daemon.on('settings', this._daemonThunks.settings);
    daemon.on('state', this._daemonThunks.state);
  }
  componentWillUnmount() {
    if (super.componentWillUnmount) super.componentWillUnmount();
    daemon.removeListener('state', this._daemonThunks.state);
    daemon.removeListener('settings', this._daemonThunks.settings);
    daemon.removeListener('config', this._daemonThunks.config);
    daemon.removeListener('account', this._daemonThunks.account);
  }
  daemonAccountChanged(account) {}
  daemonConfigChanged(config) {}
  daemonSettingsChanged(settings) {}
  daemonStateChanged(state) {}
};

