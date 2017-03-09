import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class AdvancedSettings extends React.Component  {
  state = {
    advanced: !!daemon.settings.showAdvancedSettings
  }
  onAdvancedClick() {
    var advanced = !this.state.advanced;
    this.setState({ advanced: advanced });
    daemon.post.applySettings({ showAdvancedSettings: advanced });
  }
  render() {
    return(
      <div className={"collapsible" + (this.state.advanced ? " open" : "")} ref="root">
        <div className="collapsible-title" onClick={e => this.onAdvancedClick(e)}>Advanced Settings</div>
        <div className="collapsible-content">
          <div className="pane" data-title="Privacy Settings">
            <CheckboxSetting name="overrideDNS" label="Use Cypherpunk DNS"/>
            <CheckboxSetting name="optimizeDNS" indented hidden={daemon.account.account.type !== 'developer'} label="Force CypherPlay&trade;"/>
            <CheckboxSetting name="blockAds" indented label="Block Ads"/>
            <CheckboxSetting name="blockMalware" indented label="Block Malware"/>
            <LinkSetting name="firewall" to="/configuration/firewall" label="Internet Killswitch" formatValue={v => ({ 'on': "Always On", 'auto' : "Auto", 'off': "Off" })[v]}/>
            <CheckboxSetting name="allowLAN" indented label="Always Allow LAN Traffic"/>
            {/* hide until feature works correctly */}
            <CheckboxSetting name="blockIPv6" indented hidden label="Block IPv6 Traffic"/>
          </div>

          <div className={classList('pane', { 'hidden': !(true || daemon.account.account.type === 'developer' || process.platform === 'darwin') })} data-title="Routing Settings">
            <CheckboxSetting name="routeDefault" hidden={!(true || daemon.account.account.type === 'developer')} label="Route Internet Traffic via VPN"/>
            <CheckboxSetting name="exemptApple" indented hidden={process.platform !== 'darwin'} label="Exempt Apple Services"/>
          </div>

          <div className="pane" data-title="Connection Settings">
            <LinkSetting name="remotePort" to="/configuration/remoteport" label="Remote Port" formatValue={v => v.replace(':', ' ').toUpperCase()}/>
            <InputSetting name="localPort" label="Local Port"/>
          </div>
        </div>
      </div>
    );
  }
}
