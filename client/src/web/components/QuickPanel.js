import React from 'react';
import ReactDOM from 'react-dom';
import daemon, { DaemonAware } from '../daemon';
import { Location } from './RegionSelector';
import RetinaImage, { Flag } from './Image';
import { classList } from '../util.js';

const CypherPlayIcon = { 1: require('../assets/img/icon_cypherplay.png'), 2: require('../assets/img/icon_cypherplay@2x.png') };
const FastestIcon = { 1: require('../assets/img/icon_fastest.png'), 2: require('../assets/img/icon_fastest@2x.png') };

export class QuickPanel extends DaemonAware(React.Component) {

  constructor(props) {
    super(props);
    Object.assign(this.state, this.checkPingStats(daemon.state.pingStats));
  }
  state = {
    locations: daemon.config.locations,
    regions: daemon.config.regions,
    countryNames: daemon.config.countryNames,
    regionNames: daemon.config.regionNames,
    regionOrder: daemon.config.regionOrder,
    location: daemon.settings.location,
    locationFlag: daemon.settings.locationFlag,
    favorites: Array.toDict(daemon.settings.favorites, f => f, f => true),
    lastConnected: daemon.settings.lastConnected,
    pingStats: daemon.state.pingStats,
    selected: 6,
    fastest: null,
    fastestUS: null,
    fastestUK: null,
    custom1: null,
    custom2: null,
  }
  daemonConfigChanged(config) {
    if (config.hasOwnProperty('locations')) this.setState({ locations: config.locations });
    if (config.hasOwnProperty('regions')) this.setState({ regions: config.regions });
    if (config.hasOwnProperty('countryNames')) this.setState({ countryNames: config.countryNames });
    if (config.hasOwnProperty('regionNames')) this.setState({ regionNames: config.regionNames });
    if (config.hasOwnProperty('regionOrder')) this.setState({ regionOrder: config.regionOrder });
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('location')) this.setState({ location: settings.location });
    if (settings.hasOwnProperty('locationFlag')) this.setState({ locationFlag: settings.locationFlag });
    if (settings.hasOwnProperty('favorites')) this.setState({ favorites: Array.toDict(settings.favorites, f => f, f => true) });
    if (settings.hasOwnProperty('lastConnected')) this.setState({ lastConnected: settings.lastConnected });

    let selected = 6; // other
    switch (this.state.locationFlag) {
      case 'cypherplay': selected = 0; break;
      case 'fastest': selected = 1; break;
      case 'fastest-us': selected = 2; break;
      case 'fastest-uk': selected = 3; break;
      default:
        if (this.state.location === this.state.custom1) selected = 4;
        else if (this.state.location === this.state.custom2) selected = 5;
        break;
    }
    if (selected !== this.state.selected) this.setState({ selected: selected });
  }
  daemonStateChanged(state) {
    if (state.hasOwnProperty('pingStats')) this.setState(this.checkPingStats(state.pingStats));
  }

  checkPingStats(stats) {
    let fastest = null, fastestUS = null, fastestUK = null;
    Object.forEach(stats, ([id, ping]) => {
      console.log(id, ping);
    });
    return { pingStats: stats };
  }

  onClick(button, event) {
    if (button === 6) {
      // Show full location list
    } else if (button != this.state.selected && !this.state.buttons[button].disabled) {
      let settings = {};
      if (button === 0) {
        settings.optimizeDNS = true;
      } else if (this.state.selected === 0) {
        settings.optimizeDNS = false;
      }
      switch (button) {
        case 0: settings.location = this.state.fastest; break;
        case 1: settings.location = this.state.fastest; break;
        case 2: settings.location = this.state.fastestUS; break;
        case 3: settings.location = this.state.fastestUK; break;
        case 4: break;
        case 5: break;
      }
      daemon.call.applySettings(settings).then(() => {
        daemon.post.connect();
      });
    }
  }

  render() {
    return(
      <div className="quick-panel">
        <div className="description">CONNECT TO</div>
        <Location className="selected-location" location={this.state.locations[this.state.location]}/>
        <div className="grid">
          <div className={classList("cypherplay", { "selected disabled": this.state.selected === 0 })} onClick={e => this.onClick(0, e)}>
            <RetinaImage src={CypherPlayIcon}/><span>CypherPlay&trade;</span>
          </div>
          <div className={classList("fastest", { "selected disabled": this.state.selected === 1 })} onClick={e => this.onClick(1)}>
            <RetinaImage src={FastestIcon}/><span>Fastest</span>
          </div>
          <div className={classList("fastest-us", { "selected disabled": this.state.selected === 2 })} onClick={e => this.onClick(2)}>
            <Flag country="us"/><span>Fastest US</span>
          </div>
          <div className={classList("fastest-uk", { "selected disabled": this.state.selected === 3 })} onClick={e => this.onClick(3)}>
            <Flag country="gb"/><span>Fastest UK</span>
          </div>
          <div className={classList("favorite disabled", { "selected disabled": this.state.selected === 4 })} onClick={e => this.onClick(4, e)}>

          </div>
          <div className={classList("recent disabled", { "selected disabled": this.state.selected === 5 })} onClick={e => this.onClick(5, e)}>

          </div>
          <div className={classList("other selected", { "selected": this.state.selected === 6 })} onClick={e => this.onClick(6, e)}>
            <i className="horizontal ellipsis icon"/><span>Other</span>
          </div>
        </div>
      </div>
    );
  }
}

export default QuickPanel;
