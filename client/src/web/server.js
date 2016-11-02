
const SERVER = "https://cypherpunk.engineering";

const server = {};

function makeQueryString(params) {
  var queryString = '';
  for (let key of Object.keys(params)) {
    if (Array.isArray(params[key])) {
      for (let item of params[key]) {
        processedUrl += '&' + encodeURIComponent(key) + '[]=' + encodeURIComponent(item);
      }
    } else {
      processedUrl += '&' + encodeURIComponent(key) + '=' + encodeURIComponent(params[key]);
    }
  }
  return queryString.substr(1);
}

function xhr(method, url, params, { paramType = 'json' } = {}) {
  if (!url.startsWith('http')) {
    url = SERVER + '/' + url.replace(/^\//, '');
  }
  var xhr = new XMLHttpRequest();
  xhr.stack = new Error().stack;

  function makeError(status, message, data) {
    var error = new Error(message);
    error.status = status;
    error.xhr = xhr;
    error.data = data;
    if (typeof data === 'object' && data.error) {
      error.type = data.error;
    }
    return error;
  }

  var promise = new Promise(function xhrPromise(resolve, reject) {
    var processedUrl = url;
    var contentType = 'application/x-www-form-urlencoded';
    var data = null;
    if (params) {
      if (method === 'GET') {
        // Append args to any existing query string
        processedUrl = url + makeQueryString().replace(/^(?=.)/, processedUrl.indexOf('?') == -1 ? '?' : '&');
      } else if (method === 'POST' || method === 'PUT') {
        if (paramType.indexOf('json') != -1) {
          contentType = 'application/json';
          data = JSON.stringify(params);
        } else {
          data = makeQueryString(params);
        }
      }
    }
    xhr.open(method, processedUrl, true);
    xhr.setRequestHeader("Content-Type", contentType);
    xhr.setRequestHeader("Accept", "application/json,text/plain;q=0.9,text/html;0.8,*/*;q=0.7");
    xhr.withCredentials = true;
    xhr.onload = function() {
      var data = this.response;
      if (typeof data === 'string' && data.length > 0 && data[0] == '{') {
        try {
          data = JSON.parse(data);
        } catch (e) {}
      }
      if (this.status < 200 || this.status >= 300 || (typeof data === 'object' && data.error)) {
        reject(makeError(this.status, ((typeof data === 'object') && data.message) || "Server call failed with code " + (this.statusText || this.status), data));
      } else {
        resolve({ data: data, status: this.status, xhr: this });
      }
    };
    xhr.onerror = function() {
      reject(makeError('error', "Server connection failed"));
    };
    xhr.onabort = function() {
      reject(makeError('abort', "Server call aborted"));
    };
    xhr.send(data);
  });

  var originalThen = promise.then;
  promise.then = function then(mapOrCallback, cbResolved, cbRejected) {
    if (typeof mapOrCallback === 'function') {
      return originalThen.apply(promise, arguments);
    }
    let handlers = {};
    for (let key of Object.keys(mapOrCallback)) {
      for (let subKey of key.split('|')) {
        handlers[subKey] = mapOrCallback[key];
      }
    }
    function callHandler(status, data, error) {
      var statusString = '' + status;
      function* candidateKeys() {
        if (error && error.type) {
          yield [error.type, error];
        }
        if (typeof status === 'number') {
          yield [status, data];
          yield [statusString, data];
          yield [statusString.replace(/..$/, '??'), data];
          yield ['???', data];
        }
        if (error) {
          yield [statusString, error];
        }
        yield ['*', error || makeError(status, ((typeof data === 'object') && data.message) || "Unhandled response: " + status, data)];
      }
      // Find first matching pattern and call its handler
      for (let [key, param] of candidateKeys()) {
        if (handlers[key]) {
          return handlers[key](param, status, xhr);
        }
      }
      // Default callbacks
      if (error) {
        if (cbRejected) {
          return cbRejected(error);
        } else {
          throw error;
        }
      } else if (cbResolved) {
        return cbResolved({ data: data, status: status, xhr : xhr });
      }
      throw makeError(status, ((typeof data === 'object') && data.message) || "Unhandled response: " + status, data);
    };
    return originalThen.call(promise, ({ data, status, xhr }) => {
      return callHandler(status, data);
    }, (error) => {
      return callHandler(error.status || 'error', error.data, error);
    });
  }
  promise.xhr = xhr;
  return promise;
};

function http(url) {
  return {
    get:    function(params) { return impl.xhr('GET',    url, params); },
    post:   function(params) { return impl.xhr('POST',   url, params); },
    put:    function(params) { return impl.xhr('PUT',    url, params); },
    delete: function(params) { return impl.xhr('DELETE', url, params); },
  };
}

function api(url) {
  return {
    get:    xhr.bind(server, 'GET',    url),
    post:   xhr.bind(server, 'POST',   url),
    put:    xhr.bind(server, 'PUT',    url),
    delete: xhr.bind(server, 'DELETE', url),
  }
}

// Returns a Promise which will be either fulfilled with an object of
// { data: "..." | { ... }, status: 2xx, xhr: XMLHttpRequest }, or rejected
// with an Error in the following circumstances:
//
//   Connection failure: Error { status: 'error' }
//   Request aborted: Error { status: 'abort' }
//   Non-2xx HTTP response: Error { status: HTTP code, message, type, data }
//   Response has 'error' field: Error { status, message, type, data }
//
// where message and type are extracted from the response data's message and
// error fields, respectively, when available. Each error also has an xhr
// property and an originalStack property referring to where the request was
// originally made.
//
// The returned Promise further has an overloaded then() function which can
// take a handler map for various statuses instead of a callback, as follows:
//
//   promise.then(map);
//   promise.then(map, cbResolved);
//   promise.then(map, cbResolved, cbRejected);
//   promise.then(cbResolved);
//   promise.then(cbResolved, cbRejected);
//
// The map parameter is an object with patterns as keys and callbacks as values,
// where the most specific match will be invoked and its return value used to
// fulfill the returned promise. If no handler is matched, the cbResolved
// handler will be invoked for non-errors or the cbRejected handler for errors,
// the same as for the normal then() function. If no handler is matched and
// no default calbacks are provided, the returned promise will reject with
// an 'unhandled' error.
//
// The supported handler map key/value patterns are:
//
//   200: data => ...        // numeric HTTP status code
//   '202': data => ...      // string HTTP status code
//   '4??': data => ...      // any status code between 400-499
//   '???': data => ...      // any status code
//   'named': error => ...   // named error in response
//   'error': error => ...   // connection or any error
//   'abort': error => ...   // request aborted
//   '*': error => ...       // any result (including unhandled responses)
//
// For errors, the specificity of patterns is as follows:
//
//   1. Name of error in response data
//   2. HTTP status code patterns
//   3. Generic 'error' or 'abort' code
//   4. Universal '*' handler
//
Object.assign(server, {
  api:    api,
  get:    xhr.bind(server, 'GET'),
  post:   xhr.bind(server, 'POST'),
  put:    xhr.bind(server, 'PUT'),
  delete: xhr.bind(server, 'DELETE'),

  // Idea? put actual API endpoints here too?
});

export default server;
