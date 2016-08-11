require('../less/app.less');

import React from 'react';
import ReactDOM from 'react-dom';
import Interface from './uicomponents/Interface.js';

const app = document.getElementById('app');

ReactDOM.render(<Interface/>, app);
