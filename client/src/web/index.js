import './assets/fonts/dosis.css';
import 'semantic/semantic.min.css';
import './index.css';

let app = null;

// Do this one old school (i.e. without React) so it can display near-instantaneously
const [ root, loader ] = (function() {
  var r = document.createElement('div');
  r.id = 'root-container';
  var l = document.createElement('div');
  l.id = 'load-screen';
  l.className = 'ui active dimmer';
  var i = document.createElement('div');
  i.className = 'ui big text loader';
  i.innerText = "Loading";
  l.appendChild(i);
  document.body.appendChild(r)
  document.body.appendChild(l);
  return [r, l];
})();

require.ensure(['./app.jsx'], function(require) { app = require('./app.jsx'); });
