import daemon from './daemon';
import { makeQueryString } from './server';
import './util';

const electron = require('electron');

const trackingID = 'UA-80859092-3';
const debug = electron.remote.getGlobal('args').debug;
const version = electron.remote.app.getVersion();
const language = navigator.language.toLowerCase();

let enabled = false;
let clientID = null;

let queue = [];
let queueTimeout = null;
let queueShortTimeout = null;

const TIMEOUT = debug ? 1000 : 60000;
const SHORT_TIMEOUT = 1000;

// useful parameters:
// qt: queue time, millisecond delay of submission (i.e. nowTimestamp - queuedTimestamp)
// uip: actual IP address (e.g. naked IP instead of VPN IP?)
// geoid: geographical location
// ul: user language
// sr: screen resolution
// ni: non-interactive hit


function makeClientID() {
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
    return v.toString(16);
  });
}

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
  }).then(response => items.forEach(i => i.resolve(response)), error => { error.handled = true; items.forEach(i => i.reject(error)); });
  // FIXME: Add retries for failed requests
  // FIXME: Delay requests if we know there's no internet connection (killswitch)

  if (queue.length >= 20) {
    queueShortTimeout = setTimeout(processQueue, SHORT_TIMEOUT);
  } else if (queue.length > 0) {
    queueTimeout = setTimeout(processQueue, TIMEOUT);
  }
}

function cancelQueue() {
  if (queueTimeout !== null) {
    clearTimeout(queueTimeout);
    queueTimeout = null;
  }
  if (queueShortTimeout !== null) {
    clearTimeout(queueShortTimeout);
    queueShortTimeout = null;
  }
  let items = queue;
  queue = [];
  let err = Object.assign(new Error("Analytics processing terminated"), { handled: true });
  items.forEach(i => i.reject(err));
}

function sendInternal(type, payload) {
  if (Array.isArray(payload)) {
    return Promise.all(payload.map(p => sendInternal(type, p)));
  } else if (payload && typeof payload === 'object') {
    return new Promise((resolve, reject) => {
      let props = { t: type, v: 1, tid: trackingID, cid: clientID, ds: 'app', an: 'Cypherpunk Privacy', av: version, ul: language };
      let uip = Application.ownIP;
      if (uip) {
        props.uip = uip;
      }
      let now = new Date();
      queue.push({ type, time: now, payload: Object.assign(props, payload), resolve, reject });
      if (queue.length == 20) {
        if (queueTimeout !== null) {
          clearTimeout(queueTimeout);
          queueTimeout = null;
        }
        if (queueShortTimeout === null) {
          queueShortTimeout = setTimeout(processQueue, SHORT_TIMEOUT);
        }
      } else if (queueShortTimeout === null && queueTimeout === null) {
        queueTimeout = setTimeout(processQueue, TIMEOUT)
      }
    });
  } else {
    return Promise.reject(new Error("Invalid analytics payload"));
  }
}

function send(type, payload) {
  return sendInternal(type, payload).catch(e => undefined);
}



let analytics = {
  activate() {
    if (!enabled) {
      daemon.post.applySettings({ enableAnalytics: true });
      enabled = true;
      queue = [];
      if (daemon.settings.analyticsID && typeof daemon.settings.analyticsID === 'string' && daemon.settings.analyticsID.length == 36) {
        clientID = daemon.settings.analyticsID;
      } else {
        clientID = makeClientID();
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
    return send('pageview', { dh: hostname, dp: path, dt: title || undefined });
  },
  screenview(name) {
    if (!enabled) return Promise.resolve();
    return send('screenview', { cd: name });
  },
  event(evCategory, evAction, { label, value } = {}) {
    if (!enabled) return Promise.resolve();
    return send('event', { ec: evCategory, ea: evAction, el: label, ev: value });
  },
  timing(category, variable, time, { label, loadTime, dnsTime, downloadTime, redirectTime, tcpTime, responseTime, domTime, contentTime } = {}) {
    if (!enabled) return Promise.resolve();
    return send('timing', { utc: category, utv: variable, utt: time, utl: label, plt: loadTime, dns: dnsTime, pdt: downloadTime, rrt: redirectTime, tcp: tcpTime, srt: responseTime, dit: domTime, clt: contentTime });
  },
  /*
  transaction(trnID, options = {}) {
    if (!enabled) return Promise.resolve();
    return google.transaction(trnID, options, clientID).catch(e => { e.handled = true; throw e; });
  },
  social(socialAction, socialNetwork, socialTarget) {
    if (!enabled) return Promise.resolve();
    return google.social(socialActions, socialNetwork, socialTarget, clientID).catch(e => { e.handled = true; throw e; });
  },
  */
  exception(exDesc, exFatal) {
    if (!enabled) return Promise.resolve();
    return send('exception', { exd: exDesc, exf: exFatal ? 1 : 0 });
  },
  /*
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
