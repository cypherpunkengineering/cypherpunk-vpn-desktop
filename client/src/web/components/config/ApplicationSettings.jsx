import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import analytics from '../../analytics';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

let autostartEnabled = false;
let autostartListeners = [];

ipc.on('autostart-value', (event, enabled) => {
  autostartEnabled = enabled;
  for (let listener of autostartListeners) {
    if (listener.onAutoStartSettingChanged) listener.onAutoStartSettingChanged(enabled);
  }
});


export default class ApplicationSettings extends React.Component {
  componentWillMount() {
    ipc.send('autostart-get'); // refresh autostart value
  }
  componentDidMount() {
    var self = this;
    $(this.refs.runOnStartup).checkbox({ onChange: function() { self.onAutoStartSettingClicked(this.checked); } });
    this.onAutoStartSettingChanged(autostartEnabled);
    autostartListeners.push(this);
  }
  componentWillUnmount() {
    autostartListeners = autostartListeners.filter(l => l !== this);
  }
  onAutoStartSettingChanged(enabled) {
    $(this.refs.runOnStartup).checkbox('set ' + (enabled ? 'checked' : enabled === null ? 'indeterminate' : 'unchecked'));
  }
  onAutoStartSettingClicked(enabled) {
    ipc.send('autostart-set', enabled);
    analytics.event('Setting', 'autostart', { label: enabled });
  }
  render() {
    return(
      <div className="pane" data-title="App Settings">
        <div className="setting">
          <div className="ui toggle checkbox" ref="runOnStartup">
            <input type="checkbox" name="runonstartup" id="runonstartup" onChange={event => this.onAutoStartSettingClicked(event)}/>
            <label>Launch on Startup</label>
          </div>
        </div>
        <CheckboxSetting name="autoConnect" label="Connect on Launch"/>
        <CheckboxSetting name="showNotifications" label="Show Desktop Notifications"/>
      </div>
    );
  }
}
