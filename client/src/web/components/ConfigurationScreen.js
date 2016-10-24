import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import GeneralSettings from './config/GeneralSettings.js'
import AdvancedSettings from './config/AdvancedSettings.js'
import RouteTransition from './Transition';
import { PanelTitlebar } from './Titlebar';
import Modal from './modal';

const transitionMap = {
  '': { '*': 'swipeLeft' },
  '*': { 'configuration/*': 'swipeRight' },
};

export default class ConfigurationScreen extends React.Component  {
  render() {
    var { props } = this;
    return(
      <Modal className="settings right panel" onClose={() => { History.push('/connect'); }}>
        {/*<RouteTransition transition={transitionMap}>
          {this.props.children || null}*/}
          <div className="panel" key="self" id="settings-main-panel">
            <PanelTitlebar title="Configuration"/>
            <div className="scrollable content">
              <GeneralSettings/>
              <AdvancedSettings/>
            </div>
          </div>
        {/*</RouteTransition>*/}
      </Modal>
    );
  }
}
