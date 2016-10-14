window.$ = window.jQuery = require('jquery');

import './assets/css/app.less';
import 'semantic';

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory/*, hashHistory as History*/ } from 'react-router';

import SpinningImage from './assets/img/bgring3.png';
import CypherPunkLogo from './assets/img/cp_logo1.png';
import InfoIcon from './assets/img/icon_info.svg';
import SettingsIcon from './assets/img/icon_settings.svg';


import daemon from './daemon.js';

import StatusScreen from './StatusScreen.jsx';
import ConfigurationScreen from './ConfigurationScreen.jsx';
import FirewallScreen from './ConfigurationScreen/FirewallScreen.jsx';
import EncryptionScreen from './ConfigurationScreen/EncryptionScreen.jsx';
import PasswordScreen from './ConfigurationScreen/PasswordScreen.jsx';
import EmailScreen from './ConfigurationScreen/EmailScreen.jsx';
import HelpScreen from './ConfigurationScreen/HelpScreen.jsx';

window.History = createMemoryHistory(window.location.href);

daemon.ready(() => {
  daemon.once('state', state => {
    History.push('/login');
  });
  daemon.post.get('state');
});

function humanReadableSize(count) {
  if (count >= 1024 * 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024 / 1024) / 10).toFixed(1) + "T";
  } else if (count >= 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024) / 10).toFixed(1) + "G";
  } else if (count >= 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024) / 10).toFixed(1) + "M";
  } else if (count >= 1024) {
    return parseFloat(Math.round(count * 10 / 1024) / 10).toFixed(1) + "K";
  } else {
    return parseFloat(count).toFixed(0);
  }
}

class MainBackground extends React.Component {
  render() {
    return(
      <div id="main-background">
        <div>
          <img class="r1" src={SpinningImage}/>
          <img class="r2" src={SpinningImage}/>
          <img class="r3" src={SpinningImage}/>
          <img class="r4" src={SpinningImage}/>
          <img class="r5" src={SpinningImage}/>
          <img class="r6" src={SpinningImage}/>
          <img class="r7" src={SpinningImage}/>
        </div>
      </div>
    );
  }
}

class Titlebar extends React.Component {
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

class LoadDimmer extends React.Component {
  render() {
    return(
      <div id="load-screen" class="full screen ui active dimmer" style={{visibility: 'visible', zIndex: 20}}>
        <div class="ui big text loader">Loading</div>
      </div>
    );
  }
}

class ConnectScreen extends React.Component {
  constructor(props) {
    super(props);
    Object.assign(this.state, this.translateDaemonState(daemon.state));

    this.handleConnectClick = this.handleConnectClick.bind(this);
    this.handleRegionSelect = this.handleRegionSelect.bind(this);
    this.handleDaemonSettingsChange = this.handleDaemonSettingsChange.bind(this);
    this.handleDaemonStateChange = this.handleDaemonStateChange.bind(this);
  }

  state = {
    regions: daemon.config.servers,
    selectedRegion: daemon.settings.server,
    receivedBytes: 0,
    sentBytes: 0,
    connectionState: 'disconnected',
  }

