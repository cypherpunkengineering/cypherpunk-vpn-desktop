import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';

import WorldMapImage from '../assets/img/worldmap_2000.png';
const MAP_SIZE = 2000;
const LONG_OFFSET = 11;




const pi = Math.PI, halfPi = pi / 2, epsilon = Number.EPSILON;

function vanDerGrinten3Raw(lambda, phi) {
  if (Math.abs(phi) < epsilon)
    return [lambda, 0];
  var sinTheta = phi / halfPi,
      theta = Math.asin(sinTheta);
  if (Math.abs(lambda) < epsilon || Math.abs(Math.abs(phi) - halfPi) < epsilon)
    return [0, pi * Math.tan(theta / 2)];
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
  var coords = vanDerGrinten3Raw((long - LONG_OFFSET) * pi / 180, lat * pi / 180);
  coords[0] = (coords[0] * 150 + (920 / 2)) * (MAP_SIZE / 920);
  coords[1] = (coords[1] * 150 + (500 / 2 + 500 * 0.15)) * (MAP_SIZE / 920);
  return coords;
}

function transformToLatLong(x, y) {
  // FIXME
}


export class WorldMap extends React.Component {
  static defaultProps = {
    lat: 0,
    long: 0,
    scale: 1,
    location: null,
    locations: {},
  }
  constructor(props) {
    super(props);
    this.state.locations = this.translateLocations(props.locations);
    Object.assign(this.state, this.translateLocation(props));
  }
  state = {
    moving: false,
    up: false,
    x: 0,
    y: 0,
    scale: 1,
    locations: {},
  }
  translateLocations(locations) {
    let locs = {};
    if (locations) {
      for (let id of Object.keys(locations)) {
        let loc = locations[id];
        if (!loc.hasOwnProperty('lat') || !loc.hasOwnProperty('long'))
          continue;
        let [ x, y ] = transformToXY(loc.lat, loc.long);
        locs[id] = { x, y, scale: loc.scale || 1 };
      }
    }
    return locs;
  }
  translateLocation(props, state) {
    if (!state) state = {};
    var { lat = null, long = null, scale = 1.0 } = (props.locations && props.location) ? props.locations[props.location] || {} : props;
    var [ x, y ] = (lat !== null && long !== null) ? transformToXY(lat, long) : [ null, null ];
    state.x = x;
    state.y = y;
    state.scale = scale;
    return state;
  }
  componentWillReceiveProps(props) {
    let state = {}, update = false;
    if (props.locations !== this.props.locations) {
      state.locations = this.translateLocations(props.locations);
      update = true;
    }
    if (props.location !== this.props.location || (!props.location && (props.lat !== this.props.lat || props.long !== this.props.long || props.scale !== this.props.scale))) {
      this.translateLocation(props, state);
      update = true;
    }
    if (update) {
      state.moving = true;
      state.up = true;
      if (this.movingTimeout) {
        clearTimeout(this.movingTimeout);
      }
      if (this.markerTimeout) {
        clearTimeout(this.markerTimeout);
      }
      this.movingTimeout = setTimeout(() => { delete this.movingTimeout; this.setState({ moving: false }); }, 1200);
      this.markerTimeout = setTimeout(() => { delete this.markerTimeout; this.setState({ up: false }); }, 800);
      this.setState(state);
    }
  }
  componentWillUnmount() {
    if (this.movingTimeout) {
      cancelTimeout(this.movingTimeout);
      delete this.movingTimeout;
    }
    if (this.markerTimeout) {
      cancelTimeout(this.markerTimeout);
      delete this.markerTimeout;
    }
  }
  render() {
    return (
      <div className={classList("worldmap", { 'moving': this.state.moving }, this.props.className)}>
        <div className="world" style={{ transform: (this.state.x !== null && this.state.y !== null) ? `scale(${this.state.scale}) translate(${-this.state.x}px,${-this.state.y}px)` : `scale(0.25) translate(-50%,-65%)` }}>
          <img src={WorldMapImage} width={MAP_SIZE}/>
          {Object.mapToArray(this.state.locations, (id, gps) => {
            return id === 'cypherplay' ? null : <div key={id} className={classList("point", { 'selected': this.props.location === id })} style={{ left: `${gps.x}px`, top: `${gps.y}px`, transform: `translate(-50%,-50%) scale(${1/*/gps.scale*/})` }}/>;
          }).filter(x => x)}
        </div>
        <i className={classList("marker icon", { up: this.state.up, hidden: (this.state.x === null || this.state.y === null || this.props.location === 'cypherplay') })}/>
      </div>
    );
  }
}

export default WorldMap;
