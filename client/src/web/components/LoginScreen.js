import React from 'react';
import { Link } from 'react-router';
import { Dragbar } from './Titlebar.js';
import { Title } from './Titlebar.js';
import { TransitionGroup, Transition, RouteTransition, ReactCSSTransitionGroup } from './Transition';
import RetinaImage from './Image.js';
import daemon from '../daemon.js';
import server from '../server.js';
import { DEFAULT_REGION_DATA, classList } from '../util.js';
const { session } = require('electron').remote;


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



const PageTitle = ({ key = "title", top = 230, className = null, ...props } = {}) => <h3 key={key} className={classList('cp title', className)} style={{ top: top + 'px' }}{...props}><div className="welcome">Welcome to</div><img className="logo" src={require('../assets/img/logo_text.svg')} alt="Cypherpunk Privacy"/></h3>;
const PageBackground = ({ key, src, top = 0, ...props } = {}) => <RetinaImage key={key} className="background" src={src} style={{ marginTop: top + 'px' }} {...props}/>;
const BackLink = ({ key = "back", to, text = "Back", icon = 'undo', ...props } = {}) => <Link key={key} className="back link" to={to} tabIndex="0" {...props}><i className={classList("icon", icon)}/>{text}</Link>;

const LoginImage = require('../assets/img/login_illustration.png');
const LoginImage2x = require('../assets/img/login_illustration@2x.png');

const DefaultPageTitle = PageTitle();
const DefaultPageBackground = PageBackground({ key: "bg-default", top: 36, src: { 1: LoginImage, 2: LoginImage2x } });

const TitleHeight = 30;

/*
const Headers = {
  'default': { src: { 1: LoginImage, 2: LoginImage2x }, height: 275 },
};
*/

function pageValue(pageType, member) {
  return pageType.hasOwnProperty(member) ? pageType[member] : Page[member];
}

class Page extends React.Component {
  static elements = [ DefaultPageBackground ];
  static defaultProps = {
    top: 240,
  };
  render() {
    return (
      <form className={classList("cp ui form", this.props.className)} style={{ top: this.props.top + 'px' }}>
        {this.props.children}
      </form>
    );
  }
}



export class Check extends Page {
  static elements = [ DefaultPageTitle ];
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
      <Page className="login-check" top={310}>
        <div className="ui inline active massive text loader" ref="loader"></div>
      </Page>
    );
  }
}

export class Logout extends Page {
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
      <Page className="login-check">
        <div className="ui inline active massive text loader" ref="loader"></div>
      </Page>
    );
  }
}

export class EmailStep extends Page {
  static elements = [ DefaultPageBackground, DefaultPageTitle ];
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
      <Page className="login-email" top={310}>
        {/*<div className="welcome">Welcome.</div>*/}
        <div className="desc">Please input your email to begin.</div>
        <div className="ui icon input">
          <input type="text" placeholder="Email" required autoFocus="true" ref="email" defaultValue={daemon.account.account ? daemon.account.account.email : ""} onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="search chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
      </Page>
    );
  }
}

export class PasswordStep extends Page {
  static elements = [ DefaultPageBackground, BackLink({ key: 'back-from-password', to: '/login/email' }) ];
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
      <Page className="login-password">
        <div className="name">{this.props.location.query.email}</div>
        <div className="desc">Welcome back!</div>
        <div className="ui icon input">
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
        <a className="forgot link" tabIndex="0">Forgot password?</a>
        {/*<Link className="back link" to="/login/email" tabIndex="0"><i className="undo icon"></i>Back</Link>*/}
      </Page>
    );
  }
}

export class RegisterStep extends Page {
  static elements = [ DefaultPageBackground, BackLink({ key: 'back-from-register', to: '/login/email' }) ];
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
      <Page className="login-register">
        <div className="desc">Registering new account for {this.props.location.query.email}...</div>
        <div className="ui icon input">
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
        </div>
        <a class="cp yellow button" ref="register" onClick={() => this.onSubmit()}>Register</a>
        <div className="ui inline text loader" ref="loader"></div>
        {/*<Link className="back link" to="/login/email" tabIndex="0"><i className="undo icon"></i>Back</Link>*/}
      </Page>
    );
  }
}

export class ConfirmationStep extends Page {
  static elements = [ DefaultPageBackground, BackLink({ key: 'back-from-confirm', to: '/login/email' }) ];
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
  onResend() {

  }
  componentDidMount() {
    this.beginPolling();
  }
  componentWillUnmount() {
    this.endPolling();
  }
  render() {
    return (
      this.state.timedOut
        ?
        <Page className="login-confirm">
          <div className="desc">
            We haven't received your email confirmation yet.
          </div>
          <a className="button" tabIndex="0" onClick={() => this.onRetry()}><i className="refresh icon"/> Check again</a>
          <a className="button" tabIndex="0" onClick={() => this.onResend()}><i className="mail outline icon"/> Resend email</a>
        </Page>
        :
        <Page className="login-confirm">
          <div className="desc">Awaiting email confirmation...</div>
          <div className="ui inline active large text loader" ref="loader"></div>
        </Page>
    );
  }
}


export class AnalyticsStep extends Page {
  static elements = [ DefaultPageBackground ];
  onAllow() {
    console.log("Allow");
  }
  onDisallow() {
    console.log("Don't allow");
  }
  render() {
    return (
      <Page className="login-analytics">
        <div className="desc">
          <h3>One Final Step</h3>
          Share anonymized analytics about your VPN connections to help make Cypherpunk Privacy faster and more reliable.
        </div>
        <div className="ui inline active large text loader">Placeholder</div>
        <button className="allow" onClick={() => this.onAllow()}>Allow</button>
        <a className="disallow" onClick={() => this.onDisallow()}>Don't allow</a>
      </Page>
    );
  }
}


const LOGIN_ORDER = [ 'email', 'password', 'register', 'confirm', 'analytics' ];


export default class LoginScreen extends React.Component {
  static getTransition(diff) {
    if (diff['check'] === 'enter' || diff['check'] === 'leave') {
      // If going to or from the check screen, always fade everything
      return 'fadeIn';
    }
    let result = {
      '*': 'fadeIn',
    };
    let from, to;
    LOGIN_ORDER.forEach((key, index) => {
      if (diff[key] === 'leave') from = index;
      else if (diff[key] === 'enter') to = index;
    });
    if (from !== undefined && to !== undefined) {
      let transition = from < to ? 'nudgeLeft' : 'nudgeRight';
      result[LOGIN_ORDER[from]] = transition;
      result[LOGIN_ORDER[to]] = transition;
    }
    return result;
  }
  render() {
    let pageType = React.Children.only(this.props.children).type;
    let pageElements = pageValue(pageType, 'elements');
    return (
      <TransitionGroup transition={LoginScreen.getTransition} component="div" className="cp blurring full screen" id="login-screen">
        {pageElements}
        {this.props.children}
        <div key="version" id="version">{"v"+require('electron').remote.app.getVersion()}</div>
      </TransitionGroup>
    );
  }
}
