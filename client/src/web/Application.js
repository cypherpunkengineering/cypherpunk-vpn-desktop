import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
import { ipcRenderer } from 'electron';
import LoginScreen, * as Login from './components/LoginScreen';
import ConnectScreen from './components/ConnectScreen';
import ConfigurationScreen from './components/ConfigurationScreen'
import EmailScreen from './components/account/EmailScreen'
import ShareScreen from './components/account/ShareScreen'
import PrivacyScreen from './components/config/PrivacyScreen'
import TrustedNetworksScreen from './components/config/TrustedNetworks'
import FirewallScreen from './components/config/FirewallScreen'
import HelpScreen from './components/account/HelpScreen'
import PasswordScreen from './components/account/PasswordScreen'
import AccountScreen from './components/AccountScreen'
import './assets/css/app.less';

// import { configureStore } from './store/configureStore';
import RouteTransition from './components/Transition';
import daemon from './daemon.js';
import server from './server.js';

const transitionMap = {
    'login': {
      'connect': 'fadeIn',
    },
    'account': {
      'connect': 'swipeLeft',
    },
    'connect': {
      'account': 'swipeRight',
      'configuration': 'swipeLeft',
    },
    'configuration': {
      'configuration/*': 'swipeLeft',
      'connect': 'swipeRight',
    },
    'account': {
      'account/*': 'swipeLeft',
      'connect': 'swipeRight',
    },
    '*': {
      'login': '',
      'root': '',
      '*': 'fadeIn',
    },
};

export default class Application {
  static init() {
    server.refreshSession = () => {
      if (!daemon.account.account.email || !daemon.account.account.token) {
        throw new Error("No stored account credentials found");
      }
      return server.post('/api/v1/account/authenticate/token', { email: daemon.account.account.email, token: daemon.account.account.token }).then(data => true);
    };
    server.onAuthFailure = () => {
      setTimeout(() => History.push('/login/email'), 0);
    }
    History.push('/login');
  }
  static navigate(location) {
    Application.History.push(location);
  }
  static onNavigation(location, action) {
    ipcRenderer.send('navigate', location);
  }
  static isLoggedIn() {
    return Application.History.location.pathname.match(/^\/(?!login)./);
  }
  static render() {
    return (
      <Router history={window.History}>
        <Route path="/" component={RouteTransition} transition={transitionMap}>
          {/*<IndexRoute component={LoginScreen}/>*/}
          <Route path="login" component={LoginScreen}>
            <Route path="check" component={Login.Check}/>
            <Route path="email" component={Login.EmailStep}/>
            <Route path="password" component={Login.PasswordStep}/>
            <Route path="register" component={Login.RegisterStep}/>
            <Route path="confirm" component={Login.ConfirmationStep}/>
            <Route path="logout" component={Login.Logout}/>
            <IndexRedirect to="check"/>
          </Route>
          <Route path="connect" component={ConnectScreen}>
            <Route path="/configuration" component={ConfigurationScreen}>
              <Route path="privacy" component={PrivacyScreen}/>
              <Route path="firewall" component={FirewallScreen}/>
              <Route path="trustednetworks" component={TrustedNetworksScreen}/>
            </Route>
            <Route path="/account" component={AccountScreen}>
              <Route path="email" component={EmailScreen}/>
              <Route path="password" component={PasswordScreen}/>
              <Route path="help" component={HelpScreen}/>
              <Route path="share" component={ShareScreen}/>
            </Route>
          </Route>
        </Route>
      </Router>
    );
  }
}

// const store = configureStore();
window.History = Application.History = hashHistory;
hashHistory.listen((location, action) => Application.onNavigation(location, action));
ipcRenderer.on('navigate', (event, location) => Application.navigate(location));

// Add a simple handler for uncaught errors in promises (this will almost
// entirely only be used by XHR promises).
window.addEventListener('unhandledrejection', function (event) {
  if (event.reason.handled) {
    // e.g. a 403 which was "handled" by moving to the login screen
    event.preventDefault();
    return false;
  } else {
    console.error("Unhandled error: " + event.reason.message);
    console.dir(event.reason);
    alert(event.reason.message);
  }
});

$(window).blur(function(){
  $('body').removeClass('is-focused');
});

$(window).focus(function(){
  $('body').addClass('is-focused');
});

daemon.ready(() => {
  daemon.once('state', state => {
    Application.init();
  });
  daemon.post.get('state');
});

ReactDOM.render(Application.render(), document.getElementById('root-container'));
