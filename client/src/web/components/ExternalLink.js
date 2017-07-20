import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';
const { shell } = require('electron').remote;

export const WEBSITE_ROOT = 'https://cypherpunk.com';


export function ExternalLink({ className, href, params = null, ...props } = {}) {
  if (href.startsWith('/') && !href.startsWith('//')) {
    href = WEBSITE_ROOT + href;
  }
  if (params && typeof params === 'object') {
    let prefix = href.indexOf('?') < 0 ? '?' : '&';
    let query = Object.mapToArray(params, (k,v) => `${k}=${encodeURIComponent(v)}`).join('&');
    if (query) href = `${href}${prefix}${query}`;
  }
  return <a className={classList('external', className)} tabIndex="0" onClick={() => { shell.openExternal(href); }} {...props}/>;
}

export default ExternalLink;
