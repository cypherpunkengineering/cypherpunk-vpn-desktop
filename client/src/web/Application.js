import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
import LoginScreen, * as Login from './components/LoginScreen';
import ConnectScreen from './components/ConnectScreen';
import ConfigurationScreen from './components/ConfigurationScreen'
import EmailScreen from './components/config/EmailScreen'
import EncryptionScreen from './components/config/EncryptionScreen'
import FirewallScreen from './components/config/FirewallScreen'
import HelpScreen from './components/config/HelpScreen'
import PasswordScreen from './components/config/PasswordScreen'
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
    '*': {
      'login': '',
      'root': '',
      '*': 'fadeIn',
    },
};

export default class Application {
  static init() {
    server.refreshSession = () => {
      if (!daemon.account.email || !daemon.account.token) {
        throw new Error("No stored account credentials found");
      }
      return server.post('/api/v1/account/authenticate/token', { email: daemon.account.email, token: daemon.account.token }).then(data => true);
    };
    server.onAuthFailure = () => {
      setTimeout(() => History.push('/login/email'), 0);
    }
    History.push('/login');
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
            <IndexRedirect to="check"/>
          </Route>
          <Route path="connect" component={ConnectScreen}>
            <Route path="/configuration" component={ConfigurationScreen}>
              <Route path="email" component={EmailScreen}/>
              <Route path="password" component={PasswordScreen}/>
              <Route path="encryption" component={EncryptionScreen}/>
              <Route path="firewall" component={FirewallScreen}/>
              <Route path="help" component={HelpScreen}/>
            </Route>
            <Route path="/account" component={AccountScreen}/>
          </Route>
        </Route>
      </Router>
    );
  }
}

// const store = configureStore();
window.History = Application.History = hashHistory;

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

daemon.ready(() => {
  daemon.once('state', state => {
    Application.init();
  });
  daemon.post.get('state');
});

ReactDOM.render(Application.render(), document.getElementById('root-container'));
