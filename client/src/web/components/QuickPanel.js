import React from 'react';
import ReactDOM from 'react-dom';
import daemon, { DaemonAware } from '../daemon';
import { Location } from './RegionSelector';
import RetinaImage, { Flag } from './Image';

const CypherPlayIcon = { 1: require('../assets/img/icon_cypherplay.png'), 2: require('../assets/img/icon_cypherplay@2x.png') };
const FastestIcon = { 1: require('../assets/img/icon_fastest.png'), 2: require('../assets/img/icon_fastest@2x.png') };

export class QuickPanel extends DaemonAware(React.Component) {

  state = {
    locations: daemon.config.locations,
    regions: daemon.config.regions,
    countryNames: daemon.config.countryNames,
    regionNames: daemon.config.regionNames,
    regionOrder: daemon.config.regionOrder,
    selected: daemon.settings.location,
    favorites: Array.toDict(daemon.settings.favorites, f => f, f => true),
    recent: daemon.settings.recent,
    pingStats: daemon.state.pingStats,
  }
  daemonConfigChanged(config) {
    if (config.hasOwnProperty('locations')) this.setState({ locations: config.locations });
    if (config.hasOwnProperty('regions')) this.setState({ regions: config.regions });
    if (config.hasOwnProperty('countryNames')) this.setState({ countryNames: config.countryNames });
    if (config.hasOwnProperty('regionNames')) this.setState({ regionNames: config.regionNames });
    if (config.hasOwnProperty('regionOrder')) this.setState({ regionOrder: config.regionOrder });
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('location')) this.setState({ selected: settings.location });
    if (settings.hasOwnProperty('favorites')) this.setState({ favorites: Array.toDict(settings.favorites, f => f, f => true) });
    if (settings.hasOwnProperty('recent')) this.setState({ recent: settings.recent });
  }
  daemonStateChanged(state) {
    if (state.hasOwnProperty('pingStats')) this.setState({ pingStats: daemon.state.pingStats });
  }


  render() {
    return(
      <div className="quick-panel">
        <div className="description">CONNECT TO</div>
        <Location className="selected-location" location={this.state.locations[this.state.selected]}/>
        <div className="grid">
          <div className="auto-routing"><RetinaImage src={CypherPlayIcon}/><span>CypherPlay&trade;</span></div>
          <div className="fastest"><RetinaImage src={FastestIcon}/><span>Fastest</span></div>
          <div className="fastest-us"><Flag country="us"/><span>Fastest US</span></div>
          <div className="fastest-uk"><Flag country="gb"/><span>Fastest UK</span></div>
          <div className="favorite disabled"></div>
          <div className="recent disabled"></div>
          <div className="other selected"><i className="horizontal ellipsis icon"/><span>Other</span></div>
        </div>
      </div>
    );
  }
}

export default QuickPanel;
