import Analytics from 'electron-google-analytics';
import daemon from './daemon';
import './util';

const electron = require('electron');

const trackingID = 'UA-80859092-2';
const userAgent = window.navigator.userAgent;
const debug = false; //electron.remote.getGlobal('args').debug;

let google = new Analytics(trackingID, { userAgent, debug, version: 1 });
let enabled = false;
let clientID = null;

let queue = [];

let analytics = {
  activate() {
    if (!enabled) {
      enabled = true;
      queue = [];
      if (daemon.settings.analyticsID) {
        clientID = daemon.settings.analyticsID;
      } else {
        clientID = Math.floor(Math.random() * (1e12 - 1) + 1);
        daemon.post.applySettings({ analyticsID: clientID });
      }
      console.log("Enabling Google Analytics with clientID " + clientID);
    }
  },
  deactivate() {
    if (enabled) {
      console.log("Disabling Google Analytics");
      daemon.post.applySettings({ enableAnalytics: false });
      enabled = false;
      clientID = null;
      queue = [];
    }
  },
  get enabled() {
    return enabled;
  },
  get clientID() {
    return clientID;
  },

  pageview(hostname, url, title) {
    if (!enabled) return Promise.resolve();
    return google.pageview(hostname, url, title).catch(e => { e.handled = true; throw e; });
  },
  event(evCategory, evAction, options = {}) {
    if (!enabled) return Promise.resolve();
    return google.event(evCategory, evAction, Object.assign({ clientID }, options)).catch(e => { e.handled = true; throw e; });
  },
  screen(appName, appVer, appID, appInstallerID, screenName) {
    if (!enabled) return Promise.resolve();
    return google.screen(appName, appVer, appID, appInstallerID, screenName, clientID).catch(e => { e.handled = true; throw e; });
  },
  transaction(trnID, options = {}) {
    if (!enabled) return Promise.resolve();
    return google.transaction(trnID, options, clientID).catch(e => { e.handled = true; throw e; });
  },
  social(socialAction, socialNetwork, socialTarget) {
    if (!enabled) return Promise.resolve();
    return google.social(socialActions, socialNetwork, socialTarget, clientID).catch(e => { e.handled = true; throw e; });
  },
  exception(exDesc, exFatal) {
    if (!enabled) return Promise.resolve();
    return google.exception(exDesc, exFatal, clientID).catch(e => { e.handled = true; throw e; });
  },
  refund(transactionID, evCategory = 'Ecommerce', evAction = 'Refund', nonInteraction = 1) {
    if (!enabled) return Promise.resolve();
    return google.refund(transactionID, evCategory, evAction, nonInteraction, clientID).catch(e => { e.handled = true; throw e; });
  },
  item(trnID, itemName, options = {}) {
    if (!enabled) return Promise.resolve();
    return google.item(trnID, itemName, options, clientID).catch(e => { e.handled = true; throw e; });
  },
  timingTrk(timingCtg, timingVar, timingTime, options = {}) {
    if (!enabled) return Promise.resolve();
    return google.timingTrk(timingCtg, timingVar, timingTime, options, clientID).catch(e => { e.handled = true; throw e; });
  },
  send(hitType, params) {
    if (!enabled) return Promise.resolve();
    return google.send(hitType, params, clientID).catch(e => { e.handled = true; throw e; });
  }
};

window.analytics = analytics;

export default analytics;
