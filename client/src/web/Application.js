import React from 'react';
import { render } from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory } from 'react-router';
import LoginScreen from './components/LoginScreen';
import ConnectScreen from './components/ConnectScreen';
import ConfigurationScreen from './components/ConfigurationScreen'
import EmailScreen from './components/config/EmailScreen'
import EncryptionScreen from './components/config/EncryptionScreen'
import FirewallScreen from './components/config/FirewallScreen'
import HelpScreen from './components/config/HelpScreen'
import PasswordScreen from './components/config/PasswordScreen'
import StatusScreen from './components/StatusScreen'
import './assets/css/app.less';

// import { configureStore } from './store/configureStore';
import Root from './components/Root';
import daemon from './daemon.js';

daemon.ready(() => {
  daemon.once('state', state => {
    hashHistory.push('/login');
  });
  daemon.post.get('state');
});

// const store = configureStore();

render((
  <Router history={hashHistory}>
    <Route path="/" component={Root}>
      {/*<IndexRoute component={LoginScreen}/>*/}
      <Route path="/login" component={LoginScreen}/>
      <Route path="/connect" component={ConnectScreen}/>
      <Route path="/configuration" component={ConfigurationScreen}>
        <Route path="/email" component={EmailScreen}/>
        <Route path="/password" component={PasswordScreen}/>
        <Route path="/encryption" component={EncryptionScreen}/>
        <Route path="/firewall" component={FirewallScreen}/>
        <Route path="/help" component={HelpScreen}/>
      </Route>
      <Route path="/status" component={StatusScreen}/>
      <IndexRedirect to="/login"/>
    </Route>
  </Router>
), document.getElementById('root-container'));
