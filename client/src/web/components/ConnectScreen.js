import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { MainTitlebar, Dragbar, Titlebar, Title } from './Titlebar';
import MainBackground from './MainBackground';
import OneZeros from './OneZeros';
import daemon, { DaemonAware } from '../daemon';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER, classList } from '../util';
import RouteTransition from './Transition';
import RegionSelector from './RegionSelector';
import ReconnectButton from './ReconnectButton';
import { OverlayContainer } from './Overlay';
import RetinaImage from './Image';
import QuickPanel from './QuickPanel';
import WorldMap from './WorldMap';
import LocationList, { Location, CypherPlayItem } from './LocationList';

const transitionMap = {
    '/tutorial/*': {
      null: 'fadeIn',
    },
    '*': {
      '/tutorial/*': 'fadeIn',
      '*': 'reveal',
    },
};

const GPS = {
  'cypherplay': { lat: -30, long: 45, scale: 0.25 },
  'amsterdam': { lat: -52.3702, long: 4.8952, scale: 1.5 },
  'atlanta': { lat: -33.7490, long: -84.3880, scale: 1 },
  'chennai': { lat: -13.0827, long: 80.2707, scale: 1 },
  'chicago': { lat: -41.8781, long: -87.6298, scale: 1 },
  'dallas': { lat: -32.7767, long: -96.7970, scale: 1 },
  'devhonolulu': { lat: -21.3069, long: -157.8533, scale: 4.0 },
  'devkim': { lat: -35.6895, long: 139.6917, scale: 1.5 },
  'devtokyo1': { lat: -35.6895, long: 139.6917, scale: 1.5 },
  'devtokyo3': { lat: -35.6895, long: 139.6917, scale: 1.5 },
  'devtokyo4': { lat: -35.6895, long: 139.6917, scale: 1.5 },
  'frankfurt': { lat: -50.1109, long: 8.6821, scale: 1.5 },
  'hongkong': { lat: -22.3964, long: 114.1095, scale: 1.5 },
  'istanbul': { lat: -41.0082, long: 28.9784, scale: 1.5 },
  'london': { lat: -51.5074, long: 0.1278, scale: 1.5 },
  'losangeles': { lat: -34.0522, long: -118.2437, scale: 1 },
  'melbourne': { lat: 37.8136, long: 144.9631, scale: 1 },
  'miami': { lat: -25.6717, long: -80.1918, scale: 1 },
  'milan': { lat: -45.4654, long: 9.1859, scale: 1.5 },
  'montreal': { lat: -45.5017, long: -73.5673, scale: 1 },
  'moscow': { lat: -55.7558, long: 37.6173, scale: 1 },
  'newjersey': { lat: -40.0583, long: -74.4057, scale: 1 },
  'newyork': { lat: -40.7128, long: -74.0059, scale: 1 },
  'oslo': { lat: -59.9139, long: 10.7522, scale: 1.5 },
  'paris': { lat: -48.8566, long: 2.3522, scale: 1.5 },
  'phoenix': { lat: -33.4484, long: -112.0740, scale: 1 },
  'saltlakecity': { lat: -40.7608, long: -111.8910, scale: 1 },
  'saopaulo': { lat: 23.5505, long: -46.6333, scale: 1 },
  'seattle': { lat: -47.6062, long: -122.3321, scale: 1 },
  'siliconvalley': { lat: -37.3875, long: -122.0575, scale: 1 },
  'singapore': { lat: -1.3521, long: 103.8198, scale: 1.5 },
  'stockholm': { lat: -59.3293, long: 18.0686, scale: 1.5 },
  'sydney': { lat: 33.8688, long: 151.2093, scale: 1 },
  'tokyo': { lat: -35.6895, long: 139.6917, scale: 1.5 },
  'toronto': { lat: -43.6532, long: -79.3832, scale: 1 },
  'vancouver': { lat: -49.2827, long: -123.1207, scale: 1 },
  'washingtondc': { lat: -38.9072, long: -77.0369, scale: 1 },
  'zurich': { lat: -47.3769, long: 8.5417, scale: 1.5 },
};

const AccountIcon = { [1]: require('../assets/img/account_icon.png'), [2]: require('../assets/img/account_icon@2x.png') };
const CypherPlayIcon = { [1]: require('../assets/img/icon_cypherplay.png'), [2]: require('../assets/img/icon_cypherplay@2x.png') };

const Header = ({ name, count = null, ...props }) => <div className="header" data-count={count} {...props}>{name}</div>;

