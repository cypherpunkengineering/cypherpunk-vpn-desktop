import React from 'react';
import { Link } from 'react-router';
import { Dragbar } from './Titlebar.js';
import { Title } from './Titlebar.js';
import { TransitionGroup, Transition, RouteTransition, ReactCSSTransitionGroup } from './Transition';
import RetinaImage from './Image.js';
import daemon from '../daemon.js';
import server from '../server.js';
import { DEFAULT_REGION_DATA } from '../util.js';
const { session } = require('electron').remote;

const LoginImage = require('../assets/img/login_illustration.png');
const LoginImage2x = require('../assets/img/login_illustration@2x.png');

function refreshRegionList() {
  var countryNames = daemon.config.countryNames || DEFAULT_REGION_DATA.countryNames;
  var regionNames = daemon.config.regionNames || Array.toDict(DEFAULT_REGION_DATA.regions, x => x[0], x => x[1]);
  var regionOrder = daemon.config.regionOrder || DEFAULT_REGION_DATA.regions.map(x => x[0]);
  return Promise.resolve().then(() => {
    if (countryNames !== daemon.config.countryNames || regionNames !== daemon.config.regionNames || regionOrder !== daemon.config.regionOrder) {
      return daemon.call.applySettings({ countryNames, regionNames, regionOrder });
    }
  }).then(() => {
    return server.get('/api/v0/location/world').then(response => {
      if (response.data.country) countryNames = Object.assign({}, countryNames, response.data.country);
      if (response.data.region) regionNames = Object.assign({}, regionNames, response.data.region);
      if (response.data.regionOrder) regionOrder = response.data.regionOrder;
    }, err => {});
  }).then(() => {
    if (countryNames !== daemon.config.countryNames || regionNames !== daemon.config.regionNames || regionOrder !== daemon.config.regionOrder) {
      return daemon.call.applySettings({ countryNames, regionNames, regionOrder });
    }
  }).then(() => { countryNames, regionNames, regionOrder });
}

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
    if (data.account.type !== 'developer') {
      // switch off non-developer settings
      daemon.post.applySettings({ /*routeDefault: true,*/ optimizeDNS: false });
    }
    if (!data.account.confirmed) {
      History.push({ pathname: '/login/confirm', query: { email: daemon.account.account.email } });
    } else {
      return Promise.all([ refreshRegionList(), refreshLocationList() ]).then(() => {
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
    // Use setImmediate to avoid changing history in the same callstack
    setImmediate(() => {
      if (daemon.account.account && daemon.account.account.confirmed && daemon.account.privacy && daemon.config.locations && daemon.config.regions && daemon.config.countryNames && daemon.config.regionNames && daemon.config.regionOrder) {
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
    });
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
      <form className="cp login-check ui form" style={{ marginTop: this.props.height + 'px' }}>
        <div className="ui inline active massive text loader" ref="loader"></div>
      </form>
    );
  }
}

export class Logout extends React.Component {
  componentDidMount() {
    setImmediate(() => { // need to use setImmediate since we might modify History
      daemon.post.disconnect(); // make sure we don't stay connected
      server.post('/api/v0/account/logout', null, { refreshSessionOnForbidden: false, catchAuthFailure: false })
        .catch(err => console.error("Error while logging out:", err))
        .then(() => daemon.call.setAccount({ account: { email: daemon.account.account.email } }))
        .then(() => { session.defaultSession.clearStorageData({ storages: [ 'cookies' ] }, () => History.push('/login/email')); });
    })
  }
  render() {
    return (
      <form className="cp login-check ui form" style={{ marginTop: this.props.height + 'px' }}>
        <div className="ui inline active massive text loader" ref="loader"></div>
      </form>
    );
  }
}

export class EmailStep extends React.Component {
  state = { check: false }
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
      <form className="cp login-email ui form" style={{ marginTop: this.props.height + 'px' }}>
        {/*<div className="welcome">Welcome.</div>*/}
        <div className="desc">Please input your email to begin.</div>
        <div className="ui icon input" onClick={() => this.setState({ check: !this.state.check })}>
          <input type="text" placeholder="Email" required autoFocus="true" ref="email" defaultValue={daemon.account.account ? daemon.account.account.email : ""} onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="search chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
        <ReactCSSTransitionGroup transitionName="nudgeLeft" transitionEnterTimeout={700} transitionLeaveTimeout={700}>
          <div key="a">
            {this.state.check ? <div key="c">CHECK</div> : null}
          </div>
        </ReactCSSTransitionGroup>
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
      <form className="cp login-password ui form" style={{ marginTop: this.props.height + 'px' }}>
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
      <form className="cp login-register ui form" style={{ marginTop: this.props.height + 'px' }}>
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
  state = {
    timedOut: false,
  }
  beginPolling() {
    this.attempts = 0;
    this.interval = setInterval(this.onTimer.bind(this), 5000);
  }
  endPolling() {
    if (this.interval) { clearInterval(this.interval); this.interval = null; }
  }
  onTimer() {
    if (this.attempts++ < 10)
      refreshAccount(); // navigates automatically
    else {
      this.endPolling();
      this.setState({ timedOut: true });
    }
  }
  onRetry() {
    this.beginPolling();
    this.setState({ timedOut: false });
  }
  componentDidMount() {
    this.beginPolling();
  }
  componentWillUnmount() {
    this.endPolling();
  }
  render() {
    if (this.state.timedOut) {
      return (
        <form className="cp login-confirm ui form" style={{ marginTop: this.props.height + 'px' }}>
          <div className="desc">
            We haven't received your email confirmation yet.<br/><br/>
            <a onClick={() => this.onRetry()}>Check again</a>
          </div>
          <Link className="back link" to="/login/email" tabIndex="0">Back</Link>
        </form>
      );
    } else {
      return(
        <form className="cp login-confirm ui form" style={{ marginTop: this.props.height + 'px' }}>
          <div className="desc">Awaiting email confirmation...</div>
          <div className="ui inline active large text loader" ref="loader"></div>
          <Link className="back link" to="/login/email" tabIndex="0">Back</Link>
        </form>
      );
    }
  }
}


export class AnalyticsStep extends React.Component {
  onAllow() {
    console.log("Allow");
  }
  onDisallow() {
    console.log("Don't allow");
  }
  render() {
    return (
      <form className="cp login-analytics ui form" style={{ marginTop: this.props.height + 'px' }}>
        <div className="desc">
          <h3>One Final Step</h3>
          Share anonymized analytics about your VPN connections to help make Cypherpunk Privacy faster and more reliable.
        </div>
        <div className="ui inline active large text loader">Placeholder</div>
        <button className="allow" onClick={() => this.onAllow()}>Allow</button>
        <a className="disallow" onClick={() => this.onDisallow()}>Don't allow</a>
      </form>
    );
  }
}


const LOGIN_ORDER = [ 'email', 'password', 'register', 'confirm', 'analytics' ];
const DEFAULT_HEADER_HEIGHT = 275;
const HEADER_IMAGES = {
  'check': { src: { 1: LoginImage, 2: LoginImage2x }, title: true, height: 275 },
  'email': { src: { 1: LoginImage, 2: LoginImage2x }, title: true, height: 275 },
  'analytics': null,
};


export default class LoginScreen extends React.Component {
  static getTransition(diff) {
    console.log("getTransition", diff);
    let from, to;
    Object.forEach(diff, (key, action) => {
      if (action === 'leave') from = key;
      else if (action === 'enter' || action === 'appear') to = key;
      else debugger;
    });
    if (!from || !to) return '';
    if (from === 'check' || to === 'check') return 'fadeIn';
    return (LOGIN_ORDER.indexOf(to) < LOGIN_ORDER.indexOf(from)) ? 'nudgeRight' : 'nudgeLeft';
  }
  render() {
    var headers = React.Children.map(this.props.children, child => HEADER_IMAGES[child.props.route.path]).filter(i => i);
    var title = headers.some(h => h.title) ? <Title key="header-title" component="h3"/> : null;
    var height = headers.reduce((a, b) => Math.max(a, b && b.height || 0), 40);
    return (
      <div className="cp blurring full screen" id="login-screen" ref="root">
        <Dragbar>
          <RetinaImage className="logo" src={{ 1: LoginImage, 2: LoginImage2x }} />
          {title}
        </Dragbar>
        {/*<ReactCSSTransitionGroup
          component={this.props.component || TransitionContainer}
          transitionName={transition}
          transitionEnter={!!transition}
          transitionLeave={!!transition}
          transitionAppear={!!transition}
          transitionEnterTimeout={transitionTime}
          transitionLeaveTimeout={transitionTime}
          transitionAppearTimeout={transitionTime}
          >
          {React.Children.map(children, c => c ? React.cloneElement(c, { key: this.getChildKey(c) }) : null)}
        </ReactCSSTransitionGroup>*/}
        <div style={{ height: '275px' }}/>
        <TransitionGroup transition={LoginScreen.getTransition}>
          {React.Children.map(this.props.children, child => {
            let header = HEADER_IMAGES[child.props.route.path];
            return React.cloneElement(child, { headerHeight: header && header.height || DEFAULT_HEADER_HEIGHT });
          })}
        </TransitionGroup>
        <div id="version">{"v"+require('electron').remote.app.getVersion()}</div>
      </div>
    );
  }
}
