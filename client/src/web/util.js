import React from 'react';
import ReactDOM from 'react-dom';

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

export * from '../util.js';
