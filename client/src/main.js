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
  clean: null,
};

global.exit = function exit(code) {
  return new Promise((resolve, reject) => {
    exiting = true;
    app.exit(code);
    // intentionally doesn't resolve
  });
};

process.on('uncaughtException', function(err) {
  console.log('Uncaught exception:', err);
  app.exit(1);
});
process.on('unhandledrejection', function (err, promise) {
  console.log('Unhandled rejection:', err, promise);
});

{
  let a = process.argv.slice(1);
  while (a.length) {
    switch (a.shift()) {
      case '--debug': args.debug = true; break;
      case '--background': args.showWindowOnStart = false; break;
    }
  }
}

if (process.platform !== 'darwin' && app.makeSingleInstance((argv, cwd) => {
  console.log("Attempted to start second instance: ", argv, cwd);
  if (window) {
    window.show();
    window.focus();
  }
  return true;
})) {
  exiting = true;
  app.exit(0);
} else {
  require('./app.js');
}
