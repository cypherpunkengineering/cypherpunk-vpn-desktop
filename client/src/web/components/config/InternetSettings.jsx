import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';
import { classList } from '../../util.js';

import { CheckboxSetting, LinkSetting, InputSetting } from './Settings';

export default class InternetSettings extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      settings: { overrideDNS: true },
    });
  }
  render() {
    return(
      <div className={classList("pane", "advanced", { 'hidden': !this.props.advanced })} data-title="Internet">
        <CheckboxSetting name="overrideDNS" className="" hidden={!this.props.advanced} label="Use Existing DNS" on={false} off={true}/>
        <CheckboxSetting name="routeDefault" className="" hidden={!this.props.advanced || daemon.account.account.type !== 'developer'} label="Use Split Tunneling" on={false} off={true}/>
        <CheckboxSetting name="exemptApple" className="" hidden={!this.props.advanced || process.platform !== 'darwin'} label="Exempt Apple Services"/>
        {/*<CheckboxSetting name="optimizeDNS" className="advanced" hidden={!this.props.advanced || (daemon.account.account.type !== 'developer' && daemon.account.account.type !== 'staff')} disabled={!this.state.overrideDNS} label="Force CypherPlay&trade;"/>*/}
        {/*<CheckboxSetting name="allowLAN" className="advanced" hidden={!this.props.advanced} label="Always Allow LAN Traffic"/>*/}
      </div>
    );
  }
}
