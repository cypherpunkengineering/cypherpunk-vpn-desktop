import { ipcRenderer as ipc } from 'electron';

// Workaround to get an asynchronous window shutdown so we have time to clean stuff up

var unloadHandlers = [];
module.exports = window.addUnloadHandler = function addUnloadHandler(fn) {
  unloadHandlers.push(fn);
};

var doClose = window.close.bind(window);
var overrideClose = true;
var exiting = false;

function performCloseChain() {
  var p = Promise.resolve(null);
  unloadHandlers.forEach(fn => { p = p.then(fn); });
  return p
    .then(doClose)
    .catch(() => { exiting = false; });
}

// Hook the window.onbeforeunload event (in case the window itself is closed)
window.addEventListener('beforeunload', e => {
  if (overrideClose) {
    setImmediate(() => ipc.send('close'));
    e.returnValue = false;
    return false;
  } else if (!exiting) {
    exiting = true;
    if (unloadHandlers) {
      setImmediate(performCloseChain);
      e.returnValue = false;
      return false;
    }
  }
});

// Hook the window.close function too, to do programmatic shutdown more cleanly
window.close = function close() {
  if (overrideClose) {
    setImmediate(() => ipc.send('close'));
  } else if (!exiting) {
    exiting = true;
    if (unloadHandlers) {
      setImmediate(performCloseChain);
    } else {
      setImmediate(doClose);
    }
  }
};

ipc.on('close', function() {
  overrideClose = false;
  setImmediate(() => window.close());
});
