const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, nativeImage: NativeImage, ipcMain: ipc } = electron;
const fs = require('fs');
import { eventPromise, timeoutPromise } from './util.js';
import Notification from './notification.js';

// Define global objects (easy to access from browser process too)
global.daemon = null;
global.window = null;
global.tray = require('./tray.js');
global.location = {};
global.loggedIn = false;
global.exiting = false;

global.args = {
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

// Expose the autostart setting to the window
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

// Catch navigation events from the window and use it to figure out login status
// TODO: Should probably watch daemon.account.account.secret or something instead
ipc.on('navigate', (event, loc) => {
  location = loc;
  var l = loc.pathname.match(/^\/(?!login)./);
  if (l != loggedIn) {
    loggedIn = l;
    if (l) {
      app.emit('account-changed', daemon.account);
    } else {
      app.emit('account-changed', null);
    }
  }
})

// This event is received when the user attempts to close the window
ipc.on('close', event => {
  if (exiting) {
    // Respond by telling the window to actually close.
    event.sender.send('close');
  } else {
    // FIXME: If we have a dock/taskbar button, minimize.
    // if (...) { window.minimize(); } else
    // Otherwise, just hide the window.
    {
      window.hide();

      // However, the first time the window is "closed" (and we're not exiting),
      // we should display a desktop notification to remind the user that the
      // application is still running, at least on Windows since that's not
      // common to all applications.
      new Notification({ body: "Cypherpunk Privacy will keep running in the background - control it from the system tray." });
    }
  }
});

// This event is received when someone is attempting to kill the main process.
app.on('before-quit', event => {
  exiting = true;
  // Tell the window to close.
  if (window) window.webContents.send('close');
});

// This event is received at the start of the main process quit sequence.
app.on('will-quit', event => {
  if (daemon) {
    // Cancel the event since we need an asynchronous quit sequence.
    event.preventDefault();
    let d = daemon;
    daemon = null;
    // Disconnect from the daemon and then quit again (setImmediate is
    // needed since app.quit() won't work in the same callstack).
    d.disconnect().then(() => { setImmediate(() => app.quit()); });
    return;
  }
  if (tray) {
    tray.destroy();
  }
});


eventPromise(app, 'ready').then(() => {
  //dpi = electron.screen.getPrimaryDisplay().scaleFactor >= 2 ? '@2x' : '';
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
  // Trigger account-changed events if the daemon.account property is touched
  daemon.on('account', account => {
    if (loggedIn) {
      // ...but only trigger if it looks like we actually have a full set of account data
      if (daemon.account.account && daemon.account.account.confirmed && daemon.account.privacy) {
        app.emit('account-changed', daemon.account);
      } else {
        app.emit('account-changed', null);
      }
    }
  });
  createMainWindow();
  tray.create();
}).catch(err => {
  if (err) {
    dialog.showErrorBox("Initialization Error", "An unexpected error happened while launching Cypherpunk Privacy:\n\n" + (err.stack ? err.stack : err));
    app.exit(1);
  } else {
    app.exit(0);
  }
});


function createMainWindow() {
  window = new BrowserWindow({
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

  window.setMenu(null);
  window.on('close', () => {
    window.webContents.closeDevTools();
  });
  window.on('closed', () => {
    window = null;
  });

  if (args.showWindowOnStart) {
    let showTimeout = null;
    let show = function() {
      window.show();
      args.showWindowOnStart = false;
      clearTimeout(showTimeout);
      showTimeout = null;
    };
    window.once('ready-to-show', show);
    showTimeout = setTimeout(show, 500);
  }
  window.maximizedPrev = null;

  window.loadURL(`file://${__dirname}/web/index.html`);
  if (args.debug) {
    window.webContents.on('devtools-opened', () => {
      window.show();
      window.focus();
    });
    window.webContents.openDevTools({ mode: 'detach' });
  }

  if (daemon) daemon.notifyWindowCreated();
};

app.on('window-all-closed', function() {
  // On OSX, an application stays alive even after all windows have been
  // closed (in the dock and/or tray), so don't exit.

  // On Windows, the taskbar button goes away if the window is closed,
  // and as a result we currently don't support taskbar-only mode, as
  // the application should keep running even with the Window closed.
});
