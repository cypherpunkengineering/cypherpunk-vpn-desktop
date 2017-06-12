import React from 'react';
import ReactDOM from 'react-dom';
import '../util.js';

Object.defineProperty(React.Component.prototype, 'dom', {
  get: function() {
    return ReactDOM.findDOMNode(this);
  }
});
Object.defineProperty(React.Component.prototype, '$dom', {
  get: function() {
    return $(this.dom);
  }
});
Object.defineProperty(React.Component.prototype, '$', {
  writable: false,
  value: function(selector) {
    return $(selector, this.dom);
  }
});

export function classList() {
  var result = [];
  for (var i = 0; i < arguments.length; i++) {
    var arg = arguments[i];
    if (typeof arg === 'string') {
      result.push(arg);
    } else if (Array.isArray(arg)) {
      result.push(classList(...arg));
    } else if (arg && typeof arg === 'object') {
      Object.keys(arg).forEach(k => {
        if (arg[k]) {
          result.push(k);
        }
      });
    }
  }
  return result.join(' ');
}
window.classList = classList;

export * from '../util.js';
