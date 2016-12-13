import React from 'react';
import { Link } from 'react-router';
import { PanelTitlebar } from './Titlebar';
import Modal from './Modal';
import daemon from '../daemon';
import RouteTransition from './Transition';
const { shell } = require('electron').remote;

import AccountIcon from '../assets/img/icon-account-big.svg';

const transitionMap = {
  '': { '*': 'swipeLeft' },
  '*': { '': 'swipeRight', 'account/*': 'swipeRight' },
};

export default class AccountScreen extends React.Component  {
  isPremium() {
    return daemon.account.account.type !== 'free';
  }
  getPlanName() {
    return {
      'free': "Free",
      'premium': "Premium",
      'family': "Family",
      'enterprise': "Enterprise",
      'staff': "Staff",
      'developer': "Developer",
    }[daemon.account.account.type];
  }
  getRenewalType() {
    return {
      'annually': "annually",
      'semiannually': "semiannually",
      'monthly': "monthly",
      '12m': "annually",
      '1y': "annually",
      '6m': "semiannually",
      '3m': "quarterly",
      '1m': "monthly",
      'forever': "forever"
    }[daemon.account.subscription.renewal];
  }
  getRenewalString() {
    if (daemon.account.subscription.renewal === 'forever') {
      return "Lifetime";
    } else if (daemon.account.subscription.expiration != 0) { // note: !=, not !==
      var d = new Date(daemon.account.subscription.expiration);
      var now = new Date();
      return `${d<=now ? "Expired" : "Renews " + this.getRenewalType()} on ${d.getMonth()+1}/${d.getDate()}/${d.getFullYear()}`;
    } else {
      return "Renews " + this.getRenewalType();
    }
  }
  getContent() {
    if (this.props.children) {
      return this.props.children
    }
    else {
      var renewal = this.isPremium() ? <div className="period">{this.getRenewalString()}</div> : null;
      var upgradeString = "Manage Account";
      var upgradeURL = '/account';
      switch (daemon.account.account.type) {
        case 'free':
          upgradeString = "Upgrade to Premium";
          upgradeURL = '/account/upgrade';
          break;
        case 'premium':
        case 'family':
          switch (daemon.account.subscription.renewal) {
            case 'annually':
            case 'forever':
            case '12m':
              break;
            default:
              upgradeString = "Change Subscription"
              upgradeURL = '/account/upgrade';
              break;
          }
          break;
      }
      return(
        <div>
        <PanelTitlebar title="My Account" back="/connect"/>
        <div className="scrollable content">
          <div className="user pane">
            <div className="user"><img src={AccountIcon}/><span className="email">{daemon.account.account.email}</span></div>
            <div className={this.isPremium() ? "premium plan" : "plan"}>{this.getPlanName()}</div>
            {renewal}
          </div>
          <div className="pane" data-title="Account Settings">
            <div className="setting"><a tabIndex="0" onClick={() => { shell.openExternal('https://cypherpunk.com' + upgradeURL + '?user=' + encodeURIComponent(daemon.account.account.email) + '&secret=' + encodeURIComponent(daemon.account.secret)); }}>{upgradeString}</a></div>
            <div className="setting"><Link to="/account/email" tabIndex="0"><div>Email<small>{daemon.account.account.email}</small></div></Link></div>
            <div className="setting"><Link to="/account/password" tabIndex="0">Password</Link></div>
          </div>
          <div className="pane" data-title="More">
            <div className="setting"><Link to="/account/share" tabIndex="0">Share Cypherpunk with a Friend</Link></div>
            <div className="setting"><a tabIndex="0" onClick={() => { shell.openExternal('https://cypherpunk.zendesk.com/tickets/new'); }}><div>Message the Founders<small>We love interacting with our users, all messages are responded to within 12 hours.</small></div></a></div>
            <div className="setting">{/*<Link to="/account/help" tabIndex="0">Help</Link>*/}<a tabIndex="0" onClick={() => { shell.openExternal('https://support.cypherpunk.com/hc'); }}>Help</a></div>
            <div className="setting"><Link to="/login/logout" tabIndex="0">Log Out</Link></div>
          </div>
        </div>
        </div>
      );
    }
  }

  render() {
    var pathname = this.props.location.pathname;
    var pathArray = pathname.split("/");
    var currentRoute = pathArray[pathArray.length - 1];
    var transition = transitionMap[currentRoute];

    return(
      <Modal className="account left panel" onClose={() => { History.push('/connect'); }}>
        <RouteTransition transition={transitionMap}>
          {this.getContent()}
        </RouteTransition>
      </Modal>
    );
  }
}
