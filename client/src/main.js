const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, nativeImage: NativeImage, ipcMain: ipc } = electron;
const fs = require('fs');
import { eventPromise, timeoutPromise } from './util.js';
let daemon = null;

let exiting = false;
let main = null;
let tray = null;
let os = ({ 'win32': '_win', 'darwin': '_osx', 'linux': '_lin' })[process.platform] || '';
let dpi = '';
let location = {};
let loggedIn = false;

let args = {
  debug: false,
  showWindowOnStart: true,
};
process.argv.forEach(arg => {
  if (arg === "--debug") {
    args.debug = true;
  } else if (arg === '--background') {
    args.showWindowOnStart = false;
  }
});

const AutoLaunch = require('./autostart.js');

ipc.on('autostart-get', (event) => {
  AutoLaunch.isEnabled()
    .then(enabled => event.sender.send('autostart-value', enabled))
    .catch(error => {
      console.warning("Unable to read autostart configuration");
      event.sender.send('autostart-value', null);
    });
});

ipc.on('autostart-set', (event, enable) => {
  (enable ? AutoLaunch.enable() : AutoLaunch.disable())
    .then(result => event.sender.send('autostart-value', enable))
    .catch(error => {
      console.error("Unable to write autostart configuration");
      event.sender.send('autostart-value', null);
    })
});

ipc.on('navigate', (event, loc) => {
  location = loc;
  var l = loc.pathname.match(/^\/(?!login)./);
  if (l != loggedIn) {
    loggedIn = l;

  }
})



function connectToDaemon(port) {
  return new Promise((resolve, reject) => {
    daemon = new RPC({
      url: 'ws://127.0.0.1:' + port,
      onopen: evt => { resolve(daemon); },
      onerror: evt => { reject(evt); },
    });
  });
}

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


function displayNotification(message) {
  if (daemon && !daemon.settings.showNotifications)
    return;
  if (os == '_win' && tray) {
    tray.displayBalloon({
      icon: undefined, // FIXME
      title: "Cypherpunk Privacy",
      content: message,
    });
  } else if (main) {
    // Apparently only works in the renderer process via webkitNotifications?
    main.webContents.executeJavaScript(`
      new Notification("Cypherpunk Privacy", { body: ${JSON.stringify(message)} });
    `);
  }
}


ipc.on('close', event => {
  if (exiting) {
    event.sender.send('close');
  } else {
    // FIXME: If we have a dock/taskbar button, minimize.
    // if (...) { main.minimize(); } else
    // Otherwise, just hide the window.
    {
      main.hide();

      // However, the first time the window is "closed" (and we're not exiting),
      // we should display a desktop notification to remind the user that the
      // application is still running, at least on Windows since that's not
      // common to all applications.
      displayNotification("Cypherpunk Privacy will keep running in the background - control it from the system tray.");
    }
  }
});

app.on('before-quit', event => {
  exiting = true;
  if (main) {
    main.webContents.send('close');
  }
});

app.on('will-quit', event => {
  if (daemon) {
    event.preventDefault();
    let d = daemon;
    daemon = null;
    d.disconnect().then(() => { setTimeout(() => app.quit()); });
    return;
  }
  if (tray) {
    tray.destroy();
  }
});


eventPromise(app, 'ready').then(() => {
  dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
  //return require('./install.js').checkInstalled();
  return 'installed';
}).then(status => {
  if (status === 'installed') {
    daemon = require('./daemon.js');
    var next = timeoutPromise(Promise.all(['up', 'config', 'account', 'settings', 'state'].map(e => eventPromise(daemon, e))), 2000);
    return next;
  } else {
    console.log("Installation status: " + status);
    app.exit(0);
    throw null; // dirty way to halt rest of promise chain
  }
}).then(() => {
  createMainWindow();
  createTray();
}).catch(err => {
  if (err) {
    dialog.showErrorBox("Initialization Error", "An unexpected error happened while launching Cypherpunk Privacy:\n\n" + err);
    app.exit(1);
  } else {
    app.exit(0);
  }
});

/*
const preinitPromises = [
  eventPromise(app, 'ready'),
  eventPromise(daemon, 'up'),
  eventPromise(daemon, 'config'),
  eventPromise(daemon, 'account'),
  eventPromise(daemon, 'settings'),
  eventPromise(daemon, 'state'),
];

timeoutPromise(Promise.all(preinitPromises), 2000).then(() => {
  dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
  return require('./install.js').checkInstalled();
}).then(status => {
  if (status === 'installed') {
    createMainWindow();
    createTray();
  }
}).catch(err => {
  // FIXME: Message box with initialization error
  console.error(err);
  app.quit();
});
*/