  translateDaemonState(state) {
    var newState = {};
    var stateString = {
      'CONNECTING': 'connecting',
      'CONNECTED': 'connected',
      'DISCONNECTING': 'disconnecting',
      'DISCONNECTED': 'disconnected',
    }[state.state];
    if (stateString)
      newState.connectionState = stateString;
    if (state.bytesReceived !== undefined)
      newState.receivedBytes = state.bytesReceived;
    if (state.bytesSent !== undefined)
      newState.sentBytes = state.bytesSent;
    return newState;
  }
  handleDaemonSettingsChange(settings) {
    if (settings.server) {
      this.setState({ selectedRegion: settings.server });
      $(this.refs.regionDropdown).dropdown('set selected', settings.server);
    }
  }
  handleDaemonStateChange(state) {
    this.setState(this.translateDaemonState(state));
  }
  componentDidMount() {
    daemon.on('settings', this.handleDaemonSettingsChange);
    daemon.on('state', this.handleDaemonStateChange);
    $(this.refs.regionDropdown).dropdown({
      onChange: this.handleRegionSelect
    });
  }
  componentWillUnmount() {
    daemon.removeListener('state', this.handleDaemonStateChange);
    daemon.removeListener('settings', this.handleDaemonSettingsChange);
  }
  shouldComponentUpdate(nextProps, nextState) {
    return true;
  }
  componentWillUpdate(nextProps, nextState) {
    if (this.state.connectionState !== nextState.connectionState) {
      $(document.body).removeClass(this.state.connectionState).addClass(nextState.connectionState);
      if (nextState.connectionState === 'disconnected')
        $('#main-background').removeClass('animating');
      else
        $('#main-background').addClass('animating');
    }
  }
  render() {
    var connectCircleStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: (this.state.connectionState === 'connected' ? '1' : this.state.connectionState == 'disconnected' ? '1' : '0.75'),
      strokeWidth: '20',
      strokeDasharray: '314.16',
      strokeDashoffset: '78.54',
      strokeLinecap: 'round',
      transform: 'rotate(-45deg)',
      transformOrigin: '50% 200px',
      fill: 'none'
    }
    var connectLineStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: (this.state.connectionState === 'connected' ? '1' : this.state.connectionState == 'disconnected' ? '1' : '0.75'),
      strokeWidth: '20',
      strokeDasharray: '83',
      strokeDashoffset: (this.state.connectionState === 'connected' ? '0' : this.state.connectionState == 'disconnected' ? '127' : '83'),
      strokeLinecap: 'round',
      fill: 'none'
    }
    var connectSmallLineStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: '1',
      strokeWidth: '20',
      strokeDasharray: '20',
      strokeDashoffset: (this.state.connectionState === 'connected' ? '0' : this.state.connectionState == 'disconnected' ? '20' : '20'),
      strokeLinecap: 'round',
      fill: 'none'
    }
    var buttonLabel = {
      'disconnected': "Tap to protect",
      'connecting': "Connecting...",
      'connected': "You are protected",
      'disconnecting': "Disconnecting...",
    }[this.state.connectionState];

    return(
      <div id="connect-screen" class="full screen" style={{visibility: 'visible'}}>
        <Titlebar/>
        <div id="connect-container">

          <svg width="120" height="400" ref="connectButton" onClick={this.handleConnectClick}>
            <circle cx="60" cy="200" r="50" style={connectCircleStyle} />
            <line x1="60" y1="98" x2="60" y2="181" style={connectLineStyle} />
            <line x1="60" y1="111" x2="80" y2="111"  style={connectSmallLineStyle} />
            <line x1="60" y1="137" x2="80" y2="137"  style={connectSmallLineStyle} />
          </svg>

          {/*<i id="connect" class={"ui fitted massive power link icon" + (this.state.connectionState === 'connected' ? " green" : this.state.connectionState == 'disconnected' ? " red" : " orange disabled")} ref="connectButton" onClick={this.handleConnectClick}></i>*/}
        </div>
        <div id="connect-status" ref="connectStatus">{buttonLabel}</div>
        <div id="region-select" class={"ui selection dropdown" + (this.state.connectionState === 'disconnected' ? "" : " disabled")} ref="regionDropdown">
          <input type="hidden" name="region" value={this.state.selectedRegion}/>
          <i class="dropdown icon"></i>
          <div class="default text">Select Region</div>
          <div class="menu">
            { this.state.regions.map(s => <div class="item" data-value={s.id} key={s.id}><i class={s.country + " flag"}></i>{s.name}</div>) }
          </div>
        </div>
        <div id="connection-stats" class="ui two column center aligned grid">
          <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.receivedBytes)}</div><div class="label">Received</div></div></div>
          <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.sentBytes)}</div><div class="label">Sent</div></div></div>
        </div>
      </div>
    );
  }
  handleConnectClick() {
    switch (this.state.connectionState) {
      case 'disconnected':
        this.setState({ connectionState: 'connecting' });
        daemon.post.applySettings({
          protocol: "udp",
          server: this.getSelectedRegion(),
        });
        daemon.post.connect();
        daemon.post.get('state');
        break;
      case 'connecting':
      case 'connected':
        this.setState({ connectionState: 'disconnecting' });
        daemon.post.disconnect();
        daemon.post.get('state');
        break;
      case 'disconnecting':
        break;
    }
  }
  getSelectedRegion() {
    return $(this.refs.regionDropdown).dropdown('get value');
  }
  handleRegionSelect(server) {
    daemon.post.applySettings({ server: server });
  }
}

