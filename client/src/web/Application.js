// While we use webpack's ProvidePlugin, there are apparently corner cases where
// this is not called early enough, which can break Semantic UI components. Work
// around it by explicitly installing these globals in this particular module.
global.$ = global.jQuery = require('jquery');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
import { ipcRenderer, remote } from 'electron';
import EventEmitter from 'events';
import LoginScreen, * as Login from './components/LoginScreen';
import MainScreen from './components/MainScreen';
import ConfigurationScreen from './components/ConfigurationScreen';
import EmailScreen from './components/account/EmailScreen';
import ShareScreen from './components/account/ShareScreen';
import ProfileScreen from './components/config/ProfileScreen';
import TrustedNetworksScreen from './components/config/TrustedNetworksScreen';
import RemotePortScreen from './components/config/RemotePortScreen';
import FirewallScreen from './components/config/FirewallScreen';
import HelpScreen from './components/account/HelpScreen';
import PasswordScreen from './components/account/PasswordScreen';
import AccountScreen from './components/AccountScreen';
import TutorialScreen from './components/TutorialScreen';

import 'semantic/components/button';
import 'semantic/components/checkbox';
import 'semantic/components/dimmer';
import 'semantic/components/dropdown';
import 'semantic/components/flag';
import 'semantic/components/icon';
import 'semantic/components/input';
import 'semantic/components/loader';
import 'semantic/components/popup';

import './assets/css/main.less';

// import { configureStore } from './store/configureStore';
import { TransitionGroup } from './components/Transition';
import daemon from './daemon.js';
import server from './server.js';
import analytics from './analytics.js';
import { compareVersions } from './util.js';

const ERROR_NAMES = {
  'TLS_HANDSHAKE_ERROR': "Server handshake error",
  'AUTHENTICATION_FAILURE': "Server authentication failure",
  'UNKNOWN_CRITICAL_ERROR': "Unknown error",
  'UNKNOWN_ERROR': "Unknown error",
};

function getTransition(diff) {
  if (diff.login === 'leave' && diff.main === 'enter') return 'fadeIn';
  if (diff.login === 'enter' || diff.root === 'enter') return null;
  return 'fadeIn';
}

let lastPath = null;

global.Application = null;
global.History = hashHistory;

export const RootContainer = (props) => <TransitionGroup transition={getTransition} {...props}/>;

export default class ApplicationClass extends EventEmitter {
  constructor() {
    super();

    this.on('error', function() {});

    global.Application = this;
    this.History = global.History;
    this.History.listen((location, action) => this.onNavigation(location, action));
    ipcRenderer.on('navigate', (event, location) => this.navigate(location));

    server.refreshSession = () => {
      if (!daemon.account.account.email || !daemon.account.account.token) {
        throw new Error("No stored account credentials found");
      }
      return server.post('/api/v1/account/authenticate/token', { email: daemon.account.account.email, token: daemon.account.account.token }).then(data => true);
    };
    server.onAuthFailure = () => {
      setImmediate(() => History.push('/login/logout'));
    }

    // Listen for daemon state changes
    daemon.on('state', state => this.daemonStateChanged(state));
    daemon.on('error', error => this.daemonError(error));

    if (daemon.settings.enableAnalytics) {
      analytics.activate();
    }

    History.push('/login');
    // TODO: Later we'll probably want to run this at a different timing
    this.checkForUpdates();
  }

