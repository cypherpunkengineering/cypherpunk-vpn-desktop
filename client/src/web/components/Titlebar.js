import React from 'react';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory } from 'react-router';
import RetinaImage from './Image';
import { classList } from '../util';

const AccountIcon = { [1]: require('../assets/img/pia_account_icon.png'), [2]: require('../assets/img/pia_account_icon@2x.png') };
import SettingsIcon from '../assets/img/icon_settings.svg';

const LogoText = require('../assets/img/pia_logo_text_190.png');
const LogoText2x = require('../assets/img/pia_logo_text_190@2x.png');


// A little helper component to render the main title of the app.

export const Title = ({ component = 'div', left = "Cypherpunk", right = "Privacy", className, ...props } = {}) => {
  const ActualComponent = component;
  return <ActualComponent className={classList('cp title', className)} {...props}><RetinaImage src={{ 1: LogoText, 2: LogoText2x }} alt="Private Internet Access"/></ActualComponent>;
}


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
        {this.props.children}
      </div>
    );
  }
}


// On the main screen, the Titlebar is a larger version of the Dragbar that
// also provides a title and the main navigation controls.

export class Titlebar extends React.Component {
  constructor(props) {
    super(props);
  }
  render() {
    return(
      <div id="titlebar" className={classList("cp titlebar", this.props.className)}>
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
        <Link className="account" to="/account" data-tooltip="My Account" data-inverted="" data-position="bottom left"><RetinaImage src={AccountIcon}/></Link>
        <Title/>
        <Link className="settings" to="/configuration" data-tooltip="Configuration" data-inverted="" data-position="bottom right"><img src={SettingsIcon} /></Link>
      </Titlebar>
    );
  }
}


// Titlebar variation for secondary screens (contain just a back button)

export class SecondaryTitlebar extends React.Component {
  render() {
    return(
      <Titlebar className="secondary">
        <Link className="back" to={this.props.back}><i className="angle left icon"></i></Link>
        <span>{this.props.title}</span>
        <a style={{ visibility: 'hidden' }}/>
      </Titlebar>
    );
  }
}


// Titlebar variation for panels attached to the main screen (no navigation buttons)

export class PanelTitlebar extends React.Component {
  render() {
    return(
      <Titlebar className="secondary">
        <span>{this.props.title}</span>
      </Titlebar>
    )
  }
}