const Server = ({ location, type, selected = false, favorites = null, pingStats = null, onLocationClick = null, onLocationFavoriteClick = null, onMouseEnter = null }) => {
  const clickable = type !== 'header';
  const key = type + '-' + location.id;
  const ping = pingStats && pingStats[location.id];
  var onclick = event => {
    var value = event.currentTarget.getAttribute('data-value');
    if (event.target.className.indexOf('cp-fav') != -1) {
      onLocationFavoriteClick && onLocationFavoriteClick(value);
    } else if (event.currentTarget.className.indexOf('disabled') == -1) {
      onLocationClick && onLocationClick(value);
    }
  };
  if (clickable)
    return <Location location={location} key={key} onMouseEnter={onMouseEnter} onClick={onclick} selected={selected} favorite={favorites && !!favorites[location.id]} ping={ping}/>;
  else
    return <Location location={location} key={key} onMouseEnter={onMouseEnter}/>;
};

const mapSort = (a, b, map) => map(a).localeCompare(map(b));

export function makeRegionList({ regions, locations, regionOrder, regionNames, countryNames, favorites = null, lastConnected = null, location = null, pingStats = null, sortOrder = null, onLocationClick = null, onLocationFavoriteClick = null, onMouseEnter = null }) {
  let items;
  let sorter = (a, b) => mapSort(a, b, v => v.name);
  let ping = l => pingStats && pingStats[l.id] && pingStats[l.id].average || 999;
  if (sortOrder === 'alphabetical') {
    items = Object.values(locations)
      .sort(sorter)
      .map(l => Server({ location: l, type: 'location', selected: location === l.id, pingStats, onLocationClick, onLocationFavoriteClick, onMouseEnter }));
    if (items.length > 0) items.unshift(<Header key="alphabetical" name="Alphabetical"/>);
  } else if (sortOrder === 'bypingtime') {
    sorter = (a, b) => (ping(a) - ping(b)) || mapSort(a, b, v => v.name);
    items = Object.values(locations)
      .sort(sorter)
      .map(l => Server({ location: l, type: 'location', selected: location === l.id, pingStats, onLocationClick, onLocationFavoriteClick, onMouseEnter }));
    if (items.length > 0) items.unshift(<Header key="bypingtimes" name="Fastest"/>);
  } else { // sortOrder === 'geographical'
    items = Array.flatten(
      regionOrder.map(g => ({
        id: g,
        name: regionNames[g],
        locations:
          Array.flatten(
            Object.mapToArray(regions[g], (c,l) => [c,l]) // get all countries of region as well as list of location IDs for each
              .sort((a, b) => mapSort(a, b, v => countryNames[v[0]])) // sort by country name
              .map(([country, locs]) => // project to list of <Location> elements, sorted by name
                locs
                  .map(l => locations[l])
                  .filter(l => l)
                  .sort((a, b) => mapSort(a, b, v => v.name))
                  .map(l => Server({ location: l, type: 'location', selected: location === l.id, pingStats, onLocationClick, onLocationFavoriteClick, onMouseEnter }))
              )
              .filter(l => l && l.length > 0) // filter out empty countries
          )
      }))
        .filter(r => r.locations && r.locations.length > 0) // filter out empty regions
        .map(r => [ <Header key={'region-' + r.id.toLowerCase()} name={r.name} data-value={'region-' + r.id.toLowerCase()} onMouseEnter={onMouseEnter}/> ].concat(r.locations)) // project to element list containing region header and locations
    );
  }
  if (lastConnected) {
    let recent = Object.keys(lastConnected).filter(l => (!favorites || !favorites[l]) && locations[l] && lastConnected[l]).sort((a, b) => (lastConnected[b] - lastConnected[a]) || mapSort(a, b, v => locations[v]));
    if (recent.length > 0) {
      // Prepend recent list
      items = [ <Header key="recent" name="Recent"/> ].concat(recent.slice(0, 3).map(l => Server({ location: locations[l], type: 'recent', selected: location === l.id, pingStats, onLocationClick, onLocationFavoriteClick, onMouseEnter })), items);
    }
  }
  if (favorites) {
    let favItems = Object.keys(favorites).filter(f => favorites[f] && locations[f]);
    if (favItems.length > 0) {
      // Prepend favorites list
      items = [ <Header key="favorites" name="Favorites"/> ].concat(favItems.map(f => locations[f]).sort(sorter).map(l => Server({ location: l, type: 'favorite', selected: location === l.id, pingStats, onLocationClick, onLocationFavoriteClick, onMouseEnter })), items);
    }
  }
  // Return placeholder if empty list
  if (items.length == 0) {
    items = [ <div key="empty" className="empty">No locations found.</div> ];
  }
  return items;
}


