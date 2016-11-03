import React from 'react';
import { render } from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
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

// const store = configureStore();
window.History = hashHistory;

daemon.ready(() => {
  daemon.once('state', state => {
    History.push('/login');
  });
  daemon.post.get('state');
});

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
      <IndexRedirect to="login"/>
    </Route>
  </Router>
), document.getElementById('root-container'));
