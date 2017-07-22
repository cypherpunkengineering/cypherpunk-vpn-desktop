import React from 'react';
import { Link } from 'react-router';
import { Dragbar } from './Titlebar.js';
import { Title } from './Titlebar.js';
import { TransitionGroup, Transition, RouteTransition, ReactCSSTransitionGroup } from './Transition';
import RetinaImage from './Image.js';
import daemon from '../daemon.js';
import server from '../server.js';
import analytics from '../analytics.js';
import { DEFAULT_REGION_DATA, classList } from '../util.js';
import ExternalLink from './ExternalLink';
const { session } = require('electron').remote;


function nonEmpty(obj) {
  return (typeof obf === 'object' && obj && Object.keys(obj).length > 0) ? obj : null;
}

function refreshRegionList() {
  var countryNames = nonEmpty(daemon.config.countryNames) || DEFAULT_REGION_DATA.countryNames;
  var regionNames = nonEmpty(daemon.config.regionNames) || Array.toDict(DEFAULT_REGION_DATA.regions, x => x[0], x => x[1]);
  var regionOrder = nonEmpty(daemon.config.regionOrder) || DEFAULT_REGION_DATA.regions.map(x => x[0]);
  return Promise.resolve().then(() => {
    if (countryNames !== daemon.config.countryNames || regionNames !== daemon.config.regionNames || regionOrder !== daemon.config.regionOrder) {
      return daemon.call.applyConfig({ countryNames, regionNames, regionOrder });
    }
  }).then(() => {
    return server.get('/api/v1/location/world').then(response => {
      if (response.data.country) countryNames = Object.assign({}, countryNames, response.data.country);
      if (response.data.region) regionNames = Object.assign({}, regionNames, response.data.region);
      if (response.data.regionOrder) regionOrder = response.data.regionOrder;
    }, err => {});
  }).then(() => {
    if (countryNames !== daemon.config.countryNames || regionNames !== daemon.config.regionNames || regionOrder !== daemon.config.regionOrder) {
      return daemon.call.applyConfig({ countryNames, regionNames, regionOrder });
    }
  }).then(() => { countryNames, regionNames, regionOrder });
}

function refreshLocationList() {
  return server.get('/api/v1/location/list/' + daemon.account.account.type).then(response => {
    var locations = response.data;
    Object.values(locations).forEach(l => {
      if (!l.authorized || !['ovDefault', 'ovNone', 'ovStrong', 'ovStealth'].every(t => Array.isArray(l[t]) && l[t].length > 0)) {
        l.disabled = true;
      }
    });
    var regions = Object.mapValues(Array.toMultiDict(Object.values(locations), s => s.region), (r,c) => Object.mapValues(Array.toMultiDict(c, l => l.country), (c,l) => l.map(m => m.id)));
    var result = daemon.call.applyConfig({ regions: regions, locations: locations });
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
  return server.get('/api/v1/account/status').then(response => setAccount(response.data));
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
    } else if (data.account.type === 'invitation' || data.account.type === 'pending') { /*** PREVIEW ONLY ***/
      History.push({ pathname: '/login/pending' });
    } else {
      return Promise.all([ refreshRegionList(), refreshLocationList() ]).then(() => {
        // TODO: Move to Application.onLoginSessionEstablished()
        if (History.getCurrentLocation().pathname.startsWith('/login')) {
          History.push('/login/analytics');
        }
        if (daemon.settings.autoConnect) {
          daemon.post.connect();
        }
      });
    }
  });
}


const PIPE_UPPER_TEXT = 'x`8 0 # = v 7 mb" | y 9 # 8 M } _ + kl $ #mn x -( }e f l]> ! 03 @jno x~`.xl ty }[sx k j';
const PIPE_LOWER_TEXT = 'dsK 7 & [*h ^% u x 5 8 00 M< K! @ &6^d jkn 70 :93jx p0 bx, 890 Qw ;é " >?7 9 3@ { 5x3 >';

const PageHeader = ({ key = "header", top = 160, className = null, ...props } = {}) => <div key={key} className={classList('header', className)} style={{ top: top + 'px' }} {...props}></div>;
const PagePipe = ({ key = "pipe", top = 126, className = null, upper = PIPE_UPPER_TEXT, lower = PIPE_LOWER_TEXT, ...props } = {}) =>
  <div key={key} className={classList('pipe', className)} style={{ top: top + 'px' }} {...props}>
    <div style={{ animationDuration: (upper.length*300)+'ms' }}>{upper}{upper}</div>
    <div style={{ animationDuration: (lower.length*300)+'ms' }}>{lower}{lower}</div>
  </div>;
const PageBackground = ({ key, src, top = 0, right = null, ...props } = {}) => <RetinaImage key={key} className="background" src={src} style={{ top: top + 'px', right: typeof(right) === 'number' ? right + 'px' : null }} {...props}/>;
const BackLink = ({ key = "back", to, text = "Back", icon = 'left chevron', ...props } = {}) => <Link key={key} className="back link" to={to} tabIndex="0" {...props}><i className={classList("icon", icon)}/>{text}</Link>;

