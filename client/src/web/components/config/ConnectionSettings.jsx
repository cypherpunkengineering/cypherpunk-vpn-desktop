import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class ConnectionSettings extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      settings: { encryption: true },
    })
  }
  render() {
    return (
      <div className="pane" data-title="Connection Settings">
        <LinkSetting name="encryption" to="/configuration/privacy" label="Encryption" formatValue={v => ({ 'default': "Balanced", 'none': "Max Speed", 'strong': "Max Privacy", 'stealth': "Max Stealth" })[v]}/>
        <LinkSetting name="remotePort" className="advanced" hidden={!this.props.advanced} disabled={this.state.encryption==='stealth'} to="/configuration/remoteport" label="Remote Port" formatValue={v => this.state.encryption === 'stealth' ? 'HTTPS' : v.replace(':', ' ').toUpperCase()}/>
        <InputSetting name="localPort" className="advanced" hidden={!this.props.advanced} label="Local Port"/>
      </div>
    );
  }
}
