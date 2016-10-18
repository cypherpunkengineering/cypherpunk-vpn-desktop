import React from 'react';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory, hashHistory as History } from 'react-router';
import InfoIcon from '../assets/img/icon_info.svg';
import SettingsIcon from '../assets/img/icon_settings.svg';

export default class Titlebar extends React.Component {
  constructor(props) {
    super(props);
    this._handler= this._handler.bind(this);
  }

  _handler(props) {
    // handler scope doesn't know what this is unless you call bind
    console.log(props);
  }

  componentDidMount() {
    $(this.refs.dropdown).dropdown({ action: 'hide' });
  }
  render() {
    return(
      <div id="titlebar" className="ui fixed borderless icon menu">
        <Link className="item" to="/status"><img src={InfoIcon} height="22" width="22" /></Link>
        <div className="header item" style={{ flexGrow: 1, justifyContent: "center" }}>Cypherpunk</div>
        <Link className="item" to="/configuration"><img src={SettingsIcon} height="22" width="22" /></Link>
      </div>
    );
  }
}
