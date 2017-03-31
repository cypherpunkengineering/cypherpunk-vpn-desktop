import React from 'react';
import ReactDOM from 'react-dom';
import daemon, { DaemonAware } from '../daemon';
import { Location } from './RegionSelector';
import RetinaImage, { Flag } from './Image';
import { classList } from '../util.js';

const CypherPlayIcon = { 1: require('../assets/img/icon_cypherplay.png'), 2: require('../assets/img/icon_cypherplay@2x.png') };
const FastestIcon = { 1: require('../assets/img/icon_fastest.png'), 2: require('../assets/img/icon_fastest@2x.png') };

const Header = ({ name, count = null, ...props }) => <div className="header" data-count={count} {...props}>{name}</div>;

export class QuickPanel extends DaemonAware(React.Component) {

  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      config: { locations: true, regions: true, countryNames: true, regionNames: true, regionOrder: true },
      settings: { location: true, locationFlag: true, favorites: v => ({ favorites: Array.toDict(v, f => f, f => true) }), lastConnected: true, overrideDNS: true },
      state: { state: true, connect: true, pingStats: true },
    });
  }
  static defaultProps = {
    expanded: false,
    onOtherClick: function() {},
    onListCloseClick: function() {},
    onLocationClick: function(value) {},
    onLocationFavoriteClick: function(value) {},
  }
  state = {
    selected: 6,
    fastest: null,
    fastestUS: null,
    fastestUK: null,
    custom1: null,
    custom2: null,
    favorites: {},
    sortOrder: 'geographical', // or 'bypingtime' or 'alphabetical'
    filterText: '',
  }
  daemonDataChanged(state) {
    let fastest = null, fastestUS = null, fastestUK = null;
    let locations = state.locations;
    Object.forEach(state.pingStats, (id, ping) => {
      if (id === 'updating' || !locations[id] || locations[id].region === 'DEV') return;
      if (ping.replies) {
        if (locations[id] && (!fastest || ping.average < state.pingStats[fastest].average)) fastest = id;
        if (locations[id] && locations[id].country.toLowerCase() == 'us' && (!fastestUS || ping.average < state.pingStats[fastestUS].average)) fastestUS = id;
        if (locations[id] && locations[id].country.toLowerCase() == 'gb' && (!fastestUK || ping.average < state.pingStats[fastestUK].average)) fastestUK = id;
      }
    });
    let ping = (l) => state.pingStats[l] && state.pingStats[l].replies && state.pingStats[l].average || 999;
    let sort = (a, b) => (
      //(state.favorites[b] - state.favorites[a]) ||
      (state.lastConnected[b] - state.lastConnected[a]) ||
      (ping(a) - ping(b))
    );
    let custom = Object.keys(state.locations)
      .filter(l => state.favorites[l] /*|| state.lastConnected[l] || ping(l) != 999*/)
      .sort(sort);
    let { custom1, custom2 } = state;
    if (custom1 === custom[1]) {
      custom2 = custom[0];
    } else if (custom2 === custom[0]) {
      custom1 = custom[1];
    } else {
      [ custom1, custom2 ] = custom;
    }
    if (!custom2) { // right align if only one favorite
      custom2 = custom1;
      custom1 = null;
    }

    let selected = 6; // other
    switch (state.locationFlag) {
      case 'cypherplay': if (state.overrideDNS) { selected = 0; break; } // fallthrough
      case 'fastest': selected = 1; break;
      case 'fastest-us': selected = 2; break;
      case 'fastest-uk': selected = 3; break;
      default:
        if (state.location === custom1) selected = 4;
        else if (state.location === custom2) selected = 5;
        break;
    }
    return { fastest, fastestUS, fastestUK, custom1, custom2, selected };
  }

  onClick(button, event) {
    if (button === 6) {
      this.props.onOtherClick();
    } else {
      let settings = { locationFlag: '', suppressReconnectWarning: true };
      switch (button) {
        case 0: settings.location = this.state.fastest; settings.locationFlag = 'cypherplay'; break;
        case 1: settings.location = this.state.fastest; settings.locationFlag = 'fastest'; break;
        case 2: settings.location = this.state.fastestUS; settings.locationFlag = 'fastest-us'; break;
        case 3: settings.location = this.state.fastestUK; settings.locationFlag = 'fastest-uk'; break;
        case 4: settings.location = this.state.custom1; break;
        case 5: settings.location = this.state.custom2; break;
      }
      if (settings.hasOwnProperty('location') && !settings.location) {
        console.error("Can't connect - no matching server");
        return;
      }
      daemon.call.applySettings(settings).then(() => {
        daemon.post.connect();
      });
    }
  }

  makeRegionList(regions, locations) {
    const Server = ({ location, type }) => {
      const clickable = type !== 'header';
      const key = type + '-' + location.id;
      const ping = this.state.pingStats && this.state.pingStats[location.id];
      var onclick = event => {
        var value = event.currentTarget.getAttribute('data-value');
        if (event.target.className.indexOf('cp-fav') != -1) {
          this.props.onLocationFavoriteClick(value);
        } else if (event.currentTarget.className.indexOf('disabled') == -1) {
          this.props.onLocationClick(value);
        }
      };
      if (clickable)
        return <Location location={location} key={key} className="region" onClick={onclick} selected={this.state.location === location.id} favorite={!!this.state.favorites[location.id]} ping={ping} favorite={!!this.state.favorites[location.id]}/>;
      else
        return <Location location={location} key={key} className="region"/>;
    };
    const mapSort = (a, b, map) => map(a).localeCompare(map(b));

    let items;
    let sorter = (a, b) => mapSort(a, b, v => v.name);
    let ping = l => this.state.pingStats[l.id] && this.state.pingStats[l.id].average || 999;
    if (this.state.sortOrder === 'alphabetical') {
      items = Object.values(locations)
        .sort(sorter)
        .map(l => Server({ location: l, type: 'location' }));
      if (items.length > 0) items.unshift(<Header key="alphabetical" name="Alphabetical"/>);
    } else if (this.state.sortOrder === 'bypingtime') {
      sorter = (a, b) => (ping(a) - ping(b)) || mapSort(a, b, v => v.name);
      items = Object.values(locations)
        .sort(sorter)
        .map(l => Server({ location: l, type: 'location' }));
      if (items.length > 0) items.unshift(<Header key="bypingtimes" name="Fastest"/>);
    } else {
      items = Array.flatten(
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
                    .filter(l => l)
                    .sort((a, b) => mapSort(a, b, v => v.name))
                    .map(l => Server({ location: l, type: 'location' }))
                )
                .filter(l => l && l.length > 0) // filter out empty countries
            )
        }))
          .filter(r => r.locations && r.locations.length > 0) // filter out empty regions
          .map(r => [ <Header key={'region-' + r.id.toLowerCase()} name={r.name}/> ].concat(r.locations)) // project to element list containing region header and locations
      );
    }
    var recent = Object.keys(this.state.lastConnected).filter(l => !this.state.favorites[l] && locations[l] && this.state.lastConnected[l]).sort((a, b) => (this.state.lastConnected[b] - this.state.lastConnected[a]) || mapSort(a, b, v => locations[v]));
    if (recent.length > 0) {
      // Prepend recent list
      items = [ <Header key="recent" name="Recent"/> ].concat(recent.slice(0, 3).map(l => Server({ location: locations[l], type: 'recent' })), items);
    }
    var favorites = Object.keys(this.state.favorites).filter(f => this.state.favorites[f] && locations[f]);
    if (favorites.length > 0) {
      // Prepend favorites list
      items = [ <Header key="favorites" name="Favorites"/> ].concat(favorites.map(f => locations[f]).sort(sorter).map(l => Server({ location: l, type: 'favorite' })), items);
    }
    // Return placeholder if empty list
    if (items.length == 0) {
      items = [ <div key="empty" className="empty">No locations found.</div> ];
    }
    return items;
  }

  Button = ({ className, index, disabled, updating, ...props } = {}) => <div className={classList(className, { "selected": this.state.selected === index, "disabled": disabled, "updating": updating })} tabIndex={disabled || this.props.expanded ? -1 : 0} onClick={e => !disabled && this.onClick(index, e)} {...props}/>;
  render() {
    let cypherPlayDisabledWarning = {};
    if (!this.state.overrideDNS) {
      cypherPlayDisabledWarning = {
        'data-tooltip': "CypherPlay is unavailable due to expert settings",
        'data-position': "top left"
      };
    }
    let connectString;
    switch (this.state.state) {
      default:
      case 'DISCONNECTED': connectString = "CONNECT TO"; break;
      case 'SWITCHING':
      case 'CONNECTING': connectString = "CONNECTING TO..."; break;
      case 'CONNECTED': connectString = "CONNECTED TO"; break;
      case 'DISCONNECTING': connectString = "DISCONNECTING FROM..."; break;
    }
    if (this.props.expanded) connectString = "CONNECT TO";
    let filterText = this.state.filterText ? this.state.filterText.toLocaleLowerCase() : null;
    let locationList = this.makeRegionList(this.state.regions, filterText ? Object.filter(this.state.locations, (id, location) => {
      if (location.name.toLocaleLowerCase().includes(filterText)) return true;
      if (this.state.countryNames[location.country].toLocaleLowerCase().includes(filterText)) return true;
      if (this.state.regionNames[location.region].toLocaleLowerCase().includes(filterText)) return true;
      return false;
    }) : this.state.locations);
    const Button = this.Button;
    return(
      <div className={classList("quick-panel", { "location-list-open": this.props.expanded, 'connected': this.state.state === 'CONNECTED' })}>
        <div className="drawer">
          <div className="dimmer" onClick={this.props.onListCloseClick}/>
          <div className="description">{connectString}</div>
          <div className="header">
            <Location className="selected-location" location={this.state.locations[this.state.location]}/>
            <div className="list-header">
              <div className="ui left icon input"><input name="filter-locations" type="text" size="5" value={this.state.filterText} onChange={e => this.setState({ filterText: e.target.value})}/><i className="search icon"/></div>
              <i className={classList("world icon link", { "selected": this.state.sortOrder === 'geographical' })} onClick={() => this.setState({ sortOrder: 'geographical' })}/>
              <i className={classList("dashboard icon link", { "selected": this.state.sortOrder === 'bypingtime' })} onClick={() => this.setState({ sortOrder: 'bypingtime' })}/>
              <i className={classList("sort alphabet ascending icon link", { "selected": this.state.sortOrder === 'alphabetical' })} onClick={() => this.setState({ sortOrder: 'alphabetical' })}/>
              <i className="down chevron icon link" onClick={this.props.onListCloseClick}/>
            </div>
          </div>
          <div className="list">
            <div className="locations">
              {locationList}
            </div>
          </div>

          <div className="grid">
            <Button index={0} className="cypherplay" disabled={!this.state.fastest | !this.state.overrideDNS} updating={this.state.pingStats.updating} {...cypherPlayDisabledWarning}>
              <RetinaImage src={CypherPlayIcon}/><span>CypherPlay&trade;</span>
            </Button>
            <Button index={1} className="fastest" disabled={!this.state.fastest} updating={this.state.pingStats.updating}>
              <RetinaImage src={FastestIcon}/><span>Fastest</span>
            </Button>
            <Button index={2} className="fastest" disabled={!this.state.fastest} updating={this.state.pingStats.updating}>
              <Flag country="us"/><span>Fastest US</span>
            </Button>
            <Button index={3} className="fastest" disabled={!this.state.fastest} updating={this.state.pingStats.updating}>
              <Flag country="gb"/><span>Fastest UK</span>
            </Button>
            <Button index={4} className="favorite" disabled={!this.state.custom1}>
              {this.state.custom1 && <Flag country={this.state.locations[this.state.custom1].country.toLowerCase()}/>}
              {this.state.custom1 && <span>{this.state.locations[this.state.custom1].name.replace(/,[^,]*$/, '')}</span>}
            </Button>
            <Button index={5} className="favorite" disabled={!this.state.custom2}>
              {this.state.custom2 && <Flag country={this.state.locations[this.state.custom2].country.toLowerCase()}/>}
              {this.state.custom2 && <span>{this.state.locations[this.state.custom2].name.replace(/,[^,]*$/, '')}</span>}
            </Button>
            <Button index={6} className="other">
              <i className="world icon"/><span>Other</span>
            </Button>
          </div>

        </div>
      </div>
    );
  }
}

export default QuickPanel;
