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
    this.daemonSubscribeState({
      config: { locations: true, regions: true, countryNames: true, regionNames: true, regionOrder: true },
      settings: { location: true, locationFlag: true, favorites: v => { favorites: Array.toDict(v, f => f, f => true) }, lastConnected: true, overrideDNS: true },
      state: { pingStats: true },
    });
  }
  state = {
    selected: 6,
    fastest: null,
    fastestUS: null,
    fastestUK: null,
    custom1: null,
    custom2: null,
  }
  daemonDataChanged(state) {
    let fastest = null, fastestUS = null, fastestUK = null;
    let locations = state.locations;
    Object.forEach(state.pingStats, (id, ping) => {
      if (ping.replies) {
        if (locations[id] && (!fastest || ping.average < state.pingStats[fastest].average)) fastest = id;
        if (locations[id] && locations[id].country.toLowerCase() == 'us' && (!fastestUS || ping.average < state.pingStats[fastestUS].average)) fastestUS = id;
        if (locations[id] && locations[id].country.toLowerCase() == 'gb' && (!fastestUK || ping.average < state.pingStats[fastestUK].average)) fastestUK = id;
      }
    });
    let selected = 6; // other
    switch (state.locationFlag) {
      case 'cypherplay': if (state.overrideDNS) { selected = 0; break; } // fallthrough
      case 'fastest': selected = 1; break;
      case 'fastest-us': selected = 2; break;
      case 'fastest-uk': selected = 3; break;
      default:
        if (state.location === state.custom1) selected = 4;
        else if (state.location === state.custom2) selected = 5;
        break;
    }
    return { fastest, fastestUS, fastestUK, selected };
  }

  onClick(button, event) {
    if (button === 6) {
      // Show full location list
    } else if (/*button != this.state.selected &&*/ !event.currentTarget.classList.contains('disabled')) {
      let settings = { locationFlag: '' };
      switch (button) {
        case 0: settings.location = this.state.fastest; settings.locationFlag = 'cypherplay'; break;
        case 1: settings.location = this.state.fastest; settings.locationFlag = 'fastest'; break;
        case 2: settings.location = this.state.fastestUS; settings.locationFlag = 'fastest-us'; break;
        case 3: settings.location = this.state.fastestUK; settings.locationFlag = 'fastest-uk'; break;
        case 4: break;
        case 5: break;
      }
      if (settings.hasOwnProperty('location') && !location) {
        console.error("Can't connect - no known fastest server");
        return;
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
          <div className={classList("cypherplay", { "selected": this.state.selected === 0, "disabled": !this.state.fastest || !this.state.overrideDNS })} onClick={e => this.onClick(0, e)}>
            <RetinaImage src={CypherPlayIcon}/><span>CypherPlay&trade;</span>
          </div>
          <div className={classList("fastest", { "selected": this.state.selected === 1, "disabled": !this.state.fastest })} onClick={e => this.onClick(1, e)}>
            <RetinaImage src={FastestIcon}/><span>Fastest</span>
          </div>
          <div className={classList("fastest-us", { "selected": this.state.selected === 2, "disabled": !this.state.fastestUS })} onClick={e => this.onClick(2, e)}>
            <Flag country="us"/><span>Fastest US</span>
          </div>
          <div className={classList("fastest-uk", { "selected": this.state.selected === 3, "disabled": !this.state.fastestUK })} onClick={e => this.onClick(3, e)}>
            <Flag country="gb"/><span>Fastest UK</span>
          </div>
          <div className={classList("favorite disabled", { "selected": this.state.selected === 4 })} onClick={e => this.onClick(4, e)}>

          </div>
          <div className={classList("recent disabled", { "selected": this.state.selected === 5 })} onClick={e => this.onClick(5, e)}>

          </div>
          <div className={classList("other", { "selected": this.state.selected === 6 })} onClick={e => this.onClick(6, e)}>
            <i className="horizontal ellipsis icon"/><span>Other</span>
          </div>
        </div>
      </div>
    );
  }
}

export default QuickPanel;
