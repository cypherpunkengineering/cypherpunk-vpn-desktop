import React from 'react';
import { render } from 'react-dom';
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

daemon.ready(() => {
  daemon.once('state', state => {
    hashHistory.push('/login');
  });
  daemon.post.get('state');
});

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

// const store = configureStore();
window.History = hashHistory;

render((
  <Router history={window.History}>
    <Route path="/" component={RouteTransition} transition={transitionMap}>
      {/*<IndexRoute component={LoginScreen}/>*/}
      <Route path="login" component={LoginScreen}>
        <Route path="email" component={Login.EmailStep}/>
        <Route path="password" component={Login.PasswordStep}/>
        <Route path="register" component={Login.RegisterStep}/>
        <Route path="confirm" component={Login.ConfirmationStep}/>
        <IndexRedirect to="email"/>
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
      <IndexRedirect to="login"/>
    </Route>
  </Router>
), document.getElementById('root-container'));
