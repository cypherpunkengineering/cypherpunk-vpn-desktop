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
      <div className={classList("pane", "advanced", { 'hidden': !this.props.advanced })} data-title="Expert Settings">
        <CheckboxSetting name="overrideDNS" className="advanced" hidden={!this.props.advanced} label="Use Cypherpunk DNS"/>
        <CheckboxSetting name="routeDefault" className="advanced" hidden={!this.props.advanced || daemon.account.account.type !== 'developer'} label="Don't Add Default Route" on={false} off={true}/>
        <CheckboxSetting name="exemptApple" className="advanced" hidden={!this.props.advanced || process.platform !== 'darwin'} label="Exempt Apple Services"/>
        <CheckboxSetting name="allowLAN" className="advanced" hidden={!this.props.advanced} label="Always Allow LAN Traffic"/>
      </div>
    );
  }
}
