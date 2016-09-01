
// Workaround to get an asynchronous window shutdown so we have time to clean stuff up

var unloadHandlers = [];
module.exports = window.addUnloadHandler = function addUnloadHandler(fn) {
  unloadHandlers.push(fn);
};

var oldClose = window.close;
var doClose = function() { console.log("doClose"); oldClose.call(window); };
var exiting = false;

function performCloseChain() {
  var p = Promise.resolve(null);
  unloadHandlers.forEach(fn => {
    p = p.then(fn);
  });
  return p
    .then(() => { doClose(); })
    .catch(() => { exiting = false; });
}

// Hook the window.onbeforeunload event (in case the window itself is closed)
window.addEventListener('beforeunload', e => {
  console.log("beforeunload");
  if (!exiting) {
    exiting = true;
    performCloseChain();
    e.returnValue = false;
    return false;
  }
});

// Hook the window.close function too, to do programmatic shutdown more cleanly
window.close = function close() {
  console.log("close");
  if (!exiting) {
    exiting = true;
    performCloseChain();
  }
};
