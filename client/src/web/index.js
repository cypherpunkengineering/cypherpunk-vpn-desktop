// Manually include just the minimum styles for the initial page
// (webpack will include the rest automatically in the second bundle)
import './assets/fonts/dosis.css';
import 'semantic/components/reset.min.css';
import 'semantic/components/site.min.css';
import 'semantic/components/dimmer.min.css';
import 'semantic/components/loader.min.css';
import './assets/css/index.css';

import { webFrame } from 'electron';

webFrame.setZoomLevelLimits(1, 1);

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

// Load the full app asynchronously
require.ensure(['./app.jsx'], function(require) { app = require('./app.jsx'); });
