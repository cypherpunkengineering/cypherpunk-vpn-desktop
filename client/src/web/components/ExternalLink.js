import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';
const { shell } = require('electron').remote;

export default function ExternalLink({ className, href, ...props } = {}) {
  return <a className={classList('external', className)} tabIndex="0" onClick={() => { shell.openExternal(href); }} {...props}/>;
}
