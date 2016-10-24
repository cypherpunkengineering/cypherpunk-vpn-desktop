import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import GeneralSettings from './config/GeneralSettings.js'
import AdvancedSettings from './config/AdvancedSettings.js'
import RouteTransition from './Transition';

const transitionMap = {
  '': { '*': 'swipeLeft' },
  '*': { '': 'swipeRight' },
};

export default class ConfigurationScreen extends React.Component  {
  constructor(props) {
    super(props);
    this.cloneChildren = this.cloneChildren.bind(this);
  }

  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  cloneChildren() {
    var path = this.props.location.pathname;
    // console.log(this.props.children);
    // console.log(path);
    // TODO: Should preserve previous menu while displaying subscreen
    if (this.props.children) {
      return React.cloneElement(this.props.children, { key: path })
    }
    else {
      return (
        <div id="config-screen" className="container__comp">
          <SecondaryTitlebar title="Configuration" back="/connect"/>
          <div className="container__comp--config">
            <GeneralSettings/>
            <AdvancedSettings/>
          </div>
        </div>
      )
    }
  }
  render() {
    var { props } = this;
    return(
      <RouteTransition transition={transitionMap}>
        {this.cloneChildren()}
      </RouteTransition>
    );
  }
}
