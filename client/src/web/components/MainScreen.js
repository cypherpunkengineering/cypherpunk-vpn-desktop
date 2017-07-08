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
import RetinaImage from './Image';
import QuickPanel from './QuickPanel';
import WorldMap from './WorldMap';
import LocationList, { Location, CypherPlayItem } from './LocationList';
import { Panel, PanelContent, PanelOverlay } from './Panel';
import { getAccountStatus } from './AccountScreen';


const CACHED_COORDINATES = {
  'cypherplay': { lat: 30, long: 0, scale: 0.5 },
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
    disabled: false,
    hidden: false,
    faded: false,
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
    let disabled = this.props.disabled || this.props.hidden || this.props.faded;
    return (
      <div className={classList("connect-button", { 'on': this.props.on, 'off': !this.props.on, 'hidden': this.props.hidden, 'faded': this.props.faded, 'disabled': this.props.disabled }, this.props.connectionState)} onClick={disabled ? null : e => this.onClick(e)} onKeyDown={disabled ? null : e => this.onKeyDown(e)} tabIndex={disabled ? -1 : 0}>
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

function getTransition(diff) {
  return Object.mapValues(diff, (key, type) => {
    if (key.startsWith('/tutorial')) return 'fadeIn';
    return 'reveal';
  });
}


function simplifyConnectionState(state) {
  switch (state) {
    case 'CONNECTING':
    case 'STILL_CONNECTING':
      return 'connecting';
    case 'CONNECTED':
      return 'connected';
    case 'INTERRUPTED':
    case 'RECONNECTING':
    case 'STILL_RECONNECTING':
    case 'DISCONNECTING_TO_RECONNECT':
      return 'reconnecting';
    case 'DISCONNECTING':
      return 'disconnecting';
    case 'DISCONNECTED':
      return 'disconnected';
  }
}

function describeConnectionState(state) {
  switch (state) {
    case 'CONNECTING':
    case 'STILL_CONNECTING':
      return "Connecting...";
    case 'CONNECTED':
      return "Connected";
    case 'INTERRUPTED':
      return "Interrupted";
    case 'RECONNECTING':
    case 'STILL_RECONNECTING':
    case 'DISCONNECTING_TO_RECONNECT':
      return "Reconnecting...";
    case 'DISCONNECTING':
      return "Disconnecting...";
    case 'DISCONNECTED':
      return "Disconnected";
  }
}


export default class ConnectScreen extends DaemonAware(React.Component) {
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      config: { locations: true, regions: true, countryNames: true, regionNames: true, regionOrder: true },
      settings: { location: true, locationFlag: true, favorites: v => ({ favorites: Array.toDict(v, f => f, f => true) }), lastConnected: true, overrideDNS: true, firewall: true },
      state: { state: v => ({ state: v, connectionState: simplifyConnectionState(v) }), connect: true, pingStats: true, bytesReceived: true, bytesSent: true },
    });
  }

  state = {
    connectionState: 'disconnected',
    locationListOpen: false,
    locationListSelection: null,
    mapLocation: null,
  }

  componentDidMount() {
    super.componentDidMount();
    this.tabBlocker = function(e) { if (e.key === 'Tab') { e.preventDefault(); e.stopPropagation(); return false; } };
    this.dom.addEventListener('keydown', this.tabBlocker, false);
  }
  componentWillUnmount() {
    super.componentWillUnmount();
    this.dom.removeEventListener('keydown', this.tabBlocker, false);
  }
  componentWillReceiveProps(props) {
    if (props.children && this.state.locationListOpen) {
      this.setState({ locationListOpen: false, mapLocation: null });
    }
  }

  render() {
    let panelOpen = !!this.props.children;
    let tabIndex = panelOpen ? "-1" : "0";
    let accountStatus = getAccountStatus();
    let connectionStatus = describeConnectionState(this.state.state);
    if (this.state.state === 'DISCONNECTED' && this.state.firewall === 'on') {
      connectionStatus = <span className="killswitch-warning">Killswitch Active <Link to="/configuration/firewall" data-tooltip="In order to preserve your privacy, your computer is being prevented from connecting to the internet. To regain connectivity, click here to access your settings." data-position="bottom right"><i class="info circle fitted icon"/></Link></span>;
    }
    switch (accountStatus) {
      case 'expired': connectionStatus = "Account Expired"; break;
      case 'invalid': connectionStatus = "Invalid Account"; break;
      case 'inactive': connectionStatus = "Inactive Account"; break;
    }
    return(
      <Panel id="main-screen-container" transition={getTransition}>
        <ReconnectButton key="reconnect"/>
        {this.props.children || null}
        <PanelContent key="self" id="connect-screen" className={classList({ "inert": panelOpen })}>
          <Titlebar><Title/></Titlebar>

          <Link className="left account page-link" to={panelOpen?"/main":"/account"} tabIndex={tabIndex} data-tooltip="My Account" data-position="bottom left"><RetinaImage src={AccountIcon}/></Link>
          <Link className="right settings page-link" to={panelOpen?"/main":"/configuration"} tabIndex={tabIndex} data-tooltip="Configuration" data-position="bottom right"><i className="settings icon"/></Link>

          <WorldMap locations={Object.assign(CACHED_COORDINATES, this.state.locations)} location={(this.state.locationListOpen ? this.state.mapLocation : (this.state.locationFlag === 'cypherplay' ? 'cypherplay' : this.state.location)) || 'cypherplay'} className={classList({ "side": this.state.locationListOpen })}/>

          <LocationList
            open={this.state.locationListOpen}
            selected={this.state.locationListSelection}
            onClick={id => this.onLocationClick(id)}
            onHover={id => (this.state.mapLocation !== id && this.setState({ mapLocation: id }))}
            onBack={() => this.setState({ locationListOpen: false, mapLocation: null })}
          />

          <ConnectButton
            on={this.state.connect}
            connectionState={this.state.connectionState}
            onClick={() => this.handleConnectClick()}
            hidden={this.state.locationListOpen}
            faded={accountStatus !== 'active'}
            disabled={accountStatus !== 'active'}
          />

          <div className={classList("connect-status", { "hidden": this.state.locationListOpen })}>
            <span>Status</span>
            <span>{connectionStatus}</span>
          </div>

          <div className={classList("location-selector", { "hidden": this.state.locationListOpen })} onClick={() => this.handleLocationClick()}>
            { this.state.locationFlag === 'cypherplay' ? <CypherPlayItem hideTag={true}/> : <Location location={this.state.locations[this.state.location]} hideTag={true}/> }
          </div>

        </PanelContent>
        <PanelOverlay key="overlay" onClick={() => History.push('/main')}/>
      </Panel>
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

  handleLocationClick() {
    let currentLocation = this.state.locationFlag === 'cypherplay' ? 'cypherplay' : this.state.location;
    let connectedLocation = this.state.connect && currentLocation || null;
    this.setState({ locationListOpen: true, locationListSelection: connectedLocation, mapLocation: currentLocation });
  }

  onCypherPlayClick(fastest) {
    daemon.call.applySettings({ location: fastest, fastest: fastest, locationFlag: 'cypherplay', suppressReconnectWarning: true }).then(() => {
      daemon.post.connect();
    });
    this.setState({ locationListOpen: false, mapLocation: null });
  }
  onLocationClick(location) {
    // FIXME: don't respond to clicks on disabled items
    if (location.startsWith('cypherplay:')) {
      return this.onCypherPlayClick(location.slice(11));
    }
    daemon.call.applySettings({ location: location, locationFlag: '', suppressReconnectWarning: true }).then(() => {
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
