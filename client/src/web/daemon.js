import { ipcRenderer } from 'electron';
import EventEmitter from 'events';
import { RPC, IPCImpl } from '../rpc.js';
import Loader from './loader.js';
import './unload.js';
import './util.js';

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
    case 'data':
      [ 'config', 'account', 'settings', 'state' ].forEach(type => {
        if (params[0].hasOwnProperty(type)) {
          filterChanges(daemon[type], params[0][type]);
          Object.assign(daemon[type], params[0][type]);
          if (opened) daemon.emit(type, params[0][type]); // deprecated
        }
      });
      break;
    // deprecated
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
  onpost('data', [ data ]);
  [ 'config', 'account', 'settings', 'state' ].forEach(s => onpost(s, [ data[s] ])); // deprecated
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
window.daemon = daemon;

import React from 'react';

export const DaemonAware = (Base = React.Component) => class extends Base {
  constructor(props) {
    super(props);
    this._daemon = {
      onDataChanged: data => {
        let combinedState = Object.assign({}, this.state);
        let changed = false;
        Object.forEach(this._daemon.subscriptions, (category, keyMap) => {
          if (data.hasOwnProperty(category)) {
            changed = true;
            let state = {};
            let values = data[category];
            if (typeof values === 'object' && values) {
              Object.forEach(keyMap, (key, mapper) => {
                if (values.hasOwnProperty(key)) {
                  Object.assign(state, mapper(values[key]));
                }
              });
            }
            this.setState(state);
            this['daemon' + category.charAt(0).toUpperCase() + category.slice(1) + 'Changed'](values); // deprecated
            Object.assign(combinedState, state);
          }
        });
        if (changed) {
          let extra = this.daemonDataChanged(combinedState);
          if (extra && typeof extra === 'object') {
            this.setState(extra);
          }
        }
      },
      subscriptions: { account: {}, config: {}, settings: {}, state: {} }
    };
  }
  componentDidMount() {
    if (super.componentDidMount) super.componentDidMount();
    daemon.on('data', this._daemon.onDataChanged);
  }
  componentWillUnmount() {
    if (super.componentWillUnmount) super.componentWillUnmount();
    daemon.removeListener('data', this._daemon.onDataChanged);
  }
  daemonDataChanged(state) {} // TODO: Rename to daemonStateChanged
  daemonAccountChanged(account) {}
  daemonConfigChanged(config) {}
  daemonSettingsChanged(settings) {}
  daemonStateChanged(state) {}

  daemonSubscribeState(map) {
    let state = {};
    Object.forEach(this._daemon.subscriptions, category => {
      let keyMap = map[category];
      if (keyMap) {
        Object.forEach(keyMap, (key, value) => {
          if (typeof value === 'string') {
            value = (function(name, v) { return { [name] : v }}).bind(null, value);
          } else if (typeof value !== 'function') {
            value = (function(name, v) { return { [name] : v }}).bind(null, key);
          }
          this._daemon.subscriptions[category][key] = value;
          Object.assign(state, value(daemon[category][key]));
        });
      }
    });
    if (!this.state) this.state = {};
    let combinedState = Object.assign({}, this.state);
    Object.assign(this.state, state);
    this.daemonDataChanged(Object.assign(combinedState, state));
  }
};

