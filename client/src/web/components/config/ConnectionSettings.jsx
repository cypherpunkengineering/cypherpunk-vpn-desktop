import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';
import ProfileScreen from './ProfileScreen';
import SelectScreen, { Pane, Item } from './SelectScreen';


export const ProtocolScreen = (props) => (
  <SelectScreen title="Transport Protocol" setting="protocol" {...props}>
    <Item value="udp">UDP</Item>
    <Item value="tcp">TCP</Item>
  </SelectScreen>
);

export const RemotePortScreen = (props) => (
  <SelectScreen title="Remote Port" setting="remotePort" type="number" {...props}>
    <Item value={0}>Auto</Item>
    <Item value={53}>53</Item>
    <Item value={1194}>1194</Item>
    <Item value={8080}>8080</Item>
    <Item value={9021}>9021</Item>
  </SelectScreen>
);

export default class ConnectionSettings extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      settings: { encryption: true, cipher: true, auth: true, serverCertificate: true },
    })
  }
  render() {
    return (
      <div className={classList("pane", "advanced", { 'hidden': !this.props.advanced })} data-title="Connection">
        <LinkSetting name="protocol" className="advanced" hidden={!this.props.advanced} disabled={this.state.encryption==='stealth'} to="/configuration/protocol" label="Transport Protocol" formatValue={v => v.toUpperCase()}/>
        <LinkSetting name="remotePort" className="advanced" hidden={!this.props.advanced} disabled={this.state.encryption==='stealth'} to="/configuration/remoteport" label="Remote Port" formatValue={v => v || "auto"}/>
        <InputSetting name="localPort" className="advanced" hidden={!this.props.advanced} label="Local Port"/>
      </div>
    );
  }
}
