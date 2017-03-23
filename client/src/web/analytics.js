import daemon from './daemon';
import { makeQueryString } from './server';
import './util';

const electron = require('electron');

const trackingID = 'UA-80859092-2';
const debug = electron.remote.getGlobal('args').debug;
const version = electron.remote.app.getVersion();

let enabled = false;
let clientID = null;

let queue = [];
let queueTimeout = null;
let queueShortTimeout = null;

const TIMEOUT = debug ? 10000 : 60000;
const SHORT_TIMEOUT = 1000;

// useful parameters:
// qt: queue time, millisecond delay of submission (i.e. nowTimestamp - queuedTimestamp)
// uip: actual IP address (e.g. naked IP instead of VPN IP?)
// geoid: geographical location
// sr: screen resolution
// ni: non-interactive hit


function processQueue() {
  queueTimeout = null;
  queueShortTimeout = null;

  let items = queue.slice(0, 20);
  queue = queue.slice(20);

  let now = new Date();
  fetch(items.length == 1 ? 'https://www.google-analytics.com/collect' : 'https://www.google-analytics.com/batch', {
    method: 'post',
    mode: 'no-cors',
    headers: {},
    body: items.map(i => makeQueryString(Object.assign(i.payload, { qt: (now - i.time) }))).join('\r\n')
  }).then(response => items.forEach(i => i.resolve(response)), error => items.forEach(i => i.reject(error)));

  if (queue.length >= 20) {
    queueShortTimeout = setTimeout(processQueue, SHORT_TIMEOUT);
  } else if (queue.length > 0) {
    queueTimeout = setTimeout(processQueue, TIMEOUT);
  }
}

function cancelQueue() {
  if (queueTimeout !== null) {
    cancelTimeout(queueTimeout);
    queueTimeout = null;
  }
  if (queueShortTimeout !== null) {
    cancelTimeout(queueShortTimeout);
    queueShortTimeout = null;
  }
  let items = queue;
  queue = [];
  let err = Object.assign(new Error("Analytics processing terminated"), { handled: true });
  items.forEach(i => i.reject(err));
}

function send(type, payload) {
  if (Array.isArray(payload)) {
    return Promise.all(payload.map(p => send(type, p)));
  } else if (payload && typeof payload === 'object') {
    return new Promise((resolve, reject) => {
      let props = { t: type, v: 1, tid: trackingID, cid: clientID, ds: 'app', an: 'CypherpunkPrivacy', av: version };
      let now = new Date();
      queue.push({ type, time: now, payload: Object.assign(props, payload), resolve, reject });
      if (queue.length == 20) {
        if (queueTimeout !== null) {
          cancelTimeout(queueTimeout);
          queueTimeout = null;
        }
        if (queueShortTimeout === null) {
          queueShortTimeout = setTimeout(processQueue, SHORT_TIMEOUT);
        }
      } else if (queueTimeout === null) {
        queueTimeout = setTimeout(processQueue, TIMEOUT)
      }
    });
  } else {
    return Promise.reject(new Error("Invalid analytics payload"));
  }
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
      cancelQueue();
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