const DefaultPageBackground = PageBackground({ key: "bg-default", top: 30, right: 70, src: { [1]: require('../assets/img/login_illustration3.png'), [2]: require('../assets/img/login_illustration3@2x.png') } });
const DefaultPageElements = [ DefaultPageBackground, PagePipe(), PageHeader() ];

const WaitingPageBackground = PageBackground({ key: "bg-waiting", top: 43, right: 70, src: { [1]: require('../assets/img/login_illustration_waiting.png'), [2]: require('../assets/img/login_illustration_waiting@2x.png') } });

const AnalyticsPageBackground = PageBackground({ key: "bg-analytics", top: 60, src: { [1]: require('../assets/img/analytics.png'), [2]: require('../assets/img/analytics@2x.png') } });

// Goes into the page itself
const PageTitle = ({ key = "title", text = null, className = null, children, ...props } = {}) => <div key={key} className={classList('title', className)} {...props}>{text}{children}</div>;
const DefaultPageTitle = <PageTitle><div className="welcome">Welcome to</div><img className="logo" src={require('../assets/img/logo_text.svg')} alt="Cypherpunk Privacy"/></PageTitle>;



const TitleHeight = 30;

function pageValue(pageType, member) {
  return pageType.hasOwnProperty(member) ? pageType[member] : Page[member];
}

class Page extends React.Component {
  static elements = [ DefaultPageElements ];
  static defaultProps = {
    top: 235,
  };
  render() {
    return (
      <form className={classList("cp ui form", this.props.className)} style={{ top: this.props.top + 'px' }}>
        <div class="padding"/>
        {this.props.children}
        <div class="padding"/>
      </form>
    );
  }
}



