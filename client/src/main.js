const { app } = require('electron');

// Define global objects (easy to access from browser process too)
global.daemon = null;
global.window = null;
global.tray = null;
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

if (app.makeSingleInstance((argv, cwd) => {
  console.log("Attempted to start second instance: ", argv, cwd);
  if (window) window.show();
})) {
  exiting = true;
  app.exit(0);
} else {
  require('./app.js');
}
