const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, ipcMain: ipc } = electron;
let daemon = require('./daemon.js');

let exiting = false;
let main = null;
let tray = null;

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
  createTray();
  createMainWindow();
}).catch(err => {
  app.quit();
});

function createTray() {
  var dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
  tray = new Tray(`${__dirname}/assets/img/tray_win.png`);
  function flag(code, name, checked) {
    return {
      label: name,
      icon: `${__dirname}/assets/img/flags/${code}${dpi}.png`,
      type: 'checkbox',
      checked: checked
    };
  }
  const menu = Menu.buildFromTemplate([
    { label: 'Connect' },
    { type: 'separator' },
    {
      label: 'Location: USA East',
      icon: `${__dirname}/assets/img/flags/us${dpi}.png`,
      submenu: [
        flag('fr', 'France'),
        flag('hk', 'Hong Kong'),
        flag('us', 'USA East', true),
        flag('us', 'USA West'),
        flag('gb', 'United Kingdom'),
        flag('au', 'Australia'),
      ]
    },
    { type: 'separator' },
    { label: 'Protocol: IKEv2', enabled: false },
    { label: 'IP: 43.233.13.210', enabled: false },
    { label: 'Status: Disconnected', enabled: false },
    { type: 'separator' },
    { label: 'Sign out' },
    { label: 'Quit', click: () => { app.quit(); } },
  ]);
  tray.setToolTip('Cypherpunk VPN');
  tray.setContextMenu(menu);
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
    width: 375,
    height: 622,
    //resizable: false,
    minimizable: false,
    maximizable: false,
    minWidth: 375,
    minHeight: 622,
    maxWidth: 375,
    maxHeight: 622,
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