function createTrayMenu() {
  const hasLocations = typeof daemon.config.locations === 'object' && Object.keys(daemon.config.locations).length > 0;
  const location = daemon.config.locations[daemon.settings.location];
  const state = daemon.state.state;
  const connected = state !== 'DISCONNECTED';
  let items = [];
  if (loggedIn) {
    /*if (!main || !main.isVisible())*/ {
      items.push(
        { label: "Show window", click: () => { showMainWindow(); }},
        { type: 'separator' }
      );
    }
    let connectName;
    switch (state) {
      case 'CONNECTING': connectName = "Connecting to " + location.name + "..."; break;
      case 'CONNECTED': if (daemon.state.needsReconnect) connectName = "Reconnect (apply changed settings)"; else connectName = "Connected to " + location.name; break;
      case 'DISCONNECTING': connectName = "Disconnecting..."; break;
      case 'DISCONNECTED': connectName = "Connect to " + location.name; break;
      case 'SWITCHING': connectName = "Switching to " + location.name; break;
    }
    items.push({
      label: connectName,
      icon: state === 'DISCONNECTING' ? null : getFlag(location.country.toLowerCase()),
      enabled: state === 'DISCONNECTED' || (state === 'CONNECTED' && daemon.state.needsReconnect),
      click: () => { daemon.post.connect(); }
    });
    items.push({
      label: "Disconnect",
      enabled: state === 'CONNECTING' || state === 'CONNECTED' || state === 'SWITCHING',
      click: () => { daemon.post.disconnect(); }
    });
    items.push({ type: 'separator' });
    items.push({
      label: state === 'DISCONNECTED' ? "Connect to" : "Switch to",
      enabled: hasLocations && (state === 'CONNECTED' || state === 'DISCONNECTED'),
      submenu:
        Object.keys(daemon.config.locations).map(k => daemon.config.locations[k]).map(s => ({
          label: s.name,
          icon: getFlag(s.country.toLowerCase()),
          type: 'checkbox',
          checked: daemon.settings.location === s.id,
          enabled: !s.disabled,
          click: () => {
            daemon.call.applySettings({ location: s.id })
              .then(() => {
                if (state === 'DISCONNECTED' || daemon.state.needsReconnect) {
                  daemon.post.connect();
                }
              });
          }
        }))
    });

    if (main) {
      items.push({ type: 'separator' });
      items.push({ label: "My Account", click: () => { main.webContents.send('navigate', { pathname: '/account' }); showMainWindow(); } });
      items.push({ label: "Configuration", click: () => { main.webContents.send('navigate', { pathname: '/configuration' }); showMainWindow(); } });
    }
  } else {
    items.push({ label: "Sign in", click: () => { showMainWindow(); }});
  }
  items.push(
    { type: 'separator' },
    { label: "Quit Cypherpunk Privacy", click: () => { app.quit(); } }
  );
  // Windows fix: hidden separators don't work, so manually strip out hidden items entirely
  items = items.filter(i => !i.hasOwnProperty('visible') || i.visible);
  return Menu.buildFromTemplate(items);
}

function createTray() {
  var trayIconConnected = NativeImage.createFromPath(getOSResource('assets/img/tray.png'));
  var trayIconDisconnected = NativeImage.createFromPath(getOSResource('assets/img/tray_disconnected.png'));
  if (os == '_osx') {
    trayIconConnected.setTemplateImage(true);
    trayIconDisconnected.setTemplateImage(true);
  }
  function getTrayIcon() { return daemon.state.state == 'CONNECTED' ? trayIconConnected : trayIconDisconnected; }
  tray = new Tray(getTrayIcon());
  tray.setToolTip('Cypherpunk Privacy');
  function refresh() {
    tray.setImage(getTrayIcon());
    tray.setContextMenu(createTrayMenu());
  }
  refresh();
  daemon.on('config', config => { if (config.locations) refresh(); });
  daemon.on('state', state => { if (state.state || state.needsReconnect) refresh(); });
  daemon.on('settings', settings => { if (settings.location) refresh(); });
  main.on('hide', () => { refresh(); });
  main.on('show', () => { refresh(); });
  var lastLoggedIn = undefined;
  ipc.on('navigate', (event, location) => { if (lastLoggedIn === undefined || loggedIn != lastLoggedIn) { lastLoggedIn = loggedIn; refresh(); }});
  if (os == '_win') {
    tray.on('click', (evt, bounds) => {
      showMainWindow();
    });
  }
}

function showMainWindow() {
  if (main) {
    main.show();
  } else {
    createMainWindow();
  }
}

function createMainWindow() {
  main = new BrowserWindow({
    title: 'Cypherpunk Privacy',
    //icon: icon,
    backgroundColor: '#1560bd',
    show: false,
    fullscreen: false,
    useContentSize: true,
    width: 350,
    height: 530,
    resizable: false,
    minimizable: false,
    maximizable: false,
    minWidth: 350,
    minHeight: 530,
    maxWidth: 350,
    maxHeight: 530,
    frame: false,
    titleBarStyle : 'hidden-inset'
    //skipTaskbar: true,
    //acceptFirstMouse: true,
  });
  if (daemon) {
    daemon.setMainWindow(main);
  }

  main.setMenu(null);
  main.on('close', () => {
    main.webContents.closeDevTools();
  });
  main.on('closed', () => {
    if (daemon) {
      daemon.setMainWindow(null);
    }
    main = null;
  });

  if (args.showWindowOnStart) {
    let showTimeout = null;
    let show = function() {
      main.show();
      args.showWindowOnStart = false;
      clearTimeout(showTimeout);
      showTimeout = null;
    };
    main.once('ready-to-show', show);
    showTimeout = setTimeout(show, 500);
  }
  main.maximizedPrev = null;

  main.loadURL(`file://${__dirname}/web/index.html`);
  if (args.debug) {
    main.webContents.on('devtools-opened', () => {
      main.show();
      main.focus();
    });
    main.webContents.openDevTools({ mode: 'detach' });
  }
};

app.on('window-all-closed', function() {
  // On OSX, an application stays alive even after all windows have been
  // closed (in the dock and/or tray), so don't exit.

  // On Windows, the taskbar button goes away if the window is closed,
  // and as a result we currently don't support taskbar-only mode, as
  // the application should keep running even with the Window closed.
});
