import React from 'react';
import { hashHistory } from 'react-router';
import CypherPunkLogo from '../assets/img/logomark.svg';
import Dragbar from './Dragbar.js';
import { Title } from './Titlebar.js';
import RouteTransition from './Transition';
import daemon from '../daemon.js';



function postLoginActions(email, password, account) {
  return jQuery.ajax('https://cypherpunk.engineering/api/vpn/serverList', {
    contentType: 'application/json',
    dataType: 'json',
    xhrFields: { withCredentials: true },
  }).catch((xhr, status, err) => {
    throw new Error("Login failed with status code " + xhr.status);
  }).then((data, status, xhr) => {
    var servers = Array.toDict(Array.flatten(Array.flatten(Object.mapToArray(data, (r, countries) => Object.mapToArray(countries, (c, locations) => locations.map(l => Object.assign({}, l, { country: c, region: r })))))), s => s.id);
    var regions = Object.mapValues(Array.toMultiDict(Object.values(servers), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
    return daemon.call.setAccount({
      username: email,
      secret: account.secret,
      name: "Cypher",
      email: account.acct.email,
      plan: account.acct.powerLevel, // FIXME
    }).then(() => {
      return daemon.call.applySettings({
        regions: regions,
        servers: servers,
      });
    });
  });
}


export class EmailStep extends React.Component {
  constructor(props) {
    super(props);
  }
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
  constructor(props) {
    super(props);
  }
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
    }).then(() => {
      History.push('/connect');
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
        <a class="forgot" tabIndex="0">Forgot password?</a>
      </form>
    );
  }
}

export class RegisterStep extends React.Component {
  constructor(props) {
    super(props);
  }
  onSubmit() {
    var password = this.refs.password.value;
    if (this.refs.password2.value !== password) {
      alert("Password mismatch"); // FIXME
      return;
    }
    $([this.refs.password, this.refs.password2]).prop('disabled', true).parent().addClass('loading')
    $(this.refs.password).prop('disabled', true).parent().addClass('loading');
    var account;
    jQuery.ajax('https://cypherpunk.engineering/account/register/signup', {
      cache: false,
      contentType: 'application/json',
      data: JSON.stringify({ email: this.props.location.query.email, password: password }),
      dataType: 'json',
      method: 'POST',
      xhrFields: { withCredentials: true },
    }).catch((xhr, status, err) => {
      throw new Error("Login failed with status code " + xhr.status);
    }).then((data, status, xhr) => {
      return postLoginActions(this.props.location.query.email, password, data);
    }).then(() => {
      History.push('/connect');
    }).catch(err => {
      alert(err.message);
      $(this.refs.password).prop('disabled', false).focus().select().parent().removeClass('loading');
      this.refs.password2.disabled = false;
    });
  }
  render() {
    return(
      <form class="cp login-register ui form">
        <div className="desc">Registering new account for {this.props.location.query.email}...</div>
        <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.refs.password2.focus(); e.preventDefault(); } }} />
        <input type="password" placeholder="Password (again)" required ref="password2" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
        <button class="cp yellow button" onClick={() => this.onSubmit()}>Register</button>
      </form>
    );
  }
}




export default class LoginScreen extends React.Component {
  constructor(props) {
    super(props);
  }

  /*
  componentDidMount() {
    $(this.refs.dimmer).dimmer({ closable: false });
    this.refs.dimmer.addEventListener('cancel', this.onLoginCancel.bind(this));
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
  */
  render() {
    return (
      <div className="cp blurring full screen" id="login-screen" ref="root">
        <Dragbar height="225px"/>
        {/*<dialog class="ui dimmer" ref="dimmer">
          <div class="ui big text loader">Logging in</div>
          <a tabIndex="0" onClick={this.onLoginCancel.bind(this)}>Cancel</a>
        </dialog>*/}
        <img class="logo" src={CypherPunkLogo}/>
        <Title component="h3" />
        <RouteTransition transition="nudgeLeft">
          {this.props.children}
        </RouteTransition>
      </div>
    );
  }
}
