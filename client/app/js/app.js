require('../less/app.less');
//const RPC = require('./rpc.js');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, hashHistory } from 'react-router';

import Account from './ui/screens/Configs/Account.js';
import Configs from './ui/screens/Configs.js';
import GeneralAdvanced from './ui/screens/Configs/GeneralAdvanced.js';
import Login from './ui/screens/Login.js';

const app = document.getElementById('app');

ReactDOM.render(
  <Router history={hashHistory}>
    <Route path="/" component={Login}></Route>
    <Route path="/configs" component={Configs}>
      <IndexRoute component={GeneralAdvanced}></IndexRoute>
      <Route path="/account" component={Account}></Route>

    </Route>
    <Route path="*" component={Login}></Route>
  </Router>,
  app);
