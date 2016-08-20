const { app, dialog, BrowserWindow, Tray, Menu, ipcMain: ipc } = require('electron');
const RPC = require('./js/rpc.js');

const useSemanticUi = (process.argv[process.argv.length - 1] == 'semantic');

let exiting = false;
let daemon = null;
let main = null;
let tray = null;

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

//var app = require('electron');
var path = require('path');
var fs = require('fs');
var request = require('request');
//var dialog = require('dialog');
//var BrowserWindow = require('browser-window');
//var Tray = require('tray');
//var Menu = require('menu');
//var constants = require('./js/constants.js');
//var events = require('./js/events.js');
//var profile = require('./js/profile.js');
//var service = require('./js/service.js');
//var errors = require('./js/errors.js');
//var logger = require('./js/logger.js');

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
  if (useSemanticUi) {
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
      height: 606,
      resizable: false,
      //'min-width': 300,
      //'min-height': 400,
      //'max-width': 600,
      //'max-height': 700
    });
  }

  main.setMenu(null);
  main.on('ready-to-show', function() {
    main.show();
  });
  main.maximizedPrev = null;

  if (useSemanticUi) {
    main.loadURL(`file://${__dirname}/index-semantic.html`);
  } else {
    main.loadURL(`file://${__dirname}/index.html`);
  }
  main.webContents.openDevTools({ mode: 'undocked' });
};

app.on('window-all-closed', function() {
  app.quit();
});


