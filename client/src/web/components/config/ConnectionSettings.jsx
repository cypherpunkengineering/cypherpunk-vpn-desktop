import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class ConnectionSettings extends DaemonAware(React.Component) {
  render() {
    return (
      <div className="pane" data-title="Connection Settings">
        <LinkSetting name="encryption" to="/configuration/privacy" label="Encryption" formatValue={v => ({ 'default': "Recommended", 'none': "Max Speed", 'strong': "Max Privacy", 'stealth': "Max Freedom" })[v]}/>
        <LinkSetting name="remotePort" className="advanced" hidden={!this.props.advanced} to="/configuration/remoteport" label="Remote Port" formatValue={v => v.replace(':', ' ').toUpperCase()}/>
        <InputSetting name="localPort" className="advanced" hidden={!this.props.advanced} label="Local Port"/>
      </div>
    );
  }
}
