import { app, dialog, BrowserWindow, Tray as ElectronTray, Menu, nativeImage as NativeImage, ipcMain as ipc } from 'electron';
import fs from 'fs';
import './util.js';

let os = ({ 'win32': '_win', 'darwin': '_osx', 'linux': '_lin' })[process.platform] || '';

function getOSResource(name) {
  name = `${__dirname}/${name}`;
  var os_name = name.replace(/\.[^.]*$/, os + '$&');
  if (os === '_win' && name.endsWith('.png')) {
    let ico = os_name.replace(/\.png$/, '.ico');
    if (fs.existsSync(ico)) return ico;
  }
  return fs.existsSync(os_name) ? os_name : name;
}
function getResource(name) {
  return `${__dirname}/${name}`;
}
function getFlag(country) {
  return getResource(`assets/img/flags/16/${country.toLowerCase()}.png`);
}


function navigateHandler(path) {
  return function() {
    if (window) {
      window.webContents.send('navigate', { pathname: path });
      window.show();
    }
  };
}
function reconnectHandler(settings) {
  return function() {
    daemon.call.applySettings(Object.assign(settings, { suppressReconnectWarning: true }))
      .then(() => {
        daemon.post.connect();
      });
  };
}


class Tray {
  electronTray = null;
  state = {
    config: { locations: null },
    settings: { location: null, locationFlag: null, overrideDNS: null, firewall: null },
    state: { connect: null, state: null, needsReconnect: null, pingStats: null },
    loggedIn: false,
    windowVisible: false,
  };
  constructor() {
    this.icons = {
      connecting: NativeImage.createFromPath(getOSResource('assets/img/tray_connecting.png')),
      connected: NativeImage.createFromPath(getOSResource('assets/img/tray_connected.png')),
      disconnected: NativeImage.createFromPath(getOSResource('assets/img/tray_disconnected.png')),
      killswitch: NativeImage.createFromPath(getOSResource('assets/img/tray_killswitch.png')),
      error: NativeImage.createFromPath(getOSResource('assets/img/tray_error.png')),
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
    let menuItems = this.createMenuItems();
    this.electronTray.setContextMenu(this.createTrayMenu(menuItems));
    if (process.platform === 'darwin') app.setApplicationMenu(this.createApplicationMenu(menuItems));
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
      let menuItems = this.createMenuItems();
      this.electronTray.setContextMenu(this.createTrayMenu(menuItems));
      if (process.platform === 'darwin') app.setApplicationMenu(this.createApplicationMenu(menuItems));
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
  getElectronTray() {
    return this.electronTray;
  }
  destroy() {
    if (this.electronTray) {
      this.electronTray.destroy();
      this.electronTray = null;
    }
  }
  getIcon() {
    // TODO: In case of any error, return this.icons.error
    switch (this.state.state.state)
    {
      case 'INTERRUPTED':
        return this.icons.error;
      case 'CONNECTING':
      case 'STILL_CONNECTING':
      case 'RECONNECTING':
      case 'STILL_RECONNECTING':
      case 'DISCONNECTING_TO_RECONNECT':
        return this.icons.connecting;
      case 'CONNECTED':
        return this.icons.connected;
      case 'DISCONNECTING':
      case 'DISCONNECTED':
        if (this.state.settings.firewall === 'on') return this.icons.killswitch;
        // fallthrough
      default:
        return this.icons.disconnected;
    }
  }
  getToolTip() {
    return "Cypherpunk Privacy";
  }
  createMenuItems() {
    let result = {};

    const hasLocations = typeof this.state.config.locations === 'object' && Object.keys(this.state.config.locations).length > 0;
    const location = hasLocations && this.state.config.locations[this.state.settings.location];
    const connectionState = this.state.state.state;
    const connected = connectionState !== 'DISCONNECTED';

    if (this.state.loggedIn) {
      let locationName = location ? location.name : "<unknown>";
      let locationFlag = (location && location.country) ? getFlag(location.country.toLowerCase()) : null;

      let connectEnabled = location && (connectionState === 'DISCONNECTED' || (connectionState === 'CONNECTED' && this.state.state.needsReconnect));
      let cypherplayLocation = null;
      const cypherplayName = "CypherPlay\u2122";
      if (hasLocations && this.state.state.pingStats) {
        cypherplayLocation = Object.keys(this.state.state.pingStats)
          .filter(s => this.state.config.locations[s] && this.state.config.locations[s].enabled && !this.state.config.locations[s].disabled && this.state.config.locations[s].region !== 'DEV')
          .reduce((min,s) => (this.state.state.pingStats[s] && this.state.state.pingStats[s].replies && (!min || this.state.state.pingStats[s].average < this.state.state.pingStats[min].average)) ? s : min, null);
      }
      if (this.state.settings.locationFlag === 'cypherplay') {
        locationName = cypherplayName;
        locationFlag = null;
        if (!cypherplayLocation) {
          locationName += " (calculating...)";
          connectEnabled = false;
        }
      }

      let connectName;
      switch (connectionState) {
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
      result.connect = {
        label: connectName,
        icon: connectEnabled ? locationFlag : null,
        enabled: connectEnabled,
        click: () => { daemon.post.connect(); }
      };
      result.disconnect = {
        label: "Disconnect",
        enabled: this.state.state.connect,
        click: () => { daemon.post.disconnect(); }
      };

      result.connectTo = {
        label: connectionState === 'DISCONNECTED' ? "Connect to" : "Switch to",
        enabled: hasLocations,
        submenu:
          [
            {
              label: cypherplayName,
              enabled: !!cypherplayLocation,
              click: cypherplayLocation ? reconnectHandler({ location: cypherplayLocation, locationFlag: 'cypherplay' }) : null
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
                click: reconnectHandler({ location: s.id, locationFlag: '', suppressReconnectWarning: true })
              }))
          )
      };
    }

    return result;
  }
  createApplicationMenu(items) {
    if (!items) items = this.createMenuItems();

    let template = [
      { label: "Cypherpunk Privacy", id: 'main', submenu: [
        { role: 'about' },
        { type: 'separator' },
        { label: this.state.loggedIn ? "Sign Out" : "Sign In", click: navigateHandler(this.state.loggedIn ? '/login/logout' : '/login/email') },
        { type: 'separator' },
        { label: 'Preferences', enabled: this.state.loggedIn, submenu: this.state.loggedIn ? [ { label: "My Account", accelerator: 'CommandOrControl+Alt+,', click: navigateHandler('/account') }, { label: "Configuration", accelerator: 'CommandOrControl+,', click: navigateHandler('/configuration') } ] : null },
        { type: 'separator' },
        { role: 'hide' },
        { role: 'hideothers' },
        { role: 'unhide' },
        { type: 'separator' },
        { role: 'quit' },
      ] },
      { role: 'editMenu' },
      { role: 'windowMenu' },
    ];

    if (this.state.loggedIn) {
      template.push({
        label: "Connection",
        position: 'after=main',
        submenu: [
          items.connect,
          { type: 'separator' },
          items.connectTo,
          { type: 'separator' },
          items.disconnect,
        ]
      })
    }

    return Menu.buildFromTemplate(template);
  }
  createTrayMenu(items) {
    if (!items) items = this.createMenuItems();

    let template = [];
    
    if (this.state.loggedIn) {
      template.push(
        { label: "Show window", click: () => { if (window) window.show(); }},
        { type: 'separator' },
        items.connect,
        { type: 'separator' },
        items.connectTo,
        { type: 'separator' },
        items.disconnect,
        { type: 'separator' },
        { label: "My Account", click: navigateHandler('/account') },
        { label: "Configuration", click: navigateHandler('/configuration') },
      );
    } else {
      template.push(
        { label: "Sign In", click: navigateHandler('/login/email') },
      );
    }
    template.push(
      { type: 'separator' },
      { role: 'quit' },
    );

    return Menu.buildFromTemplate(template);
  }
}

let tray = new Tray();
export default tray;
