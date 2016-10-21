import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import Titlebar, { SecondaryTitlebar } from './Titlebar';
import GeneralSettings from './config/GeneralSettings.js'
import AdvancedSettings from './config/AdvancedSettings.js'
import RouteTransition from './Transition';

const transitionMap = {

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
          {/*<div className="ui fluid inverted borderless icon menu cp_config_header">
            <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
            <div className="header item center aligned">Configuration</div>
          </div>*/}
          {/* <div className="ui two item tabular menu cp_config_tabs" ref="tab">
            <a className="item active" data-tab="general">General</a>
            <a className="item" data-tab="advanced">Advanced</a>
          </div> */}
          <div className="container__comp--config">
            <GeneralSettings/>
            <AdvancedSettings/>
          </div>
          {/* <div className="ui tab active tabscroll" data-tab="general">
            <GeneralSettings />
          </div>
          <div className="ui tab tabscroll" data-tab="advanced">
            <AdvancedSettings />
          </div> */}
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