function humanReadableSize(count) {
  if (count >= 1024 * 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024 / 1024) / 10).toFixed(1) + "T";
  } else if (count >= 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024) / 10).toFixed(1) + "G";
  } else if (count >= 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024) / 10).toFixed(1) + "M";
  } else if (count >= 1024) {
    return parseFloat(Math.round(count * 10 / 1024) / 10).toFixed(1) + "K";
  } else {
    return parseFloat(count).toFixed(0);
  }
}

class FirewallWarning extends DaemonAware(React.Component) {
  constructor() {
    super();
    this.state.visible = this.shouldDisplay();
  }
  state = {
    visible: false,
  }
  daemonSettingsChanged(settings) {
    if (settings.firewall) this.onChange();
  }
  daemonStateChanged(state) {
    if (state.state) this.onChange();
  }
  shouldDisplay() {
    return daemon.settings.firewall === 'on' && (!daemon.state.connect || daemon.state.state === 'DISCONNECTED');
  }
  onChange() {
    var visible = this.shouldDisplay();
    if (visible != this.state.visible) this.setState({ visible: visible });
  }
  disableFirewall() {
    // TODO: Maybe take the user to the firewall menu instead
    daemon.post.applySettings({ firewall: 'auto' });
  }
  render() {
    return(
      <div id="firewall-warning"
        className={this.state.visible?"visible":""}
        data-tooltip="The Internet Killswitch is active, and is preventing you from accessing the internet. Click here to recover connectivity."
        data-inverted="" data-position="bottom right"
        onClick={() => this.disableFirewall()}>
        <i className="warning sign icon"></i><span>Killswitch active</span>
      </div>
    );
  }
}

const PIPE_UPPER_TEXT = 'x`8 0 # = v 7 mb" | y 9 # 8 M } _ + kl $ #mn x -( }e f l]> ! 03 @jno x~`.xl ty }[sx k j';
const PIPE_LOWER_TEXT = 'dsK 7 & [*h ^% u x 5 8 00 M< K! @ &6^d jkn 70 :93jx p0 bx, 890 Qw ;é " >?7 9 3@ { 5x3 >';

class ConnectButton extends React.Component {
  static defaultProps = {
    upper: PIPE_UPPER_TEXT,
    lower: PIPE_LOWER_TEXT,
    on: false,
    connectionState: 'disconnected',
    hidden: false,
    onClick: function() {}
  }
  onClick(e) {
    this.props.onClick();
    this.dom.blur();
  }
  onKeyDown(e) {
    switch (e.key) {
      case 'ArrowLeft': case 'ArrowRight':
        if (!this.props.on == (e.key === 'ArrowLeft')) break;
        // fallthrough
      case ' ': case 'Enter':
        this.props.onClick();
        break;
    }
  }
  render() {
    return (
      <div className={classList("connect-button", { 'on': this.props.on, 'off': !this.props.on, 'hidden': this.props.hidden }, this.props.connectionState)} onClick={e => this.onClick(e)} onKeyDown={e => this.onKeyDown(e)} tabIndex={this.props.hidden ? -1 : 0}>
        <div className="bg">
          <div className="pipe"/>
          <div className="dot"/>
          <div className="row1" style={{ animationDuration: (this.props.upper.length*300)+'ms' }}>{this.props.upper}{this.props.upper}</div>
          <div className="row2" style={{ animationDuration: (this.props.lower.length*300)+'ms' }}>{this.props.lower}{this.props.lower}</div>
          <div className="frame"/>
        </div>
        <div className="slider"/>
        <div className="knob"/>
      </div>
    );
  }
}

