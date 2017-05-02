import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import RouteTransition from './Transition';
import { PanelTitlebar } from './Titlebar';
import { Subpanel, PanelContent } from './Panel';
import daemon, { DaemonAware } from '../daemon';

import ApplicationSettings from './config/ApplicationSettings';
import PrivacySettings from './config/PrivacySettings';
import ConnectionSettings from './config/ConnectionSettings';
import CompatibilitySettings from './config/CompatibilitySettings';

import { CheckboxSetting, LinkSetting } from './config/Settings';

const fullVersion = require('electron').remote.app.getVersion();
const shortVersion = fullVersion.replace(/\+.*$/, '');

export default class ConfigurationScreen extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      settings: { showAdvancedSettings: true }
    })
  }
  resetSettings() {
    daemon.post.resetSettings(true);
    daemon.post.applySettings({ showAdvancedSettings: false });
  }
  render() {
    return(
      <Subpanel>
        {this.props.children}
        <PanelContent key="self" className="settings">
          <PanelTitlebar title="Configuration"/>
          <div className="scrollable content">
            <ApplicationSettings advanced={this.state.showAdvancedSettings}/>
            <PrivacySettings advanced={this.state.showAdvancedSettings}/>
            <ConnectionSettings advanced={this.state.showAdvancedSettings}/>
            <CompatibilitySettings advanced={this.state.showAdvancedSettings}/>
            <div className="pane advanced" data-title="Advanced Settings">
              <CheckboxSetting name="showAdvancedSettings" className="advanced" label="Show Advanced Settings"/>
            </div>
            <div className="pane">
              <LinkSetting className="reset" onClick={() => this.resetSettings()} label="Reset Settings to Default"/>
            </div>
            <div className="version footer">
              <div><i className="tag icon"/>{`v${this.state.showAdvancedSettings ? fullVersion : shortVersion}`}</div>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
