import daemon from './daemon';
import { makeQueryString } from './server';
import './util';

const electron = require('electron');

const trackingID = 'UA-80859092-2';
const userAgent = window.navigator.userAgent;
const debug = false; //electron.remote.getGlobal('args').debug;

let enabled = false;
let clientID = null;

let queue = [];

// useful parameters:
// qt: queue time, millisecond delay of submission (i.e. nowTimestamp - queuedTimestamp)
// uip: actual IP address (e.g. naked IP instead of VPN IP?)
// geoid: geographical location
// sr: screen resolution
// ni: non-interactive hit


function send(type, payload) {
  return new Promise(function xhrPromise(resolve, reject) {
    let xhr = new XMLHttpRequest();
    let url = 'https://www.google-analytics.com/collect';
    if (Array.isArray(payload)) { // batch
      if (payload.length > 20) {
        return reject(new Error("Too many payloads at once"));
      }
      payload = payload.map(p => makeQueryString(Object.assign({ t: type, v: 1, tid: trackingID, cid: clientID, ds: 'electron' }, p))).join('\r\n');
      url = 'https://www.google-analytics.com/batch';
    } else if (payload && typeof payload === 'object') {
      payload = makeQueryString(Object.assign({ t: type, v: 1, tid: trackingID, cid: clientID, ds: 'electron' }, payload));
    } else {
      return reject(new Error("Invalid payload"));
    }
    xhr.open('POST', url, true);
    xhr.onload = function() {
      if (this.status < 200 || this.status >= 300) {
        let err = new Error("Failed to send analytics");
        err.handled = true;
        err.status = this.status;
        reject(Object.assign(new Error("Failed to send analytics"), { handled: true, status: this.status }));
      } else {
        resolve(this.response);
      }
    };
    xhr.onerror = function() {
      reject(Object.assign(new Error("Failed to send analytics"), { handled: true }));
    };
    xhr.onabort = function() {
      reject(Object.assign(new Error("Failed to send analytics"), { handled: true }));
    };
    xhr.send(payload);
  });
}



let analytics = {
  activate() {
    if (!enabled) {
      daemon.post.applySettings({ enableAnalytics: true });
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

  pageview(hostname, path, title) {
    if (!enabled) return Promise.resolve();
    return send('pageview', { dh: hostname, dp: path, dt: title });
  },
  event(evCategory, evAction, { label, value } = {}) {
    if (!enabled) return Promise.resolve();
    return send('event', { ec: evCategory, ea: evAction, el: label, ev: value });
  },
  /*
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
  */
};

window.analytics = analytics;

export default analytics;
