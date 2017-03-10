import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class CompatibilitySettings extends React.Component {
  render() {
    return(
      <div className={classList("pane", { 'hidden': !this.props.advanced })} data-title="Compatibility Settings">
        <CheckboxSetting name="overrideDNS" hidden={!this.props.advanced} label="Use Existing DNS Servers" on={false} off={true}/>
        <CheckboxSetting name="routeDefault" hidden={!this.props.advanced} label="Don't Add Default Route" on={false} off={true}/>
        <CheckboxSetting name="exemptApple" hidden={!this.props.advanced || process.platform !== 'darwin'} label="Exempt Apple Services"/>
        <CheckboxSetting name="allowLAN" hidden={!this.props.advanced} label="Always Allow LAN Traffic"/>
      </div>
    );
  }
}
