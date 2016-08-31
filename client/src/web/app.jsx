require('./assets/less/app.less');
//const RPC = require('./rpc.js');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, hashHistory } from 'react-router';

import Account from './ui/screens/Configs/Account.js';
import Configs from './ui/screens/Configs.js';
import Connect from './ui/screens/Connect.js';
import GeneralAdvanced from './ui/screens/Configs/GeneralAdvanced.js';
import Login from './ui/screens/Login.js';
import RPC from '../rpc.js';

class CypherPunkApp extends React.Component {

  render() {
    return(
      <Router history={hashHistory}>
        <Route path="/" component={Login}></Route>
        <Route path="/connect" component={Connect}></Route>
        <Route path="/configs" component={Configs}>
          <IndexRoute component={GeneralAdvanced}></IndexRoute>
          <Route path="/account" component={Account}></Route>
        </Route>
        <Route path="*" component={Login}></Route>
      </Router>
    );
  }
}

$(document).ready(() => {
  console.log("DOMContentLoaded");
  var root = document.createElement('div');
  root.id = 'root-container';
  document.body.appendChild(root)
  ReactDOM.render(<CypherPunkApp />, root);
});