  checkForUpdates() {
    server.get('/api/v1/app/versions' + (daemon.account.account && daemon.account.account.type === 'developer' ? '/developer' : '')).then(response => {
      var version = response.data[({ 'darwin':'macos', 'win32':'windows', 'linux':'debian' })[process.platform]];
      if (version !== undefined) {
        function downloadAndInstall() {
          return new Promise((resolve, reject) => {
            if (version.url) {
              remote.shell.openExternal(version.url);
            } else {
              remote.shell.openExternal('https://cypherpunk.com/download');
            }
            resolve();
          });
        }
        var current = remote.app.getVersion();
        let messageSuffix = '';
        if (typeof version.description === 'string' && version.description !== '') {
          messageSuffix += '\n\n' + version.description;
        }
        if (compareVersions(version.required, current) > 0) {
          this.showMessageBox({
            type: 'warning',
            title: "Update required",
            message: "In order to keep using Cypherpunk Privacy, a software update is required." + messageSuffix,
            buttons: [ version.url ? "Download" : "Go to download page", "Quit" ],
            defaultId : 0,
            cancelId: 1,
          }, button => {
            if (button == 0) {
              downloadAndInstall().then(() => {
                remote.app.quit();
              });
            } else {
              remote.app.quit();
            }
          });
        } else {
          let diff = compareVersions(version.latest, current), title, message;
          if (diff > 0) {
            let title, message;
            switch (diff) {
              case 1: [ title, message ] = [ "Install important update?", "A major update is available; would you like to install it?" ]; break;
              case 2: [ title, message ] = [ "Install recommended update?", "A recommended update is available; would you like to install it?" ]; break;
              default:
              case 3: [ title, message ] = [ "Install update?", "An update is available; would you like to install it?" ]; break;
            }
            message += messageSuffix;
            this.showMessageBox({
              type: 'info',
              title,
              message,
              buttons: [ version.url ? "Download" : "Go to download page", "Ignore" ],
              defaultId: 0,
              cancelId: 1,
            }, button => {
              if (button == 0) {
                downloadAndInstall();
              }
            })
          }
        }
      }
    }).catch(err => console.warn("Update check failed:", err));
  }
  showMessageBox(options, callback) {
    if (process.platform === 'darwin') {
      // On macOS what would normally be the title should instead be the message, and the message should be the detail
      options.detail = options.message;
      options.message = options.title;
      delete options.title;
    }
    return remote.dialog.showMessageBox(remote.getCurrentWindow(), options, callback);
  }
  navigate(location) {
    History.push(location);
  }
  onNavigation(location, action) {
    if (lastPath !== location.pathname) {
      lastPath = location.pathname;
      analytics.screenview(lastPath);
    }
    ipcRenderer.send('navigate', location);
  }
  isLoggedIn() {
    return History.location.pathname.match(/^\/(?!login)./);
  }
  daemonStateChanged(state) {
    if (state.state) {
      switch (state.state) {
        case 'CONNECTED':
          document.documentElement.className = 'online';
          break;
        case 'DISCONNECTED':
          document.documentElement.className = 'offline';
          break;
      }
    }
  }
  daemonError(error) {
    this.lastError = error;
    this.emit('error', error);
  }
  render() {
    return (
      <Router history={window.History}>
        <Route path="/" component={RootContainer}>
          {/*<IndexRoute component={LoginScreen}/>*/}
          <Route path="login" component={LoginScreen}>
            <Route path="check" component={Login.Check}/>
            <Route path="email" component={Login.EmailStep}/>
            <Route path="password" component={Login.PasswordStep}/>
            <Route path="register" component={Login.RegisterStep}/>
            <Route path="confirm" component={Login.ConfirmationStep}/>
            <Route path="pending" component={Login.PendingStep}/>
            <Route path="analytics" component={Login.AnalyticsStep}/>
            <Route path="logout" component={Login.Logout}/>
            <IndexRedirect to="check"/>
          </Route>
          <Route path="main" component={MainScreen}>
            <Route path="/tutorial/:page" component={TutorialScreen}/>
            <Redirect from="/tutorial" to="/tutorial/0"/>
            <Route path="/configuration" component={ConfigurationScreen}>
              <Route path="privacy" component={ProfileScreen}/>
              <Route path="firewall" component={FirewallScreen}/>
              <Route path="trustednetworks" component={TrustedNetworksScreen}/>
              <Route path="remoteport" component={RemotePortScreen}/>
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


// Add a simple handler for uncaught errors in promises (this will almost
// entirely only be used by XHR promises).
window.addEventListener('unhandledrejection', function (event) {
  if (event.reason.handled) {
    // e.g. a 403 which was "handled" by moving to the login screen
    event.preventDefault();
    analytics.exception(event.reason.toString(), 0);
    return false;
  } else {
    console.error("Unhandled error: " + event.reason.message);
    console.dir(event.reason);
    analytics.exception(event.reason.toString(), 1);
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
  global.Application = new ApplicationClass();
  ReactDOM.render(Application.render(), document.getElementById('root-container'));
});