export class Check extends Page {
  static elements = [ PagePipe(), PageHeader() ];
  static run() {
    // Use setImmediate to avoid changing history in the same callstack
    setImmediate(() => {
      if (daemon.account.account && daemon.account.account.confirmed && daemon.account.privacy && daemon.config.locations && daemon.config.regions && daemon.config.countryNames && daemon.config.regionNames && daemon.config.regionOrder) {
        // Go straight to main screen and run the check in the background; if it
        // fails, we'll go back to the login screen.
        History.push('/main');
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
      <Page className="login-check">
        <PageTitle><div className="loading">Loading...</div></PageTitle>
        <div className="ui inline active massive text loader" ref="loader"></div>
      </Page>
    );
  }
}

export class Logout extends Page {
  static elements = [];
  componentDidMount() {
    setImmediate(() => { // need to use setImmediate since we might modify History
      server.post('/api/v1/account/logout', null, { refreshSessionOnForbidden: false, catchAuthFailure: false })
        .catch(err => console.error("Error while logging out:", err))
        .then(() => daemon.post.disconnect())
        .then(() => daemon.call.setAccount({ account: { email: daemon.account.account && daemon.account.account.email || '' } }))
        .then(() => daemon.call.applySettings({ enableAnalytics: false }))
        .then(() => analytics.deactivate())
        .then(() => { session.defaultSession.clearStorageData({ storages: [ 'cookies' ] }, () => History.push('/login/email')); });
    })
  }
  render() {
    return (
      <Page className="login-logout">
        <div className="ui inline active massive text loader" ref="loader"></div>
      </Page>
    );
  }
}

export class EmailStep extends Page {
  static elements = [ DefaultPageElements ];
  onSubmit() {
    var email = this.refs.email.value;
    $(this.refs.email).prop('disabled', true).parent().addClass('loading');
    server.post('/api/v1/account/identify/email', { email: email }).then({
      200: response => History.push({ pathname: '/login/password', query: { email }}),
      401: response => History.push({ pathname: '/login/register', query: { email }}),
      402: response => History.push({ pathname: '/login/pending' }),
    }).catch(err => {
      if (!err.handled) {
        alert(err.message);
        $(this.refs.email).prop('disabled', false).focus().select().parent().removeClass('loading');
      }
    });
  }
  render() {
    return(
      <Page className="login-email">
        {DefaultPageTitle}
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
  static elements = [ DefaultPageElements, BackLink({ key: 'back-from-password', to: '/login/email' }) ];
  state = {}
  onSubmit() {
    var password = this.refs.password.value;
    $(this.refs.password).prop('disabled', true).parent().addClass('loading');
    server.post('/api/v1/account/authenticate/password', { password }).then({
      200: response => setAccount(response.data),
      401: response => { this.displayWrongPasswordMessage(); throw null; },
    }).catch(err => {
      if (err && !err.handled) {
        alert(err.message);
        console.dir(err);
      }
      $(this.refs.password).prop('disabled', false).focus().select().parent().removeClass('loading');
    });
  }
  displayWrongPasswordMessage() {
    this.setState({ wrongPassword: setTimeout(() => this.hideWrongPasswordMessage(), 1000) });
  }
  hideWrongPasswordMessage() {
    if (this.state.wrongPassword) clearTimeout(this.state.wrongPassword);
    this.setState({ wrongPassword: null });
  }
  componentWillUnmount() {
    if (this.state.wrongPassword) clearTimeout(this.state.wrongPassword);
  }
  render() {
    return(
      <Page className="login-password">
        <PageTitle>
          <div className="welcome">Welcome back,</div>
          <div className="text">{this.props.location.query.email}!</div>
        </PageTitle>
        <div className={classList("ui icon input", { 'wrong-password' : this.state.wrongPassword })}>
          <input type="password" placeholder="Password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
          <i className="chevron right link icon" onClick={() => this.onSubmit()}></i>
        </div>
        <ExternalLink className="underline forgot link" href="/recover" params={{ email: this.props.location.query.email }} tabIndex="0">Forgot password?</ExternalLink>
      </Page>
    );
  }
}

export class RegisterStep extends Page {
  static elements = [ DefaultPageElements, BackLink({ key: 'back-from-register', to: '/login/email' }) ];
  onSubmit() {
    var password = this.refs.password.value;
    $(this.refs.password).prop('disabled', true);
    $(this.refs.register).hide();
    $(this.refs.loader).addClass('active');
    server.post('/api/v1/account/register/signup', { email: this.props.location.query.email, password }).then(response => {
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
        <PageTitle><div className="welcome">Welcome,</div><div className="text">{this.props.location.query.email}!</div></PageTitle>
        <div className="desc">Setting up your new account...</div>
        <div className="ui icon input">
          <input type="password" placeholder="Choose a password" required autoFocus="true" ref="password" onKeyPress={e => { if (e.key == 'Enter') { this.onSubmit(); e.preventDefault(); } }} />
        </div>
        <a class="register link" ref="register" onClick={() => this.onSubmit()}>SIGN UP <i className="right chevron icon"/></a>
        <div className="ui inline text loader" ref="loader"></div>
        {/*<Link className="back link" to="/login/email" tabIndex="0"><i className="undo icon"></i>Back</Link>*/}
      </Page>
    );
  }
}

export class ConfirmationStep extends Page {
  static elements = [ WaitingPageBackground, PagePipe(), PageHeader(), BackLink({ key: 'back-from-confirm', to: '/login/email' }) ];
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
    if (this.attempts++ < 10) {
      refreshAccount(); // navigates automatically
    } else {
      this.endPolling();
      this.setState({ timedOut: true });
    }
  }
  onRetry() {
    this.beginPolling();
    this.setState({ timedOut: false });
  }
  onResend() {
    server.post('/api/v1/account/confirm/resend', { email: this.props.location.query.email })
      .then(() => {
        this.onRetry();
      }, err => {
        Application.showMessageBox({ title: "Failed to resend", message: "An error occurred when attempting to resend the confirmation email. Please try logging in again, or restarting the app." });
      });
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
          <PageTitle><div className="welcome">Setting up...</div><div className="text">{this.props.location.query.email}</div></PageTitle>
          <div className="desc" style={{ marginBottom: '2em' }}>
            We haven't received your<br/>email confirmation yet.
          </div>
          <a className="button" tabIndex="0" onClick={() => this.onRetry()}><i className="refresh icon"/> Check again</a>
          <a className="button" tabIndex="0" onClick={() => this.onResend()}><i className="mail outline icon"/> Resend email</a>
        </Page>
        :
        <Page className="login-confirm">
          <PageTitle><div className="welcome">Setting up...</div><div className="text">{this.props.location.query.email}</div></PageTitle>
          <div className="desc">Please check your email<br/>to confirm your account.</div>
          <div className="ui inline active large text loader" ref="loader"></div>
        </Page>
    );
  }
}

export class PendingStep extends Page {
  static elements = [ WaitingPageBackground, PagePipe(), PageHeader(), BackLink({ key: 'back-from-pending', to: '/login/email' }) ];
  render() {
    return (
      <Page className="login-pending">
        <PageTitle><div className="text">Please wait a little longer!</div></PageTitle>
        <div className="desc">Sorry, we are currently only offering limited previews via invitations only.<br/><br/>We'll let you know as soon as we launch, so please keep an eye on your inbox!</div>
      </Page>
    );
  }
}


const POST_ANALYTICS_STEP = '/main'; // '/tutorial/0';

export class AnalyticsStep extends Page {
  static elements = [ AnalyticsPageBackground ];
  onAllow() {
    daemon.call.applySettings({ enableAnalytics: true }).then(() => { analytics.activate(); History.push(POST_ANALYTICS_STEP); });
  }
  onDisallow() {
    daemon.call.applySettings({ enableAnalytics: false }).then(() => History.push(POST_ANALYTICS_STEP));
  }
  render() {
    return (
      <Page className="login-analytics" top={260}>
        <div className="desc">
          Share anonymized analytics about your VPN connections to help make Cypherpunk Privacy faster and more reliable?
        </div>
        <button className="allow" onClick={e => { this.onAllow(); e.preventDefault(); }}>Allow</button>
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
