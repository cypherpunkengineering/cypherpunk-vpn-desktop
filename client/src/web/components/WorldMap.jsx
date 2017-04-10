import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';

import WorldMapImage from '../assets/img/worldmap.png';


const GPS = {
  'honolulu': { lat: -21.3069, long: -157.8533, scale: 2.0 },
  'stockholm': { lat: -59.3293, long: 18.0686, scale: 0.5 },
}


const pi = Math.PI, halfPi = pi / 2, epsilon = Number.EPSILON;

function vanDerGrinten3Raw(lambda, phi) {
  if (Math.abs(phi) < epsilon) return [lambda, 0];
  var sinTheta = phi / halfPi,
      theta = Math.asin(sinTheta);
  if (Math.abs(lambda) < epsilon || Math.abs(Math.abs(phi) - halfPi) < epsilon) return [0, pi * Math.tan(theta / 2)];
  var A = (pi / lambda - lambda / pi) / 2,
      y1 = sinTheta / (1 + Math.cos(theta));
  return [
    pi * (Math.sign(lambda) * Math.sqrt(A * A + 1 - y1 * y1) - A),
    pi * y1
  ];
}

vanDerGrinten3Raw.invert = function(x, y) {
  if (!y) return [x, 0];
  var y1 = y / pi,
      A = (pi * pi * (1 - y1 * y1) - x * x) / (2 * pi * x);
  return [
    x ? pi * (Math.sign(x) * Math.sqrt(A * A + 1) - A) : 0,
    halfPi * Math.sin(2 * Math.atan(y1))
  ];
};

function transformToXY(lat, long) {
  var coords = vanDerGrinten3Raw((long - 11) * pi / 180, lat * pi / 180);
  coords[0] = (coords[0] * 150 + (920 / 2)) * (8000 / 920);
  coords[1] = (coords[1] * 150 + (500 / 2 + 500 * 0.15)) * (8000 / 920);
  return coords;
}

function transformToLatLong(x, y) {
  // FIXME
}


export class WorldMap extends React.Component {
  static defaultProps = {
    lat: -21.3069,
    long: -157.8533,
  }
  state = {
    up: false,
    location: 'honolulu',
  }
  move(location) {
    this.setState({ up: true, location });
    if (this.markerTimeout) {
      clearTimeout(this.markerTimeout);
    }
    this.markerTimeout = setTimeout(() => { delete this.markerTimeout; this.setState({ up: false }); }, 800);
  }
  componentWillUnmount() {
    if (this.markerTimeout) {
      cancelTimeout(this.markerTimeout);
      this.markerTimeout = null;
    }
  }
  componentWillReceiveProps(props) {

  }
  render() {
    var { lat, long, scale = 1.0 } = GPS[this.state.location];
    var [ x, y ] = vanDerGrinten3Raw((long - 11) * pi / 180, lat * pi / 180);
    x = (x * 150 + (920 / 2)) * (8000 / 920);
    y = (y * 150 + (500 / 2 + 500 * 0.15)) * (8000 / 920);
    return (
      <div className="worldmap" onClick={() => this.move(this.state.location === 'honolulu' ? 'stockholm' : 'honolulu')}>
        <div className="world" style={{ transform: `scale(${scale}) translate(${-x}px,${-y}px)` }}>
          <img src={WorldMapImage}/>
        </div>
        <i className={classList("marker icon", { up: this.state.up })}/>
      </div>
    );
  }
}

export default WorldMap;
