import React from 'react';
import { Link } from 'react-router';
import { PanelTitlebar } from './Titlebar';
import Modal from './modal';
import daemon from '../daemon';
import RouteTransition from './Transition';
import AccountIcon from '../assets/img/icon-account-big.svg';

const transitionMap = {
  'email': 'swipeLeft',
  'password': 'swipeLeft',
  'account': 'swipeRight'
};

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
  getContent() {
    if (this.props.children) {
      return this.props.children
    }
    else {
      return(
        <div>
        <PanelTitlebar title="Account" back="/connect"/>
        <div className="scrollable content">
          <div className="user pane">
            <div className="user"><img src={AccountIcon}/><span className="email">{daemon.account.email}</span></div>
            <div className="plan">{this.getPlanName()}</div>
            <div className="period">Renews on {this.getRenewalDate()}</div>
          </div>
          <div className="pane" data-title="Account Settings">
            <div className="setting"><Link to="/account/email" tabIndex="0"><div>Email<small>{daemon.account.email}</small></div></Link></div>
            <div className="setting"><Link to="/account/password" tabIndex="0">Password</Link></div>
          </div>
          <div className="pane" data-title="More">
            <div className="setting"><a tabIndex="0">Get 30 Days Premium Free</a></div>
            <div className="setting"><a tabIndex="0">Rate Our Service</a></div>
            <div className="setting"><a tabIndex="0"><div>Message the Founders<small>We love interacting with our users, all messages are responded to within 12 hours.</small></div></a></div>
            <div className="setting"><a tabIndex="0">Help</a></div>
            <div className="setting"><a tabIndex="0" onClick={() => History.push('/login')}>Log Out</a></div>
          </div>
        </div>
        </div>
      );
    }
  }

  render() {
    // console.log(this.props.location.pathname);
    var pathname = this.props.location.pathname;
    var pathArray = pathname.split("/");
    var currentRoute = pathArray[pathArray.length - 1];
    var transition = transitionMap[currentRoute];
    console.log(transition);
    console.log(transitionMap);
    return(
      <Modal className="account left panel" onClose={() => { History.push('/connect'); }}>
        <RouteTransition transition={transition}>
          {this.getContent()}
        </RouteTransition>
      </Modal>
    );
  }
}
