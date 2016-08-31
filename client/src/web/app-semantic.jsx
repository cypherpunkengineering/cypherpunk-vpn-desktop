require('./assets/fonts/dosis.css');
require('./semantic/semantic.js');
require('./assets/less/app-semantic.less');

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, hashHistory as History } from 'react-router';

import SpinningImage from './assets/img/bgring3.png';

import RPC from '../rpc.js';

let daemon = new RPC({
  url: 'ws://127.0.0.1:9337/',
  // Note: Disabling the error handler will make the RPC attempt to reconnect infinitely in the background
  //onerror: () => { window.alert("Unable to connect to background service.", "Cypherpunk VPN"); require('electron').remote.app.quit(); },
  onopen: () => { console.log("Established connection"); History.push("/connect"); console.log(window.location.href); }
});


class MainBackground extends React.Component {
  render() {
    return(
      <div id="main-background">
        <div>
          <img class="r1" src={SpinningImage}/>
          <img class="r2" src={SpinningImage}/>
          <img class="r3" src={SpinningImage}/>
          <img class="r4" src={SpinningImage}/>
          <img class="r5" src={SpinningImage}/>
          <img class="r6" src={SpinningImage}/>
          <img class="r7" src={SpinningImage}/>
        </div>
      </div>
    );
  }
}

class Titlebar extends React.Component {
  componentDidMount() {
    $(this.refs.dropdown).dropdown({ action: 'hide' });
  }
  render() {
    return(
      <div id="titlebar" class="ui fixed inverted borderless icon menu">
        <div class="header item">Cypherpunk VPN</div>
        <div class="right menu">
          <a id="wifi" class="item"><i class="wifi icon"></i></a>
          <div class="ui compact dropdown link item" ref="dropdown">
            <i class="setting icon"></i><i class="small caret down icon"></i>
            <div class="ui menu">
              <a class="item">Settings</a>
              <a class="item" onClick={function(){window.close();}}>Exit</a>
            </div>
          </div>
        </div>
      </div>
    );
  }
}

class LoadDimmer extends React.Component {
  render() {
    return(
      <div id="load-screen" class="full screen ui active dimmer" style={{visibility: 'visible', zIndex: 20}}>
        <div class="ui big text loader">Loading</div>
      </div>
    );
  }
}

class ConnectScreen extends React.Component {
  componentDidMount() {
    $(this.refs.regionDropdown).dropdown();
  }
  render() {
    return(
      <div id="connect-screen" class="full screen" style={{visibility: 'visible'}}>
        <div id="connect-container">
          <i id="connect" class="ui red fitted massive power link icon"></i>
        </div>
        <div id="connect-status">DISCONNECTED</div>
        <div id="region-select" class="ui selection dropdown" ref="regionDropdown">
          <input type="hidden" name="region" value="208.111.52.1 7133"/>
          <i class="dropdown icon"></i>
          <div class="default text">Select Region</div>
          <div class="menu">
            <div class="item" data-value="208.111.52.1 7133"><i class="jp flag"></i>Tokyo 1, Japan</div>
            <div class="item" data-value="208.111.52.2 7133"><i class="jp flag"></i>Tokyo 2, Japan</div>
            <div class="item" data-value="199.68.252.203 7133"><i class="us flag"></i>Honolulu, HI, USA</div>
          </div>
        </div>
        <div id="connection-stats" class="ui hidden two column centered grid">
          <div class="column"><div class="ui mini statistic"><div class="value" ref="received"></div><div class="label">Received</div></div></div>
          <div class="column"><div class="ui mini statistic"><div class="value" ref="sent"></div><div class="label">Sent</div></div></div>
        </div>
      </div>
    );
  }
}

class MainScreen extends React.Component {

}

class SettingsScreen extends React.Component {
  
}

class RootContainer extends React.Component {
  render() {
    return(
      <div class="full screen" style={{visibility: 'visible'}}>
        <MainBackground/>
        <Titlebar/>
        <Router history={History}>
          <Route path="/connect" component={ConnectScreen}/>
          <Route path="/" component={LoadDimmer}/>
        </Router>
      </div>
    );
  }
}

$(document).ready(() => {
  console.log("DOMContentLoaded");
  var root = document.createElement('div');
  root.id = 'root-container';
  document.body.appendChild(root)
  ReactDOM.render(<RootContainer />, root);
});