class LoginScreen extends React.Component {
  componentDidMount() {
    $(this.refs.dimmer).dimmer({ closable: false });
    this.refs.dimmer.addEventListener('cancel', this.onLoginCancel.bind(this));
  }
  onUsernameKeyPress(event) {
    if (event.which == 13) {
      event.preventDefault();
      this.refs.password.focus();
    }
  }
  onPasswordKeyPress(event) {
    if (event.which == 13) {
      event.preventDefault();
      this.refs.login.focus();
      this.onLoginClick();
    }
  }
  onLoginClick(event) {
    if (event) event.preventDefault();
    this.showDimmer();
    var username = $(this.refs.username).val((i,v) => v || "test@test.test").val(); // FIXME: debug value
    var password = $(this.refs.password).val((i,v) => v || "test123").val(); // FIXME: debug value
    var login;
    var serverList;
    jQuery.ajax('https://cypherpunk.engineering/account/authenticate/userpasswd', {
      cache: false,
      contentType: 'application/json',
      data: JSON.stringify({ login: username, password: password }),
      dataType: 'json',
      method: 'POST',
      xhrFields: { withCredentials: true },
    }).then((data, status, xhr) => {
      login = data;
      return jQuery.ajax('https://cypherpunk.engineering/api/vpn/serverList', {
        contentType: 'application/json',
        dataType: 'json',
        xhrFields: { withCredentials: true },
      });
    }).catch((xhr, status, err) => {
      throw new Error("Login failed with status code " + xhr.status);
    }).then((data, status, xhr) => {
      serverList = data;
      return daemon.call.setAccount({
        username: username,
        secret: login.secret,
        name: "Cypher",
        email: login.acct.email,
        plan: login.acct.powerLevel, // FIXME
      });
    }).then(() => {
      this.hideDimmer();
      History.push('/connect');
    }).catch(err => {
      alert(err.message || "Failed to log in"); // FIXME: Don't use alert
      this.hideDimmer();
      this.refs.username.focus();
      console.dir(err);
    });
  }
  onLoginCancel() {
    this.hideDimmer();
  }
  showDimmer() {
    $(this.refs.dimmer).dimmer('show');
    this.refs.dimmer.showModal();
    $(this.refs.dimmer).find('*:focus').blur();
  }
  hideDimmer() {
    $(this.refs.dimmer).dimmer('hide');
    this.refs.dimmer.close();
  }
  render() {
    return (
      <div className="cp blurring" id="login-screen" ref="root">
        <dialog class="ui dimmer" ref="dimmer">
          <div class="ui big text loader">Logging in</div>
          <a tabIndex="0" onClick={this.onLoginCancel.bind(this)}>Cancel</a>
        </dialog>
        <img class="logo" src={CypherPunkLogo}/>
        <h3 class="ui title header"><span>Cypherpunk</span>Privacy</h3>
        <form class="ui form">
          <input placeholder="Username / Email" required autoFocus="true" onChange={this.onUsernameKeyPress.bind(this)} ref="username" />
          <input placeholder="Password" type="password" required onChange={this.onPasswordKeyPress.bind(this)} ref="password" />
          <a class="forgot" tabIndex="0">Forgot password?</a>
          <button class="login button" onClick={this.onLoginClick.bind(this)} ref="login"><i class="sign in icon"></i>Log in</button>
          <div class="ui horizontal divider">OR</div>
          <a class="signup button" tabIndex="0"><i class="write icon"></i>Sign up</a>
        </form>
      </div>
    );
  }
}

class AccountScreen extends React.Component  {
  render() {
    return(
      <div className="full screen" style={{visibility: 'visible'}}>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Account</div>
        </div>
        <div className="ui container">
          <div>display username</div>
          <div>display current plan</div>
          <div>upgrade account</div>
          <div>change password</div>
          <div>change email</div>
          <div>help</div>
          <Link className="clicky" to="/">Logout</Link>
        </div>
      </div>
    );
  }
}

class RootContainer extends React.Component {
  render() {
    return(
      <div class="full screen" style={{visibility: 'visible'}}>
        <MainBackground/>
        <OneZeros/>
        <ConnectScreen/>
      </div>
    );
  }
}

class LoadingPlaceholder extends React.Component {
  render() { return <div/>; }
}

class OneZeros extends React.Component {
  render() {
    return(
      <div id="onezeros">
      0100101010101001010011010000101100001011101111010100101101001101101011110010101001100011011110010111000001101000011001010111001001110000011101010110111001101011001000000111011001110000011011100100101010101001010011010000101100001011101111010100101101001010111101110010011100000111101111010100101101000010000001110110011000110111001001010101010010100110100001011000010111011110101001011010010101111011
      </div>
    );
  }
}

class CypherPunkApp extends React.Component {
  render() {
    return(
      <Router history={History}>
        <Route path="/connect" component={RootContainer}></Route>
        <Route path="/account" component={AccountScreen}></Route>
        <Route path="/status" component={StatusScreen}></Route>
        <Route path="/configuration" component={ConfigurationScreen}></Route>
        <Route path="/firewall" component={FirewallScreen}></Route>
        <Route path="/encryption" component={EncryptionScreen}></Route>
        <Route path="/password" component={PasswordScreen}></Route>
        <Route path="/email" component={EmailScreen}></Route>
        <Route path="/help" component={HelpScreen}></Route>
        <Route path="/login" component={LoginScreen}></Route>
        <Route path="*" component={LoadingPlaceholder}></Route>
      </Router>
    );
  }
}

ReactDOM.render(<CypherPunkApp />, document.getElementById('root-container'));
