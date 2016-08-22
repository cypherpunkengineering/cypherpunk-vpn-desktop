require('../less/app.less');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, hashHistory } from 'react-router';

import Interface from './ui/Interface.js';
import ConnectUI from './ui/ConnectUI.js';
import Login from './ui/Login.js';
import Settings from './ui/Settings.js';
import Account from './ui/Account.js';

const app = document.getElementById('app');

ReactDOM.render(
  <Router history={hashHistory}>
    <Route path="/" component={Login}></Route>
    <Route path="/interface" component={Interface}>
      <IndexRoute component={ConnectUI}></IndexRoute>
      <Route path="/Settings" component={Settings}></Route>
      <Route path="/account" component={Account}></Route>
    </Route>
    <Route path="*" component={Login}></Route>
  </Router>,
  app);
