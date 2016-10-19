import React from 'react';
import { Link } from 'react-router';
// import { ipcRenderer as ipc } from 'electron';
// import daemon, { DaemonAware } from './daemon.js';
import GeneralSettings from './config/GeneralSettings.js'
import AdvancedSettings from './config/AdvancedSettings.js'


export default class ConfigurationScreen extends React.Component  {
  constructor(props) {
    super(props);
  }

  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Configuration</div>
        </div>
        <div className="ui two item tabular menu cp_config_tabs" ref="tab">
          <a className="item active" data-tab="general">General</a>
          <a className="item" data-tab="advanced">Advanced</a>
        </div>
        <div className="ui tab active tabscroll" data-tab="general">
          <GeneralSettings />
        </div>
        <div className="ui tab tabscroll" data-tab="advanced">
          <AdvancedSettings />
        </div>
      </div>
    );
  }
}
