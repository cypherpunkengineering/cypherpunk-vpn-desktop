import RPC from '../rpc.js';
import Loader from './loader.js';
import './unload.js';

class Daemon extends RPC {
  constructor() {
    super({
      url: 'ws://127.0.0.1:9337/',
      onerror: onerror,
      onopen: onopen
    });
  }
}

var daemon = new Daemon();
var opened = false;
var errorCount = 0;

window.addUnloadHandler(function() {
  return daemon.disconnect();
});

function onerror() {
  errorCount++;
  if (!opened) {
    if (errorCount >= 3) {
      window.alert("Unable to connect to the background service.\n\nTry reinstalling the application.");
      // FIXME: Replace with quit command that tells main process to exit too
      daemon.disconnect();
      window.close();
      return false;
    }
  } else {
    Loader.show("Reconnecting");
  }
  // Try to reconnect
  return true;
}
function onopen() {
  opened = true;
  errorCount = 0;
  Loader.hide();
}

module.exports = daemon;
