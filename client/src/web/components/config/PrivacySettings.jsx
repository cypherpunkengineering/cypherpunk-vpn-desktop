import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, CheckmarkSetting } from './Settings';

export default class PrivacySettings extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      settings: { overrideDNS: true },
    })
  }
  render() {
    return(
      <div className="pane" data-title="Privacy Settings">
        <CheckboxSetting name="blockMalware" disabled={!this.state.overrideDNS} label="Block Malware"/>
        <CheckboxSetting name="blockAds" disabled={!this.state.overrideDNS} label="Block Ads"/>
        <CheckmarkSetting label="DNS Leak Protection" checked={this.state.overrideDNS}/>
        <CheckmarkSetting label="IPv6 Leak Protection" checked={true}/>
        <LinkSetting name="firewall" to="/configuration/firewall" label="Internet Killswitch" formatValue={v => ({ 'on': "Always On", 'auto' : "Auto", 'off': "Off" })[v]}/>
      </div>
    );
  }
}
