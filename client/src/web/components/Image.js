import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';

export class RetinaImage extends React.Component {
  render() {
    var src = null;
    var srcSet = [];
    if (this.props.src) {
      if (Array.isArray(this.props.src)) {
        this.props.src.forEach(url => {
          var m = url.match(/(@([0-9.]+)x)?\.(jpe?g|gif|png)$/i);
          var s = (m && m[2]) ? Number(m[2]) : 1;
          srcSet.push({ scale: s, url });
        });
      } else if (typeof this.props.src === 'object') {
        Object.keys(this.props.src).forEach(scale => {
          var s = Number(scale.endsWith('x') ? scale.substring(0, scale.length - 1) : scale);
          srcSet.push({ scale: s, url: this.props.src[scale] });
        });
      } else if (typeof this.props.src === 'string') {
        src = this.props.src;
      }
    }
    if (!src && srcSet.length > 0) {
      srcSet.forEach(x => {
        if (x.scale == 1) {
          src = x.url;
        }
      });
    }
    var props = Object.assign({}, this.props);
    delete props.src;
    delete props.srcSet;
    delete props.children;
    if (srcSet.length > 0) {
      props.srcSet = srcSet.map(x => `${x.url} ${x.scale}x`).join(', ');
    }
    if (src) {
      props.src = src;
    }
    return React.createElement('img', props, null);
  }
}

export const Flag = ({ country, size = 24, className, ...props } = {}) => {
  country = country.toLowerCase();
  const flag = (dpi = '') => `../assets/img/flags/${size}/${country}${dpi}.png`;
  const src = { [1]: flag(), [2]: flag('@2x') };
  return country ? <RetinaImage src={src} className={classList('flag', className)} {...props} /> : null;
};

export default RetinaImage;
