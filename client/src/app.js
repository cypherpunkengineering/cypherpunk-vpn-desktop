const electron = require('electron');
const { app, dialog, BrowserWindow, Tray, Menu, nativeImage: NativeImage, ipcMain: ipc } = electron;

import fs from 'fs';
import { eventPromise, timeoutPromise } from './util.js';
import Notification from './notification.js';
import AutoLaunch from './autostart.js';

// Hardcode DNS resolution for API requests
app.commandLine.appendSwitch('host-resolver-rules', 'MAP api.cypherpunk.com 216.239.32.21');
// Disable ClearType on Windows, as it tends to look bad
if (process.platform === 'win32') {
  app.commandLine.appendSwitch('disable-lcd-text');
}

// Initialize the tray manager
tray = require('./tray.js');

// Expose the autostart setting to the window
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
});


// This event is received when someone is attempting to kill the main process.
app.on('before-quit', event => {
  exiting = true;
});

// This event is received at the start of the main process quit sequence.
app.on('will-quit', event => {
  exiting = true;
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
    return timeoutPromise(Promise.all([
      daemon.connect(),
      eventPromise(daemon, 'up'),
      eventPromise(daemon, 'data')
    ]), 10000).catch(e => {
      if (e instanceof Error && e.message === "Daemon version mismatch") {
        dialog.showErrorBox("Startup Error", "The Cypherpunk Privacy helper service is not the correct version. Please try reinstalling the application.");
      } else {
        dialog.showErrorBox("Startup Error", "The Cypherpunk Privacy helper service does not appear to be running. Please try reinstalling the application.");
      }
      throw null;
    });
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
  let lastState = 'DISCONNECTED';
  daemon.on('state', state => {
    if (state.state && state.state !== lastState) {
      if (daemon.settings.showNotifications) {
        switch (state.state) {
          case 'CONNECTED':
            if (lastState === 'RECONNECTING' || lastState === 'STILL_RECONNECTING') {
              new Notification("Reconnected to Cypherpunk network", { body: "You are once again safely connected to the Cypherpunk Privacy network." });
            } else {
              new Notification("Connected to Cypherpunk network", { body: "You are now safely connected to the Cypherpunk Privacy network. Enjoy a more free internet!" });
            }
            break;
          case 'RECONNECTING':
            if (lastState === 'CONNECTED') {
              new Notification("Reconnecting to Cypherpunk network...", { body: "Your connection to the Cypherpunk network has been disrupted, please wait while we try to restore it..." });
            }
            break;
          case 'DISCONNECTED':
            if (daemon.settings.firewall == 'on') {
              new Notification("Disconnected from Cypherpunk network", { body: "Reminder: Leak Protection is active, blocking your internet connection." });
            } else {
              new Notification("Disconnected from Cypherpunk network", { body: "You are now connecting directly to the internet." });
            }
            break;
        }
      }
      lastState = state.state;
    }
  });
  daemon.on('error', error => {
    // TODO: Not yet used
  });
  createMainWindow();
  tray.create();
}).catch(err => {
  if (err) {
    dialog.showErrorBox("Initialization Error", "An unexpected error happened while launching Cypherpunk Privacy:\n\n" + (err.stack ? err.stack : err));
  }
  app.exit(1);
});


function createMainWindow() {
  window = new BrowserWindow({
    title: 'Cypherpunk Privacy',
    //icon: icon,
    backgroundColor: '#163238',
    show: false,
    fullscreen: false,
    useContentSize: true,
    width: 350,
    height: 530,
    resizable: false,
    minimizable: process.platform === 'darwin',
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

  let displayedHideNotification = false;
  window.on('close', event => {
    if (!exiting) {
      event.preventDefault();
      window.hide();
      onHideWindow();
      if (!displayedHideNotification) {
        displayedHideNotification = new Notification({ body: "Cypherpunk Privacy will keep running in the background - control it from the system tray." }).success;
      }
    }
  });

  window.on('show', () => {
    if (!exiting) {
      onShowWindow();
    }
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
  } else {
    window.hide();
    onHideWindow();
  }

  window.loadURL(`file://${__dirname}/web/index.html`);
  if (args.debug) {
    window.webContents.on('devtools-opened', () => {
      if (!exiting) {
        window.show();
        window.focus();
      }
    });
    window.webContents.openDevTools({ mode: 'detach' });
  }

  if (daemon) daemon.notifyWindowCreated();
};


function onHideWindow() {
  window.setSkipTaskbar(true);
  if (app.dock && app.dock.hide) {
    app.dock.hide();
  }
}

function onShowWindow() {
  window.setSkipTaskbar(false);
  if (app.dock && app.dock.show) {
    app.dock.show();
  }
}

//app.on('window-all-closed', function() {
  // On OSX, an application stays alive even after all windows have been
  // closed (in the dock and/or tray), so don't exit.

  // On Windows, the taskbar button goes away if the window is closed,
  // and as a result we currently don't support taskbar-only mode, as
  // the application should keep running even with the Window closed.
//});

app.on('activate', function() {
  if (window) window.show();
});
