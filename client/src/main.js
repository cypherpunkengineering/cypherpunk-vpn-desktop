const { app, dialog, BrowserWindow, Tray, Menu, ipcMain: ipc } = require('electron');
const RPC = require('./rpc.js');
RPC.WebSocket = require('ws');

let exiting = false;
let daemon = null;
let main = null;
let tray = null;

let args = {
  debug: false,
  showWindowOnStart: true,
  uiVariant: '',
};
var noMain = false;
process.argv.forEach(arg => {
  if (arg === "--debug") {
    args.debug = true;
  } else if (arg === '--background') {
    args.showWindowOnStart = false;
  } else if (arg === '--normal') {
    args.uiVariant = '';
  } else if (arg === '--semantic') {
    args.uiVariant = 'semantic';
  } else if (arg === '--webpack') {
    args.uiVariant = 'webpack';
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
    timeoutPromise(daemon.disconnect(), 2000, true).then(() => {
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
  //createTray();
  createMainWindow();
}).catch(err => {
  app.quit();
});

function createTray() {
  tray = new Tray('app/img/tray_win.png');
  function flag(code, name, checked) {
    return {
      label: name,
      icon: 'app/img/flags/png16/' + code + '.png',
      type: 'checkbox',
      checked: checked
    };
  }
  const menu = Menu.buildFromTemplate([
    { label: 'Connect' },
    { type: 'separator' },
    {
      label: 'Location: Germany',
      icon: 'app/img/flags/png16/de.png',
      submenu: [
        flag('de', 'Germany', true),
        flag('fr', 'France'),
        flag('hk', 'Hong Kong'),
        flag('us', 'USA East'),
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
  if (args.uiVariant === 'semantic') {
    main = new BrowserWindow({
      title: 'Cypherpunk VPN (Semantic UI)',
      //icon: icon,
      backgroundColor: '#222',
      show: false,
      //frame: false,
      fullscreen: false,
      useContentSize: true,
      width: 300,
      height: 500,
      //resizable: false,
      minimizable: false,
      maximizable: false,
      minWidth: 300,
      minHeight: 400,
      maxWidth: 300,
      //maxHeight: 700,
      acceptFirstMouse: true,
    });
  } else {
    main = new BrowserWindow({
      title: 'Cypherpunk VPN',
      //icon: icon,
      backgroundColor: '#222',
      show: false,
      frame: false,
      fullscreen: false,
      width: 375,
      height: 590,
      resizable: false,
      //'min-width': 300,
      //'min-height': 400,
      //'max-width': 600,
      //'max-height': 700
    });
  }

  main.setMenu(null);
  main.on('close', () => {
    main.webContents.closeDevTools();
  })
  main.on('closed', () => {
    main = null;
  })
  main.on('ready-to-show', function() {
    if (args.showWindowOnStart) {
      main.show();
    }
  });
  main.maximizedPrev = null;

  if (args.uiVariant !== '') {
    main.loadURL(`file://${__dirname}/web/index.html?${args.uiVariant}`);
  } else {
    main.loadURL(`file://${__dirname}/web/index.html`);    
  }
  if (args.debug) {
    main.webContents.openDevTools({ mode: 'detach' });
  }
};

app.on('window-all-closed', function() {
  app.quit();
});
