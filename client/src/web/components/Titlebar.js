import React from 'react';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory } from 'react-router';

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


// Copy of window close button (for use in modals)

export class CloseButton extends React.Component {
  componentDidMount() {
    $('body > #window-close').css({ display: 'none' });
  }
  componentWillUnmount() {
    $('body > #window-close').css({ display: '' });
  }
  render() {
    return (process.platform === 'darwin') ? null : <i id="window-close" className="link icon" onClick={() => { window.close(); }}/>; 
  }
}


// The Dragbar is a minimal component that sits at the top of the frameless
// window and provides space for the OS controls and acts as a draggable
// region to be able to move the window. On OS X, the minimize/close buttons
// are automatically provided, whereas we must draw them ourselves on Windows.

export class Dragbar extends React.Component {
  constructor(props) {
    super(props);
  }
  render() {
    var className = "cp dragbar";
    var styles={};
    if (this.props.className) {
      className += " " + this.props.className;
    }
    if (this.props.height) {
      styles.height = this.props.height;
    }
    return(
      <div id="dragbar" className={className} style={styles}>
      </div>
    );
  }
}


// On the main screen, the Titlebar is a larger version of the Dragbar that
// also provides a title and the main navigation controls.

export class Titlebar extends React.Component {
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


// Titlebar variation for the main connect screen (contains main navigation buttons)

export class MainTitlebar extends React.Component {
  render() {
    return(
      <Titlebar>
        <Link className="account" to="/account" data-tooltip="Account" data-inverted="" data-position="bottom left"><img src={AccountIcon}/></Link>
        <Title/>
        <Link className="settings" to="/configuration" data-tooltip="Settings" data-inverted="" data-position="bottom right"><img src={SettingsIcon} /></Link>
      </Titlebar>
    );
  }
}


// Titlebar variation for secondary screens (contain just a back button)

export class SecondaryTitlebar extends React.Component {
  render() {
    return(
      <Titlebar>
        <Link className="back" to={this.props.back}><i className="angle left icon"></i></Link>
        {this.props.title}
        <a style={{ visibility: 'hidden' }}/>
      </Titlebar>
    );
  }
}


// Titlebar variation for panels attached to the main screen (no navigation buttons)

export class PanelTitlebar extends React.Component {
  render() {
    return(
      <Titlebar>
        {this.props.title}
      </Titlebar>
    )
  }
}
