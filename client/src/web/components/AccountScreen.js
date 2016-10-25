import React from 'react';
import { Link } from 'react-router';
import { PanelTitlebar } from './Titlebar';
import Modal from './modal';
import daemon from '../daemon';
const { shell } = require('electron').remote;

import AccountIcon from '../assets/img/icon-account-big.svg';

export default class AccountScreen extends React.Component  {
  isPremium() {
    return daemon.settings.subscription.type == 'premium';
  }
  getPlanName() {
    return this.isPremium() ? "Premium" : "Free";
  }
  getRenewalDate() {
    var d = new Date(0);
    d.setUTCSeconds(daemon.settings.subscription.expiration);
    return `${d.getMonth()+1}/${d.getDate()}/${d.getFullYear()}`;
  }
  render() {
    var renewal = this.isPremium() && daemon.settings.subscription.expiration != 0 ? <div className="period">Renews on {this.getRenewalDate()}</div> : null;
    return(
      <Modal className="account left panel" onClose={() => { History.push('/connect'); }}>
        <PanelTitlebar title="Account" back="/connect"/>
        <div className="scrollable content">
          <div className="user pane">
            <div className="user"><img src={AccountIcon}/><span className="email">{daemon.account.email}</span></div>
            <div className={this.isPremium() ? "premium plan" : "plan"}>{this.getPlanName()}</div>
            {renewal}
          </div>
          <div className="pane" data-title="Account Settings">
            <div className="setting"><a tabIndex="0" onClick={() => { shell.openExternal('https://cypherpunk.engineering/user/upgrade?user=' + encodeURI(daemon.account.email) + '&secret=' + encodeURI(daemon.account.secret)); }}>{this.isPremium() ? "Change Plan" : "Upgrade"}</a></div>
            <div className="setting"><a tabIndex="0"><div>Email<small>{daemon.account.email}</small></div></a></div>
            <div className="setting"><a tabIndex="0">Password</a></div>
          </div>
          <div className="pane" data-title="More">
            <div className="setting"><a tabIndex="0">Get 30 Days Premium Free</a></div>
            <div className="setting"><a tabIndex="0">Rate Our Service</a></div>
            <div className="setting"><a tabIndex="0"><div>Message the Founders<small>We love interacting with our users, all messages are responded to within 12 hours.</small></div></a></div>
            <div className="setting"><a tabIndex="0">Help</a></div>
            <div className="setting"><a tabIndex="0" onClick={() => History.push('/login')}>Log Out</a></div>
          </div>
        </div>
      </Modal>
    );
  }
}
