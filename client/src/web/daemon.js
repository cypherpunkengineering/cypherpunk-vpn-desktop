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
window.daemon = daemon;

import React from 'react';

export const DaemonAware = (Base = React.Component) => class extends Base {
  constructor(props) {
    super(props);
    this._daemonThunks = {
      account: a => { this.daemonChanged('account', a); this.daemonAccountChanged(a); },
      config: c => { this.daemonChanged('config', c); this.daemonConfigChanged(c); },
      settings: s => { this.daemonChanged('settings', s); this.daemonSettingsChanged(s); },
      state: s => { this.daemonChanged('state', s); this.daemonStateChanged(s); },
    };
    this._daemonSubscriptions = { account: {}, config: {}, settings: {}, state: {} };
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
  daemonChanged(category, values) {
    let subscriptions = this._daemonSubscriptions[category];
    Object.forEach(values, (key, value) => {
      if (subscriptions.hasOwnProperty(key)) {
        let s = subscriptions[key];
        let name = s.as || key;
        if (s.filter) value = s.filter(value);
        let oldValue = this.state[name];
        if (s.onBeforeChange) {
          let r = s.onBeforeChange(value, oldValue, name);
          if (r && typeof r === 'object') this.setState(r);
        }
        this.setState({ [name]: value });
        if (s.onChange) {
          let r = s.onChange(value, oldValue, name);
          if (r && typeof r === 'object') this.setState(r);
        }
      }
    });
  }
  daemonAccountChanged(account) {}
  daemonConfigChanged(config) {}
  daemonSettingsChanged(settings) {}
  daemonStateChanged(state) {}

  daemonSubscribeState(values) {
    let state = {};
    let parse = (category, key, args) => {
      let def = {};
      category = category || args.shift();
      key = key || args.shift();
      def.as = args.shift();
      let props = args.shift();
      if (typeof def.as === 'object' && !props) {
        props = def.as;
        delete def.as;
      }
      if (props && typeof props === 'object') {
        Object.assign(def, props);
      }
      this._daemonSubscriptions[category][key] = def;
      state[def.as || key] = (def.filter || (a => a))(daemon[category][key]);
    };
    if (Array.isArray(values)) {
      if (!values.length || Array.isArray(values[0])) {
        // daemonSubscribeState([ [ 'category', 'name', ... ], [ 'category', 'name', ... ] ]);
        values.forEach(value => parse(null, null, value));
      } else {
        // daemonSubscribeState([ 'category', 'name', ... ]);
        parse(null, null, value);
      }
    } else if (values && typeof values === 'object') {
      for (let category of Object.keys(this._daemonSubscriptions)) {
        let categoryValues = values[category];
        if (Array.isArray(categoryValues)) {
          if (!categoryValues.length || Array.isArray(categoryValues[0])) {
            // daemonSubscribeState({ category: [ [ 'name', ... ], [ 'name', ... ] ] });
            categoryValues.forEach(value => parse(category, null, value));
          } else {
            // daemonSubscribeState({ category: [ 'name', ... ], [ 'name', ... ] });
            parse(category, null, value);
          }
        } else if (categoryValues && typeof categoryValues === 'object') {
          // daemonSubscribeState({ category: { name: ..., name: ... } });
          Object.forEach(categoryValues, (key, value) => {
            parse(category, key, Array.isArray(value) ? value : [value]);
          });
        }
      }
    } else {
      // daemonSubscribeState('category', 'name', ...);
      parse(null, null, [...arguments]);
    }
    if (!this.state) this.state = {};
    Object.assign(this.state, state);
  }
};

