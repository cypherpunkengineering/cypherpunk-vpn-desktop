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
      state: { state: true, connect: true, pingStats: true },
    });
  }
  static defaultProps = {
    expanded: false,
    onOtherClick: function() {},
  }
  state = {
    selected: 6,
    fastest: null,
    fastestUS: null,
    fastestUK: null,
    custom1: null,
    custom2: null,
    favorites: {},
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
      this.props.onOtherClick();
    } else {
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

  makeRegionList(regions, locations) {
    const Header = ({ name, ...props }) => <div className="header" {...props}>{name}</div>;
    const Server = ({ location, type }) => {
      const clickable = type !== 'header';
      const key = type + '-' + location.id;
      const ping = this.state.pingStats && this.state.pingStats[location.id];
      var onclick = event => {
        var value = event.currentTarget.getAttribute('data-value');
        if (event.target.className.indexOf('cp-fav') != -1) {
          this.onLocationFavoriteClick(value);
        } else if (event.currentTarget.className.indexOf('disabled') == -1) {
          this.onLocationClick(value);
        }
      };
      if (clickable)
        return <Location location={location} key={key} className="region" onClick={onclick} selected={this.state.selected === location.id} favorite={!!this.state.favorites[location.id]} ping={ping}/>;
      else
        return <Location location={location} key={key} className="region"/>;
    };
    const mapSort = (a, b, map) => map(a).localeCompare(map(b));

    var items = Array.flatten(
      this.state.regionOrder.map(g => ({
        id: g,
        name: this.state.regionNames[g],
        locations:
          Array.flatten(
            Object.mapToArray(regions[g], (c,l) => [c,l]) // get all countries of region as well as list of location IDs for each
              .sort((a, b) => mapSort(a, b, v => this.state.countryNames[v[0]])) // sort by country name
              .map(([country, locs]) => // project to list of <Location> elements, sorted by name
                locs
                  .map(l => locations[l])
                  .sort((a, b) => mapSort(a, b, v => v.name))
                  .map(l => Server({ location: l, type: 'location' }))
              )
              .filter(l => l && l.length > 0) // filter out empty countries
          )
      }))
        .filter(r => r.locations && r.locations.length > 0) // filter out empty regions
        .map(r => [ <Header key={'region-' + r.id.toLowerCase()} name={r.name}/> ].concat(r.locations)) // project to element list containing region header and locations
    );

    var recent = Object.keys(this.state.lastConnected).sort((a, b) => this.state.lastConnected[b] - this.state.lastConnected[a]).filter(l => !this.state.favorites[l] && this.state.locations[l]);
    if (recent.length > 0) {
      // Prepend recent list
      items = [ <Header key="recent" name="Recent"/> ].concat(recent.slice(0, 3).map(l => Server({ location: locations[l], type: 'recent' })), items);
    }
    var favorites = Object.keys(this.state.favorites).filter(f => this.state.favorites[f] && locations[f]);
    if (favorites.length > 0) {
      // Prepend favorites list
      items = [ <Header key="favorites" name="Favorites"/> ].concat(favorites.sort((a, b) => mapSort(a, b, v => locations[v].name)).map(l => Server({ location: locations[l], type: 'favorite' })), items);
    }
    return items;
  }

  render() {
    let cypherPlayDisabledWarning = {};
    if (!this.state.overrideDNS) {
      cypherPlayDisabledWarning = {
        'data-tooltip': "CypherPlay is unavailable due to compatibility settings",
        'data-position': "top left"
      };
    }
    let connectString;
    switch (this.state.state) {
      default:
      case 'DISCONNECTED': connectString = "CONNECT TO"; break;
      case 'CONNECTING': connectString = "CONNECTING TO"; break;
      case 'CONNECTED': connectString = "CONNECTED TO"; break;
      case 'DISCONNECTING': connectString = "DISCONNECTING FROM"; break;
    }
    let Button = ({ className, index, disabled, ...props } = {}) => <div className={classList(className, { "selected": this.state.selected === index, "disabled": disabled })} tabIndex={disabled || this.props.expanded ? -1 : 0} onClick={e => !disabled && this.onClick(index, e)} {...props}/>;
    return(
      <div className={classList("quick-panel", { "location-list-open": this.props.expanded })}>
        <div className="drawer">
          <div className="description">{connectString}</div>
          <Location className="selected-location" location={this.state.locations[this.state.location]}/>
          <div className="list">
            <div className="locations">
              {this.makeRegionList(this.state.regions, this.state.locations)}
            </div>
          </div>
        </div>
        <div className="grid">
          <Button index={0} className="cypherplay" disabled={!this.state.fastest | !this.state.overrideDNS} {...cypherPlayDisabledWarning}>
            <RetinaImage src={CypherPlayIcon}/><span>CypherPlay&trade;</span>
          </Button>
          <Button index={1} className="fastest" disabled={!this.state.fastest}>
            <RetinaImage src={FastestIcon}/><span>Fastest</span>
          </Button>
          <Button index={2} className="fastest" disabled={!this.state.fastest}>
            <Flag country="us"/><span>Fastest US</span>
          </Button>
          <Button index={3} className="fastest" disabled={!this.state.fastest}>
            <Flag country="gb"/><span>Fastest UK</span>
          </Button>
          <Button index={4} className="favorite" disabled={true}>
          </Button>
          <Button index={5} className="favorite" disabled={true}>
          </Button>
          <Button index={6} className="other">
            <i className="horizontal ellipsis icon"/><span>Other</span>
          </Button>
        </div>
      </div>
    );
  }
}

export default QuickPanel;
