window.$ = window.jQuery = require('jquery');

import './assets/css/app.less';
import 'semantic';

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, hashHistory as History } from 'react-router';

import SpinningImage from './assets/img/bgring3.png';
import CypherPunkLogo from './assets/img/cp_logo1.png';

import daemon from './daemon.js';

import StatusScreen from './StatusScreen.jsx';
import ConfigurationScreen from './ConfigurationScreen.jsx';
import FirewallScreen from './ConfigurationScreen/FirewallScreen.jsx';
import EncryptionScreen from './ConfigurationScreen/EncryptionScreen.jsx';

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
      <div id="titlebar" className="ui three item fixed borderless icon menu">
        <Link className="item" to="/status"><i className="info circle inverted icon"></i></Link>
        <div className="header item">Cypherpunk</div>
        <Link className="item" to="/configuration"><i className="setting icon"></i></Link>
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
    this.handleDaemonStateChange = this.handleDaemonStateChange.bind(this);
  }

  state = {
    regions: [
      { remote: "208.111.52.1 7133", country: "jp", name: "Tokyo 1, Japan" },
      { remote: "208.111.52.2 7133", country: "jp", name: "Tokyo 2, Japan" },
      { remote: "199.68.252.203 7133", country: "us", name: "Honolulu, HI, USA" },
    ],
    selectedRegion: "208.111.52.1 7133",
    receivedBytes: 0,
    sentBytes: 0,
    connectionState: "disconnected",
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
  handleDaemonStateChange(state) {
    this.setState(this.translateDaemonState(state));
  }
  componentDidMount() {
    daemon.on('state', this.handleDaemonStateChange);
    $(this.refs.regionDropdown).dropdown({
      onChange: this.handleRegionSelect
    });
  }
  componentWillUnmount() {
    daemon.removeListener('state', this.handleDaemonStateChange);
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
          <i id="connect" class={"ui fitted massive power link icon" + (this.state.connectionState === 'connected' ? " green" : this.state.connectionState == 'disconnected' ? " red" : " orange disabled")} ref="connectButton" onClick={this.handleConnectClick}></i>
        </div>
        <div id="connect-status" ref="connectStatus">{buttonLabel}</div>
        <div id="region-select" class={"ui selection dropdown" + (this.state.connectionState === 'disconnected' ? "" : " disabled")} ref="regionDropdown">
          <input type="hidden" name="region" value={this.state.selectedRegion}/>
          <i class="dropdown icon"></i>
          <div class="default text">Select Region</div>
          <div class="menu">
            { this.state.regions.map(r => <div class="item" data-value={r.remote} key={r.remote}><i class={r.country + " flag"}></i>{r.name}</div>) }
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
        let [ ip, port ] = this.getSelectedRegion().split(' ');
        daemon.post.applySettings({
          protocol: "udp",
          remoteIP: ip,
          remotePort: parseInt(port, 10),
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
  handleRegionSelect(remote) {
  }
}

class LoginScreen extends React.Component {
  render() {
    return (
      <div className="ui container cp">
        <h1 className="ui center aligned header"><img className="logo" src={CypherPunkLogo}/></h1>
        <form className="login_screen">
        <div>
          <input placeholder="Username/Email" />
        </div>
        <div>
          <input placeholder="Password" />
        </div>
        <Link className="login" to="/connect">Log in</Link>
        <div className="forgot">Forgot password?</div>
        <div className="signup">Sign Up</div>
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
        <ConnectScreen/>
      </div>
    );
  }
}

class LoadingPlaceholder extends React.Component {
  render() { return <div/>; }
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
        <Route path="/login" component={LoginScreen}></Route>
        <Route path="*" component={LoadingPlaceholder}></Route>
      </Router>
    );
  }
}

ReactDOM.render(<CypherPunkApp />, document.getElementById('root-container'));
