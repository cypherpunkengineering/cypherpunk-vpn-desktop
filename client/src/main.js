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


let AutoLaunch = new (require('auto-launch'))({
  name: "Cypherpunk VPN"
});

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
  return getResource(`assets/img/flags/${country}.png`);
}


function displayNotification(message) {
  if (os == '_osx' && main) {
    // Apparently only works in the renderer process via webkitNotifications?
    main.webContents.executeJavaScript(`
      new Notification("Cypherpunk VPN", { body: ${JSON.stringify(message)} });
    `);
  } else if (os == '_win' && tray) {
    tray.displayBalloon({
      icon: undefined,
      title: "Cypherpunk VPN",
      content: message,
    });
  }
}


app.on('will-quit', event => {
  if (!exiting && daemon) {
    exiting = true;
    event.preventDefault();
    daemon.disconnect().then(() => {
      daemon = null;
      app.quit();
    });
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
      title: "Cypherpunk VPN", // FIXLOC
      message: "Unable to communicate with helper service.\nPlease reinstall the application.", // FIXLOC
    });
    throw err;
  })
  */
];

timeoutPromise(Promise.all(preinitPromises), 2000).then(() => {
  dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
  createTray();
  createMainWindow();
}).catch(err => {
  // FIXME: Message box with initialization error
  console.error(err);
  app.quit();
});

function createTrayMenu() {
  let server = daemon.config.servers.find(s => s.id === daemon.settings.server);
  let connected = daemon.state.state !== 'DISCONNECTED';
  let items = [
    { label: "Reconnect (apply changed settings)", visible: !!(connected && daemon.state.needsReconnect) },
    { label: "Connect", visible: !connected, click: () => { daemon.post.connect(); } },
    { label: "Disconnect", visible: connected, click: () => { daemon.post.disconnect(); } },
    { type: 'separator' },
    {
      label: "Server: " + (server ? server.name : "None"),
      icon: server ? getFlag(server.country) : null,
      submenu: !connected ? daemon.config.servers.map(s => ({
        label: s.name,
        icon: getFlag(s.country),
        type: 'checkbox',
        checked: daemon.settings.server === s.id,
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
  return Menu.buildFromTemplate(items);
}

function createTray() {
  var trayIcon = NativeImage.createFromPath(getOSResource('assets/img/tray.png'));
  if (os == '_osx') {
    trayIcon.setTemplateImage(true);
  }
  tray = new Tray(trayIcon);
  tray.setToolTip('Cypherpunk VPN');
  function refresh() {
    tray.setContextMenu(createTrayMenu());
  }
  refresh();
  daemon.on('config', config => { if (config.servers) refresh(); });
  daemon.on('state', state => { if (state.state || state.remoteIP) refresh(); });
  daemon.on('settings', settings => { if (settings.server) refresh(); });
  tray.on('click', (evt, bounds) => {
    if (main) {
      main.show();
    }
  });
}

function createMainWindow() {
  main = new BrowserWindow({
    title: 'Cypherpunk VPN (Semantic UI)',
    //icon: icon,
    backgroundColor: '#1560bd',
    show: false,
    //frame: false,
    fullscreen: false,
    useContentSize: true,
    width: 350,
    height: 508,
    //resizable: false,
    minimizable: false,
    maximizable: false,
    minWidth: 350,
    minHeight: 508,
    maxWidth: 350,
    maxHeight: 508,
    //acceptFirstMouse: true,
  });
  if (daemon) {
    daemon.setMainWindow(main);
  }

  main.setMenu(null);
  main.on('close', () => {
    main.webContents.closeDevTools();
  })
  main.on('closed', () => {
    if (daemon) {
      daemon.setMainWindow(null);
    }
    main = null;
  })
  main.on('ready-to-show', function() {
    if (args.showWindowOnStart) {
      main.show();
    }
  });
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

  // However, the first time the window is closed (and we're not exiting),
  // we should display a desktop notification to remind the user that the
  // application is still running, at least on Windows since that's not
  // common to all applications.
  if (os == '_win') {
    displayNotification("Cypherpunk VPN is still running - control it from here.");
  }
});
