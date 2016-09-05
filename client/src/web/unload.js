
// Workaround to get an asynchronous window shutdown so we have time to clean stuff up

var unloadHandlers = [];
module.exports = window.addUnloadHandler = function addUnloadHandler(fn) {
  unloadHandlers.push(fn);
};

var doClose = window.close.bind(window);
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
  if (!exiting) {
    exiting = true;
    if (unloadHandlers) {
      setTimeout(performCloseChain, 0);
      e.returnValue = false;
      return false;
    }
  }
});

// Hook the window.close function too, to do programmatic shutdown more cleanly
window.close = function close() {
  if (!exiting) {
    exiting = true;
    if (unloadHandlers) {
      setTimeout(performCloseChain, 0);
    } else {
      doClose();
    }
  }
};
