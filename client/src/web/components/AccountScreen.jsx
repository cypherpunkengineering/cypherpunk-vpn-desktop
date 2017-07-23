require('./AccountScreen.less');
import React from 'react';
import { Link } from 'react-router';
import ExternalLink from './ExternalLink';
import { PanelTitlebar } from './Titlebar';
import { Subpanel, PanelContent } from './Panel';
import daemon, { DaemonAware } from '../daemon';
import RouteTransition from './Transition';
import RetinaImage from './Image';
import { refreshAccountIfNeeded } from './LoginScreen';
import analytics from '../analytics';
const { shell } = require('electron').remote;

import AccountIcon from '../assets/img/icon-account-big.svg';

const AccountBanner = {
  free: { [1]: require('../assets/img/account_banner_free.png'), [2]: require('../assets/img/account_banner_free@2x.png') },
  premium: { [1]: require('../assets/img/account_banner_premium.png'), [2]: require('../assets/img/account_banner_premium@2x.png') },
};


const ACCOUNT_TYPE_NAMES = {
  'free': "Trial Account",
  'premium': "Premium Account",
  'family': "Family Account",
  'enterprise': "Enterprise Account",
  'staff': "Staff",
  'developer': "Developer",
  'expired': "Expired Account",
};
const SUBSCRIPTION_SCHEDULE_NAMES = {
  'annually': "12 Month Plan",
  'semiannually': "6 Month Plan",
  'monthly': "1 Month Plan",
  '12m': "12 Month Plan",
  '1y': "12 Month Plan",
  '6m': "6 Month Plan",
  '3m': "3 Month Plan",
  '1m': "1 Month Plan",
};
const SUBSCRIPTION_SCHEDULE_DAYS = {
  'annually': 365,
  'semiannually': 182,
  'monthly': 30,
  '12m': 365,
  '1y': 365,
  '6m': 180,
  '3m': 90,
  '1m': 30,
};

function parseExpirationString(value) {
  if (+value == 0) return null;
  value = new Date(value);
  if (isNaN(value.getTime())) return null;
  return value;
}

export function getAccountStatus(account) {
  if (!account) account = daemon.account;
  let { subscription } = account;
  if (!subscription) return 'invalid';
  let active = subscription.active;
  let plan = subscription.renewal || '';
  let expiration = parseExpirationString(subscription.expiration);

  if (expiration && expiration < new Date()) return 'expired';
  return active ? 'active' : 'inactive';
}

export function describeRelativeDate(date, now) {
  if (!now) now = new Date();
  let future = date >= now;
  let diff = (date.getTime() - now.getTime()) / (60 * 60 * 1000); // hours

  if (diff < 0) {
    diff = -diff;
    if (diff < 1) {
      return "just now";
    } else if (diff <= 2) {
      return "an hour ago";
    } else if (diff < 6) {
      //return diff.toFixed() + " hours ago";
      return "less than six hours ago";
    } else if (diff <= 2 * 24) {
      if (date.getDate() == now.getDate()) {
        return "today";
      }
      let yesterday = new Date(now); yesterday.setDate(now.getDate() - 1);
      if (date.getDate() == yesterday.getDate()) {
        return "yesterday";
      }
    }
    if (diff <= 30 * 24) {
      return `${Math.ceil(diff / 24).toFixed()} days ago`;
    } else if (diff <= 50 * 24) {
      return `${Math.ceil(diff / (7 * 24)).toFixed()} weeks ago`;
    } else if (diff <= 300 * 24) {
      return `${Math.ceil(diff / (30 * 24)).toFixed()} months ago`;
    } else if (diff <= 400 * 24) {
      return "a year ago";
    } else {
      return "over a year ago";
    }
  } else {
    if (diff < 1) {
      return "in less than an hour";
    } else if (diff < 6) {
      return "in less than six hours";
    } else if (diff <= 2 * 24) {
      if (date.getDate() == now.getDate()) {
        return "today";
      }
      let tomorrow = new Date(now); tomorrow.setDate(now.getDate() + 1);
      if (date.getDate() == tomorrow.getDate()) {
        return "tomorrow";
      }
    }
    if (diff <= 30 * 24) {
      return `in ${Math.ceil(diff / 24).toFixed()} days`;
    } else if (diff <= 50 * 24) {
      return `in ${Math.ceil(diff / (7 * 24)).toFixed()} weeks`;
    } else if (diff <= 300 * 24) {
      return `in ${Math.ceil(diff / (30 * 24)).toFixed()} months`;
    } else if (diff <= 400 * 24) {
      return "in one year";
    } else {
      return "in over a year";
    }
  }
}


