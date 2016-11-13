import React from 'react';
import { Link } from 'react-router';
import { PanelTitlebar } from './Titlebar';
import Modal from './modal';
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
    return daemon.settings.subscription.type == 'premium';
  }
  getPlanName() {
    return this.isPremium() ? "Premium" : "Free";
  }
  getRenewalDate() {
    //var d = new Date(0);
    //d.setUTCSeconds(daemon.settings.subscription.expiration);
    var d = new Date(daemon.settings.subscription.expiration);
    return `${d.getMonth()+1}/${d.getDate()}/${d.getFullYear()}`;
  }
  getContent() {
    if (this.props.children) {
      return this.props.children
    }
    else {
      var renewal = this.isPremium() && daemon.settings.subscription.expiration != 0 ? <div className="period">Renews on {this.getRenewalDate()}</div> : null;
      return(
        <div>
        <PanelTitlebar title="Account" back="/connect"/>
        <div className="scrollable content">
          <div className="user pane">
            <div className="user"><img src={AccountIcon}/><span className="email">{daemon.account.email}</span></div>
            <div className={this.isPremium() ? "premium plan" : "plan"}>{this.getPlanName()}</div>
            {renewal}
          </div>
          <div className="pane" data-title="Account Settings">
            <div className="setting"><a tabIndex="0" onClick={() => { shell.openExternal('https://cypherpunk.engineering/user/upgrade?user=' + encodeURI(daemon.account.email) + '&secret=' + encodeURI(daemon.account.secret)); }}>{this.isPremium() ? "Change Plan" : "Upgrade"}</a></div>
            <div className="setting"><Link to="/account/email" tabIndex="0"><div>Email<small>{daemon.account.email}</small></div></Link></div>
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
