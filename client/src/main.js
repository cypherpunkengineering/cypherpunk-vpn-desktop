const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, nativeImage: NativeImage, ipcMain: ipc } = electron;
const fs = require('fs');
let daemon = require('./daemon.js');

let exiting = false;
let main = null;
let tray = null;
let os = ({ 'win32': '_win', 'darwin': '_osx', 'linux': '_lin' })[process.platform] || '';
let dpi = '';

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


function eventPromise(emitter, name) {
  return new Promise((resolve, reject) => {
    emitter.once(name, resolve);
  });
}

function timeoutPromise(promise, delay, timeoutIsSuccess = false) {
  return new Promise((resolve, reject) => {
    var timeout = setTimeout(() => { if (timeoutIsSuccess) resolve(); else reject(); }, delay);
    promise.then(val => { clearTimeout(timeout); resolve(val); }, reject);
  });
}

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
  return getResource(`assets/img/flags/${country.toLowerCase()}.png`);
}


function displayNotification(message) {
  if (daemon && !daemon.settings.showNotifications)
    return;
  if (os == '_osx' && main) {
    // Apparently only works in the renderer process via webkitNotifications?
    main.webContents.executeJavaScript(`
      new Notification("Cypherpunk Privacy", { body: ${JSON.stringify(message)} });
    `);
  } else if (os == '_win' && tray) {
    tray.displayBalloon({
      icon: undefined,
      title: "Cypherpunk Privacy",
      content: message,
    });
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
    d.disconnect().then(() => { app.quit(); });
    return;
  }
  if (tray) {
    tray.destroy();
  }
});

const preinitPromises = [
  eventPromise(app, 'ready'),
  eventPromise(daemon, 'up'),
  eventPromise(daemon, 'config'),
  eventPromise(daemon, 'account'),
  eventPromise(daemon, 'settings'),
  eventPromise(daemon, 'state'),
  /*
  connectToDaemon(9337).catch(err => {
    dialog.showMessageBox({
      type: 'warning',
      buttons: [ "Exit" ], // FIXLOC
      title: "Cypherpunk Privacy", // FIXLOC
      message: "Unable to communicate with helper service.\nPlease reinstall the application.", // FIXLOC
    });
    throw err;
  })
  */
];

timeoutPromise(Promise.all(preinitPromises), 2000).then(() => {
  dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
  createMainWindow();
  createTray();
  if (daemon.settings.autoConnect) {
    // FIXME: This should actually be in response to establishing a user login session
    daemon.post.connect();
  }
}).catch(err => {
  // FIXME: Message box with initialization error
  console.error(err);
  app.quit();
});

function createTrayMenu() {
  let server = daemon.config.servers[daemon.settings.server];
  let connected = daemon.state.state !== 'DISCONNECTED';
  let items = [
    { label: "Show", visible: !main || !main.isVisible(), click: () => { showMainWindow(); } },
    { type: 'separator', visible: !main || !main.isVisible() },
    { label: "Reconnect (apply changed settings)", visible: !!(connected && daemon.state.needsReconnect) },
    { label: "Connect", visible: !connected, click: () => { daemon.post.connect(); } },
    { label: "Disconnect", visible: connected, click: () => { daemon.post.disconnect(); } },
    { type: 'separator' },
    {
      label: (server ? server.regionName : "No region selected"),
      icon: server ? getFlag(server.country) : null,
      submenu: !connected ? Object.keys(daemon.config.servers).map(k => daemon.config.servers[k]).map(s => ({
        label: s.regionName,
        icon: getFlag(s.country.toLowerCase()),
        type: 'checkbox',
        checked: daemon.settings.server === s.id,
        enabled: s.ovDefault && s.ovDefault != "255.255.255.255",
        click: () => { if (!connected) daemon.post.applySettings({ server: s.id }); }
      })) : null,
      enabled: !connected,
    },
    { type: 'separator', visible: connected },
    { label: "Protocol: OpenVPN", enabled: false, visible: connected },
    { label: "IP: " + daemon.state.remoteIP, enabled: false, visible: connected }, // FIXME: not the right IP, you want the public one
    { label: "Status: " + daemon.state.state, enabled: false, visible: connected },
    { type: 'separator' },
    //{ label: 'Sign out' },
    { label: 'Quit', click: () => { app.quit(); } },
  ];
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
  daemon.on('config', config => { if (config.servers) refresh(); });
  daemon.on('state', state => { if (state.state || state.remoteIP) refresh(); });
  daemon.on('settings', settings => { if (settings.server) refresh(); });
  main.on('hide', () => { refresh(); });
  main.on('show', () => { refresh(); });
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
    main.on('ready-to-show', show);
    showTimeout = setTimeout(show, 500);
  }
  main.maximizedPrev = null;

  main.loadURL(`file://${__dirname}/web/index.html`);
  if (args.debug) {
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
