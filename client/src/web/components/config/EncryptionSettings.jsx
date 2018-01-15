import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';
import ProfileScreen from './ProfileScreen';
import SelectScreen, { Pane, Item } from './SelectScreen';

export const CipherScreen = (props) => (
  <SelectScreen title="Data Encryption" setting="cipher" {...props}>
    <Item value="AES-128-GCM">AES-128 (GCM)</Item>
    <Item value="AES-128-CBC">AES-128 (CBC)</Item>
    <Item value="AES-256-GCM">AES-256 (GCM)</Item>
    <Item value="AES-256-CBC">AES-256 (CBC)</Item>
    <Item value="none">None</Item>
  </SelectScreen>
);

export const AuthScreen = (props) => (
  <SelectScreen title="Data Authentication" setting="auth" {...props}>
    <Item value="SHA1">SHA1</Item>
    <Item value="SHA256">SHA256</Item>
    <Item value="none">None</Item>
  </SelectScreen>
);

export const CertificateScreen = (props) => (
  <SelectScreen title="Handshake" setting="serverCertificate" {...props}>
    <Pane title="Elliptic Curves">
      <Item value="ECDSA-256k1">ECDSA-256k1</Item>
      <Item value="ECDSA-256r1">ECDSA-256r1</Item>
      <Item value="ECDSA-521">ECDSA-521</Item>
    </Pane>
    <Pane title="RSA">
      <Item value="RSA-2048">RSA-2048</Item>
      <Item value="RSA-3072">RSA-3072</Item>
      <Item value="RSA-4096">RSA-4096</Item>
    </Pane>
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
    const isGCM = this.state.cipher.endsWith('GCM');
    return (
      <div className={classList("pane", "advanced", { 'hidden': !this.props.advanced })} data-title="Encryption">
        {/*<LinkSetting name="encryption" className="advanced" to="/configuration/privacy" label="Encryption" formatValue={v => ({ 'default': "Balanced", 'none': "Speed", 'strong': "Privacy", 'stealth': "Stealth" })[v]}/>*/}
        <LinkSetting name="cipher" className="advanced" to="/configuration/cipher" label="Data Encryption"/>
        <LinkSetting name="auth" className="advanced indented" to="/configuration/auth" label="Data Authentication" disabled={isGCM} hidden={isGCM} formatValue={v => this.state.cipher.endsWith('GCM') ? 'none' : v} />
        <LinkSetting name="serverCertificate" className="advanced" to="/configuration/certificate" label="Handshake"/>
      </div>
    );
  }
}
