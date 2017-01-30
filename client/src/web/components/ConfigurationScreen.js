import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import GeneralSettings from './config/GeneralSettings.js'
import AdvancedSettings from './config/AdvancedSettings.js'
import RouteTransition from './Transition';
import { PanelTitlebar } from './Titlebar';
import Modal from './Modal';
import ReconnectButton from './ReconnectButton';

const transitionMap = {
  '': { '*': 'swipeLeft' },
  '*': { '': 'swipeRight', 'configuration/*': 'swipeRight' },
};

export default class ConfigurationScreen extends React.Component  {
  getContent() {
    if(this.props.children) {
      return this.props.children
    }
    else {
      return(
        <div className="panel" key="self" id="settings-main-panel">
          <PanelTitlebar title="Configuration"/>
          <div className="scrollable content">
            <GeneralSettings/>
            <AdvancedSettings/>
            <div className="version footer">
              <div><i className="tag icon"/>{"v"+require('electron').remote.app.getVersion()}</div>
            </div>
            <ReconnectButton/>
          </div>
        </div>
      );
    }
  }
  render() {
    var { props } = this;
    return(
      <Modal className="settings right panel" onClose={() => { History.push('/connect'); }}>
        <RouteTransition transition={transitionMap}>
          {this.getContent()}
        </RouteTransition>
      </Modal>
    );
  }
}
