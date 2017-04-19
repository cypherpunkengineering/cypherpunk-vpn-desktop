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
  'cypherplay': { lat: -30, long: 0, scale: 0.5 },
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

            <WorldMap locations={GPS} location={this.state.mapLocation || (this.state .locationFlag === 'cypherplay' ? 'cypherplay' : this.state.location)} className={classList({ "side": this.state.locationListOpen })}/>

            <LocationList
              open={this.state.locationListOpen}
              onClick={id => this.onLocationClick(id)}
              onHover={id => (this.state.mapLocation !== id && this.setState({ mapLocation: id }))}
              onBack={() => this.setState({ locationListOpen: false, mapLocation: null })}
            />
            
            <ConnectButton
              on={this.state.connect}
              connectionState={this.state.connectionState}
              onClick={() => this.handleConnectClick()}
              hidden={this.state.locationListOpen}
            />

            <div className={classList("connect-status", { "hidden": this.state.locationListOpen })}>
              <span>Status</span>
              <span>{this.state.connectionState}</span>
            </div>

            <div className={classList("location-selector", { "hidden": this.state.locationListOpen })} onClick={() => this.setState({ locationListOpen: true })}>
              { this.state.locationFlag === 'cypherplay' ? <CypherPlayItem hideTag={true}/> : <Location location={this.state.locations[this.state.location]} hideTag={true}/> }
            </div>

            {/*}
            <div className="info-button"/>
            <div className="info-panel">
              <div className="graph">
                <svg width="100%" height="100%" viewBox="0 0 200 50" preserveAspectRatio="xMaxYMax meet">
                  <g id="graph-lines" transform="translate(200,50)">
                    <path id="graph-line-up" d={"M" + [54,36,75,74,72,68,58,60,40,36,20,32,34,37,42,40,32,21,18,16,16,16,9,6,3,0,0,0].map((x, i) => (-i*10)+","+(-x/2)).join("L")}/>
                    <path id="graph-line-down" d={"M" + [64,53,88,86,80,78,61,73,52,38,24,16,12,40,54,63,58,54,37,36,29,45,20,17,9,6,1,0].map((x, i) => (-i*10)+","+(-x/2)).join("L")}/>
                  </g>
                </svg>
              </div>
              <div className="stats">
                <span className="title">Down</span>
                <span className="down value">1.24 Mbps</span>
                <span className="title">Up</span>
                <span className="up value">560 kbps</span>
              </div>
            </div>
            {*/}

            {/*}
            <div id="connection-stats" class="ui two column center aligned grid">
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesReceived)}</div><div class="label">Received</div></div></div>
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesSent)}</div><div class="label">Sent</div></div></div>
            </div>
            <FirewallWarning/>
            {*/}
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
    if (!this.state.connect && this.state.connectionState === 'disconnected') {
      daemon.call.connect().catch(() => {
        alert("Connection failed; did you select a region?");
        daemon.post.get('state');
      });
    } else {
      daemon.call.disconnect().catch(() => {
        daemon.post.get('state');
      });
    }
  }

  onCypherPlayClick(fastest) {
    daemon.call.applySettings({ location: fastest, locationFlag: 'cypherplay', suppressReconnectWarning: true }).then(() => {
      daemon.post.connect();
    });
    this.setState({ locationListOpen: false, mapLocation: null });
  }
  onLocationClick(location) {
    if (location.startsWith('cypherplay:')) {
      return this.onCypherPlayClick(location.slice(11));
    }
    daemon.call.applySettings({ location: location.id, locationFlag: '', suppressReconnectWarning: true }).then(() => {
      daemon.post.connect();
    });
    this.setState({ locationListOpen: false, mapLocation: null });
  }
  onLocationFavoriteClick(value) {
    if (daemon.settings.favorites.includes(value))
      daemon.post.applySettings({ favorites: daemon.settings.favorites.filter(v => v !== value) });
    else
      daemon.post.applySettings({ favorites: daemon.settings.favorites.concat([ value ]) });
  }

}
