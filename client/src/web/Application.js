import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
import { ipcRenderer, remote } from 'electron';
import LoginScreen, * as Login from './components/LoginScreen';
import ConnectScreen from './components/ConnectScreen';
import ConfigurationScreen from './components/ConfigurationScreen';
import EmailScreen from './components/account/EmailScreen';
import ShareScreen from './components/account/ShareScreen';
import PrivacyScreen from './components/config/PrivacyScreen';
import TrustedNetworksScreen from './components/config/TrustedNetworks';
import RemotePortScreen from './components/config/RemotePortScreen';
import FirewallScreen from './components/config/FirewallScreen';
import HelpScreen from './components/account/HelpScreen';
import PasswordScreen from './components/account/PasswordScreen';
import AccountScreen from './components/AccountScreen';

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
import RouteTransition from './components/Transition';
import daemon from './daemon.js';
import server from './server.js';
import { compareVersions } from './util.js';

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
      setImmediate(() => History.push('/login/email'));
    }

    // Listen for daemon state changes
    daemon.on('state', Application.daemonStateChanged);

    History.push('/login');
    // TODO: Later we'll probably want to run this at a different timing
    Application.checkForUpdates();
  }
  static checkForUpdates() {
    server.get('/api/v0/app/versions').then(response => {
      var version = response.data[({ 'darwin':'macos', 'win32':'windows', 'linux':'debian' })[process.platform]];
      if (version !== undefined) {
        function downloadAndInstall() {
          return new Promise((resolve, reject) => {
            if (false && version.url) { // TODO: Download to temporary directory and run installer

            } else {
              remote.shell.openExternal('https://cypherpunk.com/download');
            }
            resolve();
          });
        }
        var current = remote.app.getVersion();
        if (compareVersions(version.required, current) > 0) {
          Application.showMessageBox({
            type: 'warning',
            title: "Update required",
            message: "In order to keep using Cypherpunk Privacy, a software update is required.\n\n" + current + " -> " + version.latest,
            buttons: [ version.url ? "Download and install" : "Go to download page", "Quit" ],
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
            let strings = {
              '1': ["Install important update?", "A major update is available; would you like to install it?"],
              '2': ["Install recommended update?", "A recommended update is available; would you like to install it?"],
              '3': ["Install update?", "An update is available; would you like to install it?"],
            };
            let [title, message] = strings[diff] || strings[3];
            Application.showMessageBox({
              type: 'info',
              title,
              message,
              buttons: [ version.url ? "Download and install" : "Go to download page", "Ignore" ],
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
  static showMessageBox(options, callback) {
    if (process.platform === 'darwin') {
      // On macOS what would normally be the title should instead be the message, and the message should be the detail
      options.detail = options.message;
      options.message = options.title;
      delete options.title;
    }
    return remote.dialog.showMessageBox(remote.getCurrentWindow(), options, callback);
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
  static daemonStateChanged(state) {
    if (state.state) {
      switch (state.state) {
        case 'CONNECTED':
          $(document.body).one('transitionend', () => { $(document.documentElement).removeClass('online'); });
          $(document.documentElement).addClass('online');
          break;
        case 'DISCONNECTED':
          $(document.body).one('transitionend', () => { $(document.documentElement).removeClass('error'); });
          $(document.documentElement).addClass('error');
          break;
      }
    }
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
            <Route path="analytics" component={Login.AnalyticsStep}/>
            <Route path="logout" component={Login.Logout}/>
            <IndexRedirect to="check"/>
          </Route>
          <Route path="connect" component={ConnectScreen}>
            <Route path="/configuration" component={ConfigurationScreen}>
              <Route path="privacy" component={PrivacyScreen}/>
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
