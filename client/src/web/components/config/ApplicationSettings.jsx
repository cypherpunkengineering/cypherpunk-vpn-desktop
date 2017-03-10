import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class ApplicationSettings extends React.Component {
  constructor(props) {
    super(props);
    this.listeners = {
      autostart: (event, enabled) => this.onAutoStartSettingChanged(enabled),
    };
  }
  componentWillMount() {
    ipc.on('autostart-value', this.listeners.autostart);
    ipc.send('autostart-get');
  }
  componentDidMount() {
    var self = this;
    $(this.refs.runOnStartup).checkbox({ onChange: function() { self.onAutoStartSettingClicked(this.checked); } });
  }
  componentWillUnmount() {
    ipc.removeListener('autostart-value', this.listeners.autostart);
  }
  onAutoStartSettingChanged(enabled) {
    $(this.refs.runonstartup).parent().checkbox('set ' + (enabled ? 'checked' : enabled === null ? 'indeterminate' : 'unchecked'));
  }
  onAutoStartSettingClicked(enabled) {
    ipc.send('autostart-set', enabled);
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
