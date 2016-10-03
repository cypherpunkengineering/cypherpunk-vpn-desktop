const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, ipcMain: ipc } = electron;
let daemon = require('./daemon.js');

let exiting = false;
let main = null;
let tray = null;
let dpi = '';

let args = {
  debug: false,
  showWindowOnStart: true,
};
var noMain = false;
process.argv.forEach(arg => {
  if (arg === "--debug") {
    args.debug = true;
  } else if (arg === '--background') {
    args.showWindowOnStart = false;
  }
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

ipc.on('daemon', (event, arg) => {

});

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
      icon: server ? `${__dirname}/assets/img/flags/${server.country}${dpi}.png` : null,
      submenu: !connected ? daemon.config.servers.map(s => ({
        label: s.name,
        icon: `${__dirname}/assets/img/flags/${s.country}${dpi}.png`,
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
  tray = new Tray(`${__dirname}/assets/img/tray_win.png`);
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
    backgroundColor: '#222',
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
  app.quit();
});
