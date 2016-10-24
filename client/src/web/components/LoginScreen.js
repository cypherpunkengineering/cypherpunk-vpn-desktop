import React from 'react';
import { Link } from 'react-router';
import CypherPunkLogo from '../assets/img/logomark.svg';
import Dragbar from './Dragbar.js';
import { Title } from './Titlebar.js';
import RouteTransition from './Transition';
import daemon from '../daemon.js';



function checkSubscriptionStatus() {
  var subscription;
  return jQuery.ajax('https://cypherpunk.engineering/api/subscription/status', {
    contentType: 'application/json',
    dataType: 'json',
    xhrFields: { withCredentials: true },
  }).catch((xhr, status, err) => {
    throw new Error("Unable to check subscription status: " + xhr.status);
  }).then((data, status, xhr) => {
    subscription = data;
    return daemon.call.applySettings({
      subscription: subscription,
    });
  }).then(() => {
    return subscription.confirmed;
  });
}

function readServerList() {
  var servers, regions;
  return jQuery.ajax('https://cypherpunk.engineering/api/vpn/serverList', {
    contentType: 'application/json',
    dataType: 'json',
    xhrFields: { withCredentials: true },
  }).catch((xhr, status, err) => {
    throw new Error("Unable to read server list: " + xhr.status);
  }).then((data, status, xhr) => {
    servers = Array.toDict(Array.flatten(Array.flatten(Object.mapToArray(data, (r, countries) => Object.mapToArray(countries, (c, locations) => locations.map(l => Object.assign({}, l, { country: c, region: r })))))), s => s.id);
    regions = Object.mapValues(Array.toMultiDict(Object.values(servers), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
    return daemon.call.applySettings({
      regions: regions,
      servers: servers,
    });
  });
}

function postLoginActions(email, password, account) {
  return daemon.call.setAccount({
    username: email,
    secret: account.secret,
    name: "Cypher",
    email: account.acct.email,
    plan: account.acct.powerLevel, // FIXME
  }).then(() => {
    return checkSubscriptionStatus();
  }).then(confirmed => {
    if (confirmed) {
      readServerList().then(() => {
        History.push('/connect');
      });
    } else {
      History.push({ pathname: '/login/confirm', query: { email: email } });
    }
  });
}


export class EmailStep extends React.Component {
  onSubmit() {
    var email = this.refs.email.value || (this.refs.email.value = "test@test.test"); // FIXME: debug value
    $(this.refs.email).prop('disabled', true).parent().addClass('loading');
    jQuery.ajax('https://cypherpunk.engineering/account/identify/email', {
      cache: false,
      contentType: 'application/json',
      data: JSON.stringify({ email: email }),
      //dataType: 'json',
      method: 'POST',
      xhrFields: { withCredentials: true },
    }).then((data, status, xhr) => {
      History.push({ pathname: '/login/password', query: { email: email } });
    }).catch((xhr, status, err) => {
      if (xhr.status == 401) {
        History.push({ pathname: '/login/register', query: { email: email } });
      } else {
        alert("Login failed with status code " + xhr.status);
        $(this.refs.email).prop('disabled', false).focus().select().parent().removeClass('loading');
        throw new Error("Login failed with status code " + xhr.status);
      }
    });
  }
  render() {
    return(
      <form className="cp login-email ui form">
        {/*<div className="welcome">Welcome.</div>*/}
        <div className="desc">Please input your email to begin.</div>
        <div className="ui icon input">
          <input type="text" placeholder="Email" required autoFocus="true" ref="email" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
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
    jQuery.ajax('https://cypherpunk.engineering/account/authenticate/password', {
      cache: false,
      contentType: 'application/json',
      data: JSON.stringify({ password: password }), // TODO: email not needed?
      dataType: 'json',
      method: 'POST',
      xhrFields: { withCredentials: true },
    }).catch((xhr, status, err) => {
      throw new Error("Login failed with status code " + xhr.status);
    }).then((data, status, xhr) => {
      return postLoginActions(this.props.location.query.email, password, data);
    }).catch(err => {
      alert(err.message);
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
    if (this.refs.password2.value !== password) {
      alert("Password mismatch"); // FIXME
      return;
    }
    $([this.refs.password, this.refs.password2]).prop('disabled', true);
    $(this.refs.register).hide();
    $(this.refs.loader).addClass('active');
    var account;
    jQuery.ajax('https://cypherpunk.engineering/account/register/signup', {
      cache: false,
      contentType: 'application/json',
      data: JSON.stringify({ email: this.props.location.query.email, password: password }),
      //dataType: 'json',
      method: 'POST',
      xhrFields: { withCredentials: true },
    }).catch((xhr, status, err) => {
      throw new Error("Login failed with status code " + xhr.status);
    }).then((data, status, xhr) => {
      return postLoginActions(this.props.location.query.email, password, data);
    }).catch(err => {
      $([this.refs.password, this.refs.password2]).prop('disabled', false);
      $(this.refs.register).show();
      $(this.refs.loader).removeClass('active');
    });
  }
  render() {
    return(
      <form className="cp login-register ui form">
        <div className="desc">Registering new account for {this.props.location.query.email}...</div>
        <div className="ui icon input group">
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.refs.password2.focus(); e.preventDefault(); } }} />
        </div>
        <div className="ui icon input">
          <input type="password" placeholder="Password (again)" required ref="password2" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
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
    checkSubscriptionStatus().then(confirmed => {
      if (confirmed) {
        History.push('/connect');
      }
    })
  }
  componentDidMount() {
    this.setState({ interval: setInterval(this.onTimer.bind(this), 2000) });
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
  getTransition(from,to) {
    console.log(from, to);
    return (LOGIN_ORDER.indexOf(to) < LOGIN_ORDER.indexOf(from)) ? 'nudgeRight' : 'nudgeLeft';
  }
  render() {
    return (
      <div className="cp blurring full screen" id="login-screen" ref="root">
        <Dragbar height="225px"/>
        <img class="logo" src={CypherPunkLogo}/>
        <Title component="h3" />
        <RouteTransition transition={this.getTransition}>
          {this.props.children}
        </RouteTransition>
      </div>
    );
  }
}
