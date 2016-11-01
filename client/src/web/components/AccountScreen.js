import React from 'react';
import { Link } from 'react-router';
import { PanelTitlebar } from './Titlebar';
import Modal from './modal';
import daemon from '../daemon';

import AccountIcon from '../assets/img/icon-account-big.svg';

export default class AccountScreen extends React.Component  {

  getPlanName() {
    switch (daemon.account.plan) {
      // ...
    }
    return "Monthly Premium";
  }
  getRenewalDate() {
    return "02/02/2017";
  }

  render() {
    if (this.props.children) {
      return (
        <Modal className="account left panel" onClose={() => { History.push('/connect'); }}>
          {this.props.children}
        </Modal>
      )
    }
    else {
      return(
        <Modal className="account left panel" onClose={() => { History.push('/connect'); }}>
          <PanelTitlebar title="Account" back="/connect"/>
          <div className="scrollable content">
            <div className="user pane">
              <div className="user"><img src={AccountIcon}/><span className="email">{daemon.account.email}</span></div>
              <div className="plan">{this.getPlanName()}</div>
              <div className="period">Renews on {this.getRenewalDate()}</div>
            </div>
            <div className="pane" data-title="Account Settings">
              <div className="setting"><Link to="/account/email" tabIndex="0"><div>Email<small>{daemon.account.email}</small></div></Link></div>
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
}
