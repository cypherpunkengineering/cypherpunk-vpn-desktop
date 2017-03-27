import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import RouteTransition from './Transition';
import { PanelTitlebar } from './Titlebar';
import Modal from './Modal';
import ReconnectButton from './ReconnectButton';
import daemon, { DaemonAware } from '../daemon';

import ApplicationSettings from './config/ApplicationSettings';
import PrivacySettings from './config/PrivacySettings';
import ConnectionSettings from './config/ConnectionSettings';
import CompatibilitySettings from './config/CompatibilitySettings';

import { CheckboxSetting, LinkSetting } from './config/Settings';

const transitionMap = {
  '': { '*': 'swipeLeft' },
  '*': { '': 'swipeRight', 'configuration/*': 'swipeRight' },
};

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
  getContent() {
    if(this.props.children) {
      return this.props.children
    }
    else {
      return(
        <div className="panel" key="self" id="settings-main-panel">
          <PanelTitlebar title="Configuration"/>
          <div className="scrollable content">
            <ApplicationSettings advanced={this.state.showAdvancedSettings}/>
            <PrivacySettings advanced={this.state.showAdvancedSettings}/>
            <ConnectionSettings advanced={this.state.showAdvancedSettings}/>
            <CompatibilitySettings advanced={this.state.showAdvancedSettings}/>
            <div className="pane" data-title="Advanced Settings">
              <CheckboxSetting name="showAdvancedSettings" label="Show Advanced Settings"/>
            </div>
            <div className="pane">
              <LinkSetting className="reset" onClick={() => this.resetSettings()} label="Reset Settings to Default"/>
            </div>
            <div className="version footer">
              <div><i className="tag icon"/>{"v"+require('electron').remote.app.getVersion()}</div>
            </div>
          </div>
        </div>
      );
    }
  }
  render() {
    var { props } = this;
    return(
      <Modal className="settings right panel" onClose={() => { History.push('/main'); }}>
        <RouteTransition transition={transitionMap}>
          {this.getContent()}
        </RouteTransition>
      </Modal>
    );
  }
}
