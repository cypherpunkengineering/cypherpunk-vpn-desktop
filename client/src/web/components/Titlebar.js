import React from 'react';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory, hashHistory as History } from 'react-router';

import AccountIcon from '../assets/img/icon-account-big.svg';
import SettingsIcon from '../assets/img/icon_settings.svg';

// A little helper component to render the main title of the app.

export class Title extends React.Component {
  constructor(props) {
    super(props);
  }
  render() {
    var ActualComponent = this.props.component;
    return(
      <ActualComponent className={"cp title " + this.props.className}><span>{this.props.left}</span><span>{this.props.right}</span></ActualComponent>
    );
  }
}
Title.defaultProps = { component: 'div', left: "Cypherpunk", right: "Privacy", className: "" };


// On the main screen, the Titlebar is a larger version of the Dragbar that
// also provides a title and the main navigation controls.

export default class Titlebar extends React.Component {
  constructor(props) {
    super(props);
    this._handler= this._handler.bind(this);
  }
  _handler(props) {
    // handler scope doesn't know what this is unless you call bind
    console.log(props);
  }
  render() {
    return(
      <div id="titlebar" className="cp titlebar">
        {this.props.children}
      </div>
    );
  }
}

export class MainTitlebar extends React.Component {
  render() {
    return(
      <Titlebar>
        <Link className="account" to="/status" data-tooltip="Account" data-inverted="" data-position="bottom left"><img src={AccountIcon} /></Link>
        <Title/>
        <Link className="settings" to="/configuration" data-tooltip="Settings" data-inverted="" data-position="bottom right"><img src={SettingsIcon} /></Link>
      </Titlebar>
    );
  }
}

export class SecondaryTitlebar extends React.Component {
  render() {
    return(
      <Titlebar>
        <Link className="back" to={this.props.back}><i className="arrow left icon"></i></Link>
        {this.props.title}
        <a style={{ visibility: 'hidden' }}/>
      </Titlebar>
    );
  }
}
