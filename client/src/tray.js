import { app, dialog, BrowserWindow, Tray as ElectronTray, Menu, nativeImage as NativeImage, ipcMain as ipc } from 'electron';
import fs from 'fs';
import daemon from './daemon.js';
import './util.js';

let os = ({ 'win32': '_win', 'darwin': '_osx', 'linux': '_lin' })[process.platform] || '';

function getOSResource(name) {
  name = `${__dirname}/${name}`;
  var os_name = name.replace(/\.[^.]*$/, os + '$&');
  return fs.existsSync(os_name) ? os_name : name;
}
function getResource(name) {
  return `${__dirname}/${name}`;
}
function getFlag(country) {
  return getResource(`assets/img/flags/16/${country.toLowerCase()}.png`);
}

class Tray {
  electronTray = null;
  state = {
    config: { locations: null },
    settings: { location: null, locationFlag: null, overrideDNS: null },
    state: { connect: null, state: null, needsReconnect: null, pingStats: null },
    loggedIn: false,
    windowVisible: false,
  };
  constructor() {
    this.icons = {
      connected: NativeImage.createFromPath(getOSResource('assets/img/tray.png')),
      disconnected: NativeImage.createFromPath(getOSResource('assets/img/tray_disconnected.png')),
    };
    if (process.platform === 'darwin') {
      Object.keys(this.icons).forEach(k => this.icons[k].setTemplateImage(true));
    }
  }
  create() {
    if (this.electronTray) {
      this.destroy();
    }
    // Fetch current daemon properties
    ['config', 'settings', 'state'].forEach(k => { this.state[k] = Object.filter(daemon[k], this.state[k]); });
    // Create elements
    this.electronTray = new ElectronTray(this.getIcon());
    this.electronTray.setToolTip(this.getToolTip());
    this.electronTray.setContextMenu(this.createMenu());
    // Set up handlers
    if (process.platform === 'win32') {
      this.electronTray.on('click', (evt, bounds) => window && window.show());
    }
    // Set up listeners for when to refresh state
    ['config', 'settings', 'state'].forEach(k => daemon.on(k, s => this.updateState(k, s)));
    app.on('account-changed', account => this.setState({ loggedIn: account !== null }));
    window.on('show', () => this.setState({ windowVisible: true }));
    window.on('hide', () => this.setState({ windowVisible: false }));
  }
  refresh() {
    if (this.electronTray) {
      this.electronTray.setImage(this.getIcon());
      this.electronTray.setToolTip(this.getToolTip());
      this.electronTray.setContextMenu(this.createMenu());
    }
  }
  updateState(name, params) {
    var changed = false;
    var obj = this.state[name];
    Object.keys(params).forEach(k => {
      if (obj.hasOwnProperty(k) && obj[k] !== params[k]) {
        obj[k] = params[k];
        changed = true;
      }
    });
    if (changed) this.refresh();
  }
  setState(state) {
    var changed = false;
    Object.keys(state).forEach(k => {
      if (this.state[k] !== state[k]) {
        this.state[k] = state[k];
        changed = true;
      }
    });
    if (changed) this.refresh();
  }
  destroy() {
    if (this.electronTray) {
      this.electronTray.destroy();
      this.electronTray = null;
    }
  }
  getIcon() {
    return this.state.state.state == 'CONNECTED' ? this.icons.connected : this.icons.disconnected;
  }
  getToolTip() {
    return "Cypherpunk Privacy";
  }
  createMenu() {
    const hasLocations = typeof this.state.config.locations === 'object' && Object.keys(this.state.config.locations).length > 0;
    const location = hasLocations && this.state.config.locations[this.state.settings.location];
    const state = this.state.state.state;
    const connected = state !== 'DISCONNECTED';
    let items = [];
    if (this.state.loggedIn) {
      /*if (!window || !window.isVisible())*/ {
        items.push(
          { label: "Show window", click: () => { if (window) window.show(); }},
          { type: 'separator' }
        );
      }
      let locationName = location ? location.name : "<unknown>";
      let locationFlag = (location && location.country) ? getFlag(location.country.toLowerCase()) : null;
      if (this.state.settings.locationFlag === 'cypherplay') {
        locationName = "CypherPlay";
        locationFlag = null;
      }
      let connectName;
      switch (state) {
        case 'CONNECTING':
        case 'STILL_CONNECTING':
          connectName = "Connecting to " + locationName + "...";
          break;
        case 'INTERRUPTED':
        case 'RECONNECTING':
        case 'STILL_RECONNECTING':
        case 'DISCONNECTING_TO_RECONNECT':
          connectName = "Reconnecting to " + locationName + "...";
          break;
        case 'CONNECTED':
          if (this.state.state.needsReconnect)
            connectName = "Reconnect (apply changed settings)";
          else
            connectName = "Connected to " + locationName;
          break;
        case 'DISCONNECTING':
          connectName = "Disconnecting...";
          break;
        default:
        case 'DISCONNECTED':
          connectName = "Connect to " + locationName;
          break;
      }
      // FIXME: Enable/disable these based on this.state.state.connect
      items.push({
        label: connectName,
        icon: (state === 'DISCONNECTING') ? null : locationFlag,
        enabled: location && (state === 'DISCONNECTED' || (state === 'CONNECTED' && this.state.state.needsReconnect)),
        click: () => { daemon.post.connect(); }
      });
      items.push({
        label: "Disconnect",
        enabled: state === 'CONNECTING' || state === 'CONNECTED' || state === 'SWITCHING',
        click: () => { daemon.post.disconnect(); }
      });
      items.push({ type: 'separator' });
      let cypherplayLocation;
      if (hasLocations && this.state.state.pingStats && this.state.settings.overrideDNS) {
        cypherplayLocation = Object.keys(this.state.state.pingStats).filter(s => this.state.config.locations[s] && this.state.config.locations[s].region !== 'DEV').reduce((min,s) => (this.state.state.pingStats[s] && this.state.state.pingStats[s].replies && (!min || this.state.state.pingStats[s].average < this.state.state.pingStats[min].average)) ? s : min, null);
      }
      items.push({
        label: state === 'DISCONNECTED' ? "Connect to" : "Switch to",
        enabled: hasLocations && (state === 'CONNECTED' || state === 'DISCONNECTED'),
        submenu:
          [
            {
              label: "CypherPlay",
              enabled: !!cypherplayLocation,
              click: cypherplayLocation ? () => {
                daemon.call.applySettings({ location: cypherplayLocation, locationFlag: 'cypherplay' })
                  .then(() => {
                    daemon.post.connect();
                  });
              } : null
            },
            { type: 'separator' }
          ].concat(
            Object.values(this.state.config.locations)
              .filter(s => !s.disabled)
              .sort((a, b) => a.name.localeCompare(b.name))
              .map(s => ({
                label: s.name,
                icon: getFlag(s.country.toLowerCase()),
                //type: 'checkbox',
                //checked: this.state.settings.location === s.id,
                //enabled: !s.disabled,
                click: () => {
                  daemon.call.applySettings({ location: s.id, locationFlag: '', suppressReconnectWarning: true })
                    .then(() => {
                      if (state === 'DISCONNECTED' || this.state.state.needsReconnect) {
                        daemon.post.connect();
                      }
                    });
                }
              }))
          )
      });

      if (window) {
        items.push({ type: 'separator' });
        items.push({ label: "My Account", click: () => { if (window) { window.webContents.send('navigate', { pathname: '/account' }); window.show(); } }});
        items.push({ label: "Configuration", click: () => { if (window) { window.webContents.send('navigate', { pathname: '/configuration' }); window.show(); } }});
      }
    } else {
      items.push({ label: "Sign in", click: () => { if (window) window.show(); }});
    }
    items.push(
      { type: 'separator' },
      { label: "Quit Cypherpunk Privacy", click: () => { app.quit(); } }
    );
    // Windows fix: hidden separators don't work, so manually strip out hidden items entirely
    items = items.filter(i => !i.hasOwnProperty('visible') || i.visible);
    return Menu.buildFromTemplate(items);
  }
}

let tray = new Tray();
export default tray;
