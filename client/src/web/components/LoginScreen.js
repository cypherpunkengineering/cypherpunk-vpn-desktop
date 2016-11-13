import React from 'react';
import { Link } from 'react-router';
import CypherPunkLogo from '../assets/img/logomark.svg';
import { Dragbar } from './Titlebar.js';
import { Title } from './Titlebar.js';
import RouteTransition from './Transition';
import daemon from '../daemon.js';
import server from '../server.js';
const { session } = require('electron').remote;



function refreshSubscription() {
  return server.get('/api/v0/subscription/status').then(response => daemon.call.applySettings({ subscription: response.data }).then(() => response.data));
}

function refreshServerList() {
  return server.get('/api/v0/vpn/serverList').then(response => {
    var servers = Array.toDict(Array.flatten(Array.flatten(Object.mapToArray(response.data, (r, countries) => Object.mapToArray(countries, (c, locations) => locations.map(l => Object.assign({}, l, { country: c, region: r })))))), s => s.id);
    var regions = Object.mapValues(Array.toMultiDict(Object.values(servers), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
    return daemon.call.applySettings({ regions: regions, servers: servers }).then(() => servers);
  })
}

// Called to refresh any updated data and take any required next steps for
// the current account (such as confirming your email address). 
function refreshAccount() {
  return refreshSubscription().then(subscription => {
    if (!subscription.confirmed) {
      History.push({ pathname: '/login/confirm', query: { email: daemon.account.email } });
    } else {
      return refreshServerList().then(servers => {
        History.push('/connect');
      });
    }
  });
}

// Called when we have logged in to (or registered) a new account, to store
// and refresh all relevant account data.
function setAccount(loginData) {
  console.dir(loginData);
  return daemon.call.setAccount({
    email: loginData.account.email,
    token: loginData.token,
    secret: loginData.secret, // FIXME: deprecated
  }).then(() => refreshAccount());
}



export class Check extends React.Component {
  componentDidMount() {
    // Not the nicest pattern, but check if we're logged in here. A successful
    // refresh automatically takes us to the main screen, whereas an authentication
    // error will take us to the login screen (with err.handled = true). Other errors
    // are unexpected and we show an alert for those.
    setTimeout(() => { // need to use setTimeout since we might modify History
      refreshAccount().catch(err => {
        console.warn(err);
        if (!err.handled) {
          alert(err.message);
          History.push('/login/email');
        }
      });
    }, 0);
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
      server.post('/api/v0/account/logout', null, { refreshSessionOnForbidden: false, catchAuthFailure: false })
        .catch(err => console.error("Error while logging out:", err))
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
    var email = this.refs.email.value || (this.refs.email.value = "test@test.test"); // FIXME: debug value
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
          <input type="text" placeholder="Email" required autoFocus="true" ref="email" defaultValue={daemon.account.email} onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="search chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
      </form>
    );
  }
}

export class PasswordStep extends React.Component {
  onSubmit() {
    var password = this.refs.password.value || (this.refs.password.value = "test123"); // FIXME: debug value
    $(this.refs.password).prop('disabled', true).parent().addClass('loading');
    server.post('/api/v0/account/authenticate/password', { email: this.props.location.query.email, password: password }).then(response => {
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
      </div>
    );
  }
}
