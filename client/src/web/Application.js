window.$ = window.jQuery = require('jquery');

import React from 'react';
import { render } from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, browserHistory, hashHistory} from 'react-router';
import LoginScreen from './components/LoginScreen';
import ConnectScreen from './components/ConnectScreen';
import ConfigurationScreen from './components/ConfigurationScreen'
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
      <IndexRoute component={LoginScreen}/>
      <Route path="/login" component={LoginScreen}/>
      <Route path="/connect" component={ConnectScreen}/>
      <Route path="/configuration" component={ConfigurationScreen}/>
      <Route path="/status" component={StatusScreen}/>
    </Route>
  </Router>
), document.getElementById('root-container'));
