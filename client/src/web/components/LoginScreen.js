import React from 'react';
import { hashHistory } from 'react-router';
import CypherPunkLogo from '../assets/img/logomark.svg';
import Dragbar from './Dragbar.js';
import daemon from '../daemon.js';

export default class LoginScreen extends React.Component {
  constructor(props) {
    super(props);
  }

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
      var servers = Array.toDict(Array.flatten(Array.flatten(Object.mapToArray(data, (r, countries) => Object.mapToArray(countries, (c, locations) => locations.map(l => Object.assign({}, l, { country: c, region: r })))))), s => s.id);
      var regions = Object.mapValues(Array.toMultiDict(Object.values(servers), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
      return daemon.call.setAccount({
        username: username,
        secret: login.secret,
        name: "Cypher",
        email: login.acct.email,
        plan: login.acct.powerLevel, // FIXME
      }).then(() => {
        return daemon.call.applySettings({
          regions: regions,
          servers: servers,
        });
      });
    }).then(() => {
      this.hideDimmer();
      hashHistory.push('/connect');
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
        <Dragbar/>
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
