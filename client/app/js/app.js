//require('../less/app.less');
require('../sass/app.scss');
//const RPC = require('./rpc.js');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, hashHistory } from 'react-router';

import Account from './ui/screens/Configs/Account.js';
import Configs from './ui/screens/Configs.js';
import Connect from './ui/screens/Connect.js';
import GeneralAdvanced from './ui/screens/Configs/GeneralAdvanced.js';
import Login from './ui/screens/Login.js';

const app = document.getElementById('app');

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

ReactDOM.render( <CypherPunkApp />, app);