export default class ConnectScreen extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      config: { locations: true, regions: true, countryNames: true, regionNames: true, regionOrder: true },
      settings: { location: true, locationFlag: true, favorites: v => ({ favorites: Array.toDict(v, f => f, f => true) }), lastConnected: true, overrideDNS: true },
      state: { state: v => ({ connectionState: v.toLowerCase().replace('switching', 'connecting') }), connect: true, pingStats: true, bytesReceived: true, bytesSent: true },
    });
  }

  state = {
    connectionState: 'disconnected',
    locationListOpen: false,
    mapLocation: null,
  }

  render() {
    let connectedLocation = 'stockholm';
    return(
      <RouteTransition transition={transitionMap}>
        <ReconnectButton key="reconnect"/>
        {this.props.children || null}
        <div id="connect-screen" key="self" class="screen">
          <div>
            <Titlebar>
              <Title/>
            </Titlebar>
            {/*<OneZeros/>*/}

            <Link className="left account page-link" to="/account" tabIndex="0" data-tooltip="My Account" data-position="bottom left"><RetinaImage src={AccountIcon}/></Link>
            <Link className="right settings page-link" to="/configuration" tabIndex="0" data-tooltip="Configuration" data-position="bottom right"><i className="settings icon"/></Link>

            <WorldMap locations={GPS} location={this.state.mapLocation || connectedLocation || this.state.location} className="side"/>

            <div className="location-list">
              <div className="header">
                { (connectedLocation) && <div className="title">Connected to</div> }
                { (connectedLocation) && <Location location={this.state.locations[connectedLocation]} hideTag={true}/> }
                { (connectedLocation) && <div className="title">Switch to</div> }
                { (!connectedLocation) && <div className="title">Connect to</div> }
              </div>
              <div className="list" onMouseLeave={() => { if (connectedLocation && this.state.mapLocation !== connectedLocation) this.setState({ mapLocation: connectedLocation }); }}>
                <div className="cypherplay" onMouseEnter={() => { if (this.state.mapLocation !== 'cypherplay') this.setState({ mapLocation: 'cypherplay' }); }}><RetinaImage src={CypherPlayIcon}/>CypherPlay&trade;<span>AUTO</span></div>
                {makeRegionList({
                  regions: this.state.regions,
                  locations: this.state.locations,
                  regionOrder: this.state.regionOrder,
                  regionNames: this.state.regionNames,
                  countryNames: this.state.countryNames,
                  favorites: this.state.favorites,
                  lastConnected: this.state.lastConnected,
                  location: this.state.location,
                  pingStats: this.state.pingStats,
                  onMouseEnter: e => {
                    let id = e.currentTarget.getAttribute('data-value');
                    if (id && !id.startsWith('region-') && this.state.mapLocation !== id) this.setState({ mapLocation: id });
                  }
                })}
              </div>
              <div className="footer">
                <span className="back"><i className="chevron left icon"/>Back</span>
              </div>
            </div>

            {/*
            <ConnectButton on={this.state.connect} connectionState={this.state.connectionState} onClick={() => this.handleConnectClick()} hidden={this.state.locationListOpen}/>

            <div className="connect-status">
              <span>Status</span>
              <span>{this.state.connectionState}</span>
            </div>

            <div className="location-selector">
              <Location location={this.state.locations[this.state.location]} hideTag={true}/>
            </div>
            */}

            <div id="connection-stats" class="ui two column center aligned grid">
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesReceived)}</div><div class="label">Received</div></div></div>
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesSent)}</div><div class="label">Sent</div></div></div>
            </div>
            <FirewallWarning/>
            {/*
            <QuickPanel
              expanded={this.state.locationListOpen}
              onOtherClick={() => this.setState({ locationListOpen: true })}
              onListCloseClick={() => this.setState({ locationListOpen: false })}
              onLocationClick={value => this.onLocationClick(value)}
              onLocationFavoriteClick={value => this.onLocationFavoriteClick(value)}
              />
            */}
          </div>
        </div>
        <OverlayContainer key="overlay"/>
      </RouteTransition>
    );
  }
  handleConnectClick() {
    switch (this.state.connectionState) {
      case 'disconnected':
        // Fake a connection state for now, as the daemon is too busy to report it back
        daemon.call.connect().catch(() => {
          alert("Connect failed; did you select a region?");
          daemon.post.get('state');
        });
        break;
      case 'connecting':
      case 'connected':
      case 'disconnecting':
        // Fake a connection state for now, as the daemon is too busy to report it back
        daemon.call.disconnect().catch(() => {
          daemon.post.get('state');
        });
        break;
    }
  }

  onLocationClick(value) {
    daemon.call.applySettings({ location: value, locationFlag: '' }).then(() => {
      daemon.post.connect();
    });
    this.setState({ locationListOpen: false });
  }
  onLocationFavoriteClick(value) {
    if (daemon.settings.favorites.includes(value))
      daemon.post.applySettings({ favorites: daemon.settings.favorites.filter(v => v !== value) });
    else
      daemon.post.applySettings({ favorites: daemon.settings.favorites.concat([ value ]) });
  }

}
