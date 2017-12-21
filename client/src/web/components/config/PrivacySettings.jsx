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
      settings: { overrideDNS: true, firewall: true },
    })
  }
  render() {
    return(
      <div className="pane" data-title="Privacy Settings">
        {/*<CheckboxSetting name="blockMalware" disabled={!this.state.overrideDNS} label="Block Malware"/>
        <CheckboxSetting name="blockAds" disabled={!this.state.overrideDNS} label="Block Ads &amp; Trackers"/>*/}
        <CheckmarkSetting className="group" label="DNS Leak Protection" checked={this.state.overrideDNS}/>
        <CheckmarkSetting className="group" label="IPv6 Leak Protection" checked={true}/>
        <LinkSetting name="firewall" to="/configuration/firewall" label="Internet Killswitch" formatValue={v => ({ 'on': "Always On", 'auto' : "Auto", 'off': "Off" })[v]}/>
        <CheckboxSetting name="allowLAN" className="advanced" indented={true} hidden={!this.props.advanced || this.state.firewall === 'off'} disabled={this.state.firewall === 'off'} label="Always Allow LAN Traffic"/>
      </div>
    );
  }
}