/*
var main = null;
var tray = null;

if (app.dock) {
  app.dock.hide();
}

var connTray;
var disconnTray;
if (process.platform === 'darwin') {
  connTray = path.join(__dirname, 'img',
    'tray_connected_osxTemplate.png');
  disconnTray = path.join(__dirname, 'img',
    'tray_disconnected_osxTemplate.png');
} else if (process.platform === 'win32') {
  connTray = path.join(__dirname, 'img',
    'tray_connected_win.png');
  disconnTray = path.join(__dirname, 'img',
    'tray_disconnected_win.png');
} else if (process.platform === 'linux') {
  connTray = path.join(__dirname, 'img',
    'tray_connected_linux_light.png');
  disconnTray = path.join(__dirname, 'img',
    'tray_disconnected_linux_light.png');
} else {
  connTray = path.join(__dirname, 'img',
    'tray_connected.png');
  disconnTray = path.join(__dirname, 'img',
    'tray_disconnected.png');
}
var icon = path.join(__dirname, 'img', 'logo.png');

var checkService = function(callback) {
  service.ping(function(status) {
    if (!status) {
      var timeout;

      if (callback) {
        timeout = 1000;
      } else {
        timeout = 8000;
      }

      setTimeout(function() {
        service.ping(function(status) {
          if (!status) {
            tray.setImage(disconnTray);
            dialog.showMessageBox(null, {
              type: 'warning',
              buttons: ['Ok'],
              //icon: icon,
              title: 'Pritunl - Service Error',
              message: 'Unable to communicate with helper service, ' +
                'try restarting'
            });
          }

          if (callback) {
            callback(status);
          }
        });
      }, timeout);
    } else {
      if (callback) {
        callback(true);
      }
    }
  });
};

app.on('window-all-closed', function() {
  if (app.dock) {
    app.dock.hide();
  }
  checkService();
});

app.on('open-file', function() {
  openMainWin();
});

app.on('open-url', function() {
  openMainWin();
});

app.on('activate-with-no-open-windows', function() {
  openMainWin();
});

var openMainWin = function() {
  if (main) {
    main.focus();
    return;
  }

  checkService(function(status) {
    if (!status) {
      return;
    }

    if (false) {
      main = new BrowserWindow({
        title: 'Pritunl',
        icon: icon,
        frame: false,
        fullscreen: false,
        width: 420,
        height: 561,
        'min-width': 325,
        'min-height': 225,
        'max-width': 650,
        'max-height': 790
      });
      main.maximizedPrev = null;

      main.loadUrl('file://' + path.join(__dirname, 'index.html'));
    } else {
      main = new BrowserWindow({
        title: 'CypherVPN',
        icon: icon,
        frame: false,
        fullscreen: false,
        width: 300,
        height: 500,
        'min-width': 300,
        'min-height': 400,
        'max-width': 600,
        'max-height': 700
      });
      main.maximizedPrev = null;

      main.loadUrl('file://' + path.join(__dirname, 'index2.html'));

      //main.webContents.openDevTools({ mode: 'undocked' });
    }

    main.on('closed', function() {
      main = null;
    });

    if (app.dock) {
      app.dock.show();
    }
  });
};

var sync =  function() {
  request.get({
    url: 'http://' + constants.serviceHost + '/status'
  }, function(err, resp, body) {
    if (!body || !tray) {
      return;
    }

    try {
      var data = JSON.parse(body);
    } catch (e) {
      err = new errors.ParseError(
        'main: Failed to parse service status (%s)', e);
      logger.error(err);
      tray.setImage(disconnTray);
      return;
    }

    if (data.status) {
      tray.setImage(connTray);
    } else {
      tray.setImage(disconnTray);
    }
  });
};

app.on('ready', function() {
  service.wakeup(function(status) {
    if (status) {
      app.quit();
      return;
    }

    var profilesPth = path.join(app.getPath('userData'), 'profiles');
    fs.exists(profilesPth, function(exists) {
      if (!exists) {
        fs.mkdir(profilesPth);
      }
    });

    events.subscribe(function(evt) {
      if (evt.type === 'output') {
        var pth = path.join(app.getPath('userData'), 'profiles',
          evt.data.id + '.log');

        fs.appendFile(pth, evt.data.output + '\n', function(err) {
          if (err) {
            err = new errors.ParseError(
              'main: Failed to append profile output (%s)', err);
            logger.error(err);
          }
        });
      } else if (evt.type === 'connected') {
        if (tray) {
          tray.setImage(connTray);
        }
      } else if (evt.type === 'disconnected') {
        if (tray) {
          tray.setImage(disconnTray);
        }
      } else if (evt.type === 'wakeup') {
        openMainWin();
      }
    });

    var noMain = false;
    process.argv.forEach(function(val) {
      if (val === "--no-main") {
        noMain = true;
      }
    });

    if (!noMain) {
      openMainWin();
    }

    tray = new Tray(disconnTray);
    tray.on('clicked', function() {
      openMainWin();
    });
    tray.on('double-clicked', function() {
      openMainWin();
    });

    var trayMenu = Menu.buildFromTemplate([
      {
        label: 'Settings',
        click: function() {
          openMainWin();
        }
      },
      {
        label: 'Exit',
        click: function() {
          request.post({
            url: 'http://' + constants.serviceHost + '/stop'
          }, function() {
            app.quit();
          });
        }
      }
    ]);
    tray.setContextMenu(trayMenu);

    var appMenu = Menu.buildFromTemplate([
      {
        label: 'Pritunl',
        submenu: [
          {
            label: 'Quit',
            accelerator: 'CmdOrCtrl+Q',
            role: 'close'
          }
        ]
      },
      {
        label: 'Edit',
        submenu: [
          {
            label: 'Undo',
            accelerator: 'CmdOrCtrl+Z',
            role: 'undo'
          },
          {
            label: 'Redo',
            accelerator: 'Shift+CmdOrCtrl+Z',
            role: 'redo'
          },
          {
            type: 'separator'
          },
          {
            label: 'Cut',
            accelerator: 'CmdOrCtrl+X',
            role: 'cut'
          },
          {
            label: 'Copy',
            accelerator: 'CmdOrCtrl+C',
            role: 'copy'
          },
          {
            label: 'Paste',
            accelerator: 'CmdOrCtrl+V',
            role: 'paste'
          },
          {
            label: 'Select All',
            accelerator: 'CmdOrCtrl+A',
            role: 'selectall'
          }
        ]
      }
    ]);
    Menu.setApplicationMenu(appMenu);

    profile.getProfiles(function(err, prfls) {
      if (err) {
        return;
      }

      var prfl;
      for (var i = 0; i < prfls.length; i++) {
        prfl = prfls[i];

        if (prfl.autostart) {
          prfl.connect();
        }
      }
    }, true);

    sync();
    setInterval(function() {
      sync();
    }, 10000);
  });
});
*/