const AccountIllustration = ({ src, ...props }) => (
  <div className="background" {...props}>
    <RetinaImage src={src}/>
  </div>
);
const AccountUserField = ({ text, ...props }) => (
  <div className="user row">
    <i className="user icon"/>
    <span>{text}</span>
  </div>
);
const AccountPlanField = ({ className = '', text, description = null, descriptionClassName = null, ...props }) => (
  <div className={classList("plan row", className)}>
    <i className="privacy icon"/>
    { text }
    { description ? <small className={descriptionClassName}>{description}</small> : null }
  </div>
);
const AccountUpgradeButton = ({ label, url, className, ...props }) => (
  <ExternalLink className={classList("upgrade", className)} href={url}>{label}<i className="right chevron icon"/></ExternalLink>
);

const AccountUserPane = ({ account, ...props }) => {
  let upgradeURL = null, upgradeString = null;
  let expirationString = null, expirationClassName = null;

  let username, type, plan, expiration, renews = false;
  if (account.account) {
    username = account.account.email || '';
    type = account.account.type || 'free';
  }
  if (account.subscription) {
    plan = account.subscription.type || '';
    renews = !!account.subscription.renews;
    expiration = parseExpirationString(account.subscription.expiration);
  }

  let typeText = ACCOUNT_TYPE_NAMES[type];

  if (type === 'free') {
    upgradeString = "Upgrade Now";
    upgradeURL = '/account/upgrade';
  }
  if (expiration) {
    let now = new Date();
    if (expiration <= now) {
      expirationString = `Expired on ${expiration.getMonth()+1}/${expiration.getDate()}/${expiration.getFullYear()}`;
      expirationClassName = 'expired';
    } else {
      const limitDays = SUBSCRIPTION_SCHEDULE_DAYS[plan] > 30 ? 30 : 7;
      let diff = (expiration.getTime() - now.getTime()) / (24 * 60 * 60 * 1000);
      if (diff <= limitDays) {
        expirationString = (renews ? "Renews " : "Expires ") + describeRelativeDate(expiration, now);
      }
      if (!renews && diff <= 7) {
        expirationClassName = 'expiring';
      }
    }
    if (expirationString && !renews && !upgradeURL) {
      upgradeString = "Renew Now";
      upgradeURL = '/account/upgrade';
    }
  } else {
    expirationString = "Lifetime";
  }

  if (!expirationString && SUBSCRIPTION_SCHEDULE_NAMES.hasOwnProperty(plan)) {
    expirationString = SUBSCRIPTION_SCHEDULE_NAMES[plan];
  }

  /*** PREVIEW ONLY ***/
  if (type === 'free' && plan === 'preview') {
    typeText = "Preview Account";
    expirationString = "Launching soon!";
    upgradeURL = null;
  }

  if (upgradeURL) {
    upgradeURL = `${upgradeURL}?user=${encodeURIComponent(username)}&secret=${encodeURIComponent(account.secret)}`;
  }
  return (
    <div className="user pane" {...props}>
      <AccountIllustration src={type === 'free' ? AccountBanner.free : AccountBanner.premium}/>
      <AccountUserField text={username}/>
      <AccountPlanField
        className={ACCOUNT_TYPE_NAMES.hasOwnProperty(type) ? type : null}
        text={typeText}
        description={expirationString}
        descriptionClassName={expirationClassName}/>
      { upgradeURL ? <AccountUpgradeButton label={upgradeString} url={upgradeURL}/> : null }
    </div>
  );
};


export default class AccountScreen extends DaemonAware(React.Component) {
  state = {
    account: Object.assign({}, daemon.account)
  }
  daemonAccountChanged(account) {
    this.setState({ account: Object.assign({}, daemon.account) });
  }
  componentWillMount() {
    refreshAccountIfNeeded().catch(() => {});
  }
  render() {
    return(
      <Subpanel direction="left" id="account-screen">
        {this.props.children}
        <PanelContent key="self" className="account">
          <PanelTitlebar title="My Account" back="/main"/>
          <div className="scrollable content">
            <AccountUserPane account={this.state.account}/>
            <div className="pane" data-title="Account Settings">
              <div className="setting"><ExternalLink href="/account" params={{ user: this.state.account.account.email, secret: this.state.account.secret }}>Manage Account</ExternalLink></div>
            </div>
            <div className="pane" data-title="More">
              {/*<div className="setting"><ExternalLink href="/account/refer" params={{ user: this.state.account.account.email, secret: this.state.account.secret }}><div>Refer a Friend</div></ExternalLink></div>*/}
              <div className="setting"><ExternalLink className="bug" href="/support/request/new">Report an Issue</ExternalLink></div>
              <div className="setting"><ExternalLink href="/support">Go to Help Center</ExternalLink></div>
              <div className="setting"><Link className="logout" to="/login/logout" tabIndex="0" onClick={() => analytics.event('Account', 'logout')}>Sign Out</Link></div>
            </div>
            <div className="links footer">
              <ExternalLink href="/terms-of-service">Terms of Service</ExternalLink>
              <ExternalLink href="/privacy-policy">Privacy Policy</ExternalLink>
              <ExternalLink href="/legal/license/desktop">License Information</ExternalLink>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
