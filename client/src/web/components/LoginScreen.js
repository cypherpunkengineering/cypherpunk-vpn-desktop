import React from 'react';
import { Link } from 'react-router';
import CypherPunkLogo from '../assets/img/logomark.svg';
import { Dragbar } from './Titlebar.js';
import { Title } from './Titlebar.js';
import RouteTransition from './Transition';
import daemon from '../daemon.js';
import server from '../server.js';
const { session } = require('electron').remote;



function refreshLocationList() {
  return server.get('/api/v0/location/list/' + daemon.account.account.type).then(response => {
    var locations = response.data;
    Object.values(locations).forEach(l => {
      if (!l.authorized || !['ovDefault', 'ovNone', 'ovStrong', 'ovStealth'].every(t => Array.isArray(l[t]) && l[t].length > 0)) {
        l.disabled = true;
      }
    });
    var regions = Object.mapValues(Array.toMultiDict(Object.values(locations), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
    var result = daemon.call.applySettings({ regions: regions, locations: locations });
    // Workaround: ensure region selection is not empty
    if (!locations[daemon.settings.location] || locations[daemon.settings.location].disabled) {
      for (let l of Object.values(locations)) {
        if (!l.disabled) {
          result = result.then(() => daemon.call.applySettings({ location: l.id }));
          break;
        }
      }
    }
    return result.then(() => locations);
  });
}

// Refresh the current account by querying the server
function refreshAccount() {
  return server.get('/api/v0/account/status').then(response => setAccount(response.data));
}

// Updates the application state after any new account data has been received
function setAccount(data) {
  return daemon.call.setAccount(data).then(() => {
    if (false && data.account.type !== 'developer') {
      // switch off non-developer settings
      daemon.post.applySettings({ routeDefault: true });
    }
    if (!data.account.confirmed) {
      History.push({ pathname: '/login/confirm', query: { email: daemon.account.account.email } });
    } else {
      return refreshLocationList().then(locations => {
        // TODO: Move to Application.onLoginSessionEstablished()
        if (History.getCurrentLocation().pathname.startsWith('/login')) {
          History.push('/connect');
        }
        if (daemon.settings.autoConnect) {
          daemon.post.connect();
        }
      });
    }
  });
}



export class Check extends React.Component {
  static run() {
    // Use setTimeout to avoid changing history in the same callstack
    setTimeout(() => {
      if (daemon.account.account && daemon.account.account.confirmed && daemon.account.privacy && daemon.config.locations) {
        // Go straight to main screen and run the check in the background; if it
        // fails, we'll go back to the login screen.
        History.push('/connect');
        refreshAccount().catch(err => {
          // ignore errors; 403 will still take us back to the login screen
        });
      } else {
        // Need to login and/or fetch necessary data to continue
        refreshAccount().catch(err => {
          console.warn(err);
          if (!err.handled) {
            alert(err.message);
            History.push('/login/email');
          }
        });
      }
    }, 0);
  }
  componentDidMount() {
    // Not the nicest pattern, but check if we're logged in here. A successful
    // refresh automatically takes us to the main screen, whereas an authentication
    // error will take us to the login screen (with err.handled = true). Other errors
    // are unexpected and we show an alert for those.
    Check.run();
  }
  render() {
    return (
      <form className="cp login-check ui form">
        <div className="ui inline active massive text loader" ref="loader"></div>
      </form>
    );
  }
}

export class Logout extends React.Component {
  componentDidMount() {
    setTimeout(() => { // need to use setTimeout since we might modify History
      daemon.post.disconnect(); // make sure we don't stay connected
      server.post('/api/v0/account/logout', null, { refreshSessionOnForbidden: false, catchAuthFailure: false })
        .catch(err => console.error("Error while logging out:", err))
        .then(() => daemon.call.setAccount({ account: { email: daemon.account.account.email } }))
        .then(() => { session.defaultSession.clearStorageData({ storages: [ 'cookies' ] }, () => History.push('/login/email')); });
    })
  }
  render() {
    return (
      <form className="cp login-check ui form">
        <div className="ui inline active massive text loader" ref="loader"></div>
      </form>
    );
  }
}

export class EmailStep extends React.Component {
  onSubmit() {
    var email = this.refs.email.value;
    $(this.refs.email).prop('disabled', true).parent().addClass('loading');
    server.post('/api/v0/account/identify/email', { email: email }).then({
      200: response => History.push({ pathname: '/login/password', query: { email: email }}),
      401: response => History.push({ pathname: '/login/register', query: { email: email }})
    }).catch(err => {
      if (!err.handled) {
        alert(err.message);
        $(this.refs.email).prop('disabled', false).focus().select().parent().removeClass('loading');
      }
    });
  }
  render() {
    return(
      <form className="cp login-email ui form">
        {/*<div className="welcome">Welcome.</div>*/}
        <div className="desc">Please input your email to begin.</div>
        <div className="ui icon input">
          <input type="text" placeholder="Email" required autoFocus="true" ref="email" defaultValue={daemon.account.account ? daemon.account.account.email : ""} onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="search chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
      </form>
    );
  }
}

export class PasswordStep extends React.Component {
  onSubmit() {
    var password = this.refs.password.value;
    $(this.refs.password).prop('disabled', true).parent().addClass('loading');
    server.post('/api/v0/account/authenticate/password', { /*email: this.props.location.query.email,*/ password: password }).then(response => {
      return setAccount(response.data);
    }).catch(err => {
      if (!err.handled) {
        alert(err.message);
        console.dir(err);
      }
      $(this.refs.password).prop('disabled', false).focus().select().parent().removeClass('loading');
    });
  }
  render() {
    return(
      <form className="cp login-password ui form">
        <div className="desc">Logging in to {this.props.location.query.email}...</div>
        <div className="ui icon input">
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
        <a className="forgot link" tabIndex="0">Forgot password?</a>
        <Link className="back link" to="/login/email" tabIndex="0"><i className="undo icon"></i>Back</Link>
      </form>
    );
  }
}

export class RegisterStep extends React.Component {
  onSubmit() {
    var password = this.refs.password.value;
    $(this.refs.password).prop('disabled', true);
    $(this.refs.register).hide();
    $(this.refs.loader).addClass('active');
    server.post('/api/v0/account/register/signup', { email: this.props.location.query.email, password: password }).then(response => {
      return setAccount(response.data);
    }).catch(err => {
      if (!err.handled) {
        alert(err.message);
      }
      $(this.refs.password).prop('disabled', false);
      $(this.refs.register).show();
      $(this.refs.loader).removeClass('active');
    });
  }
  render() {
    return(
      <form className="cp login-register ui form">
        <div className="desc">Registering new account for {this.props.location.query.email}...</div>
        <div className="ui icon input">
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
        </div>
        <a class="cp yellow button" ref="register" onClick={() => this.onSubmit()}>Register</a>
        <div className="ui inline text loader" ref="loader"></div>
        <Link className="back link" to="/login/email" tabIndex="0"><i className="undo icon"></i>Back</Link>
      </form>
    );
  }
}

export class ConfirmationStep extends React.Component {
  onTimer() {
    refreshAccount(); // navigates automatically
  }
  componentDidMount() {
    this.setState({ interval: setInterval(this.onTimer.bind(this), 5000) });
  }
  componentWillUnmount() {
    clearInterval(this.state.interval);
  }
  render() {
    return(
      <form className="cp login-confirm ui form">
        <div className="desc">Awaiting email confirmation...</div>
        <div className="ui inline active large text loader" ref="loader"></div>
        <Link className="back link" to="/login/email" tabIndex="0">Back</Link>
      </form>
    );
  }
}


const LOGIN_ORDER = [ 'email', 'password', 'register', 'confirm' ];

export default class LoginScreen extends React.Component {
  static getTransition(from,to) {
    if (from === 'check' || to === 'check') return 'fadeIn';
    return (LOGIN_ORDER.indexOf(to) < LOGIN_ORDER.indexOf(from)) ? 'nudgeRight' : 'nudgeLeft';
  }
  render() {
    return (
      <div className="cp blurring full screen" id="login-screen" ref="root">
        <Dragbar height="225px"/>
        <img class="logo" src={CypherPunkLogo}/>
        <Title component="h3" />
        <RouteTransition transition={LoginScreen.getTransition}>
          {this.props.children}
        </RouteTransition>
        <div id="version">{"v"+require('electron').remote.app.getVersion()}</div>
      </div>
    );
  }
}
