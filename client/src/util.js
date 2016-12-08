
export function eventPromise(emitter, name) {
  return new Promise((resolve, reject) => {
    emitter.once(name, resolve);
  });
}

export function timeoutPromise(promise, delay, timeoutIsSuccess = false) {
  return new Promise((resolve, reject) => {
    var timeout = setTimeout(() => { console.log("timed out"); if (timeoutIsSuccess) resolve(); else reject(); }, delay);
    promise.then(val => { clearTimeout(timeout); resolve(val); }, reject);
  });
}

export function nodePromise(call) {
  return new Promise((resolve, reject) => {
    call(function(err, val) {
      if (err) reject(err);
      else resolve(val);
    })
  });
}
