import React from 'react';
import ReactDOM from 'react-dom';
import { MainTitlebar } from './Titlebar';
import MainBackground from './MainBackground';
import OneZeros from './OneZeros';
import { Dragbar } from './Titlebar';
import daemon, { DaemonAware } from '../daemon';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER } from '../util';
import RouteTransition from './Transition';
import RegionSelector from './RegionSelector';
import ReconnectButton from './ReconnectButton';
import { OverlayContainer } from './Overlay';

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

export default class ConnectScreen extends React.Component {
  constructor(props) {
    super(props);
    Object.assign(this.state, this.translateDaemonState(daemon.state));

    this.handleConnectClick = this.handleConnectClick.bind(this);
    this.handleDaemonSettingsChange = this.handleDaemonSettingsChange.bind(this);
    this.handleDaemonStateChange = this.handleDaemonStateChange.bind(this);
  }

  state = {
    regions: daemon.config.regions,
    selectedRegion: daemon.settings.location,
    receivedBytes: 0,
    sentBytes: 0,
    connectionState: 'disconnected',
  }

  translateDaemonState(state) {
    var newState = {};
    var stateString = {
      'CONNECTING': 'connecting',
      'CONNECTED': 'connected',
      'DISCONNECTING': 'disconnecting',
      'DISCONNECTED': 'disconnected',
      'SWITCHING': 'connecting',
    }[state.state];
    if (stateString)
      newState.connectionState = stateString;
    if (state.bytesReceived !== undefined)
      newState.receivedBytes = state.bytesReceived;
    if (state.bytesSent !== undefined)
      newState.sentBytes = state.bytesSent;
    return newState;
  }
  handleDaemonSettingsChange(settings) {
    if (settings.location) {
      this.setState({ selectedRegion: settings.location });
      $(this.refs.regionDropdown).dropdown('set selected', settings.location);
    }
  }
  handleDaemonStateChange(state) {
    this.setState(this.translateDaemonState(state));
  }
  componentDidMount() {
    daemon.on('settings', this.handleDaemonSettingsChange);
    daemon.on('state', this.handleDaemonStateChange);
  }
  componentWillUnmount() {
    daemon.removeListener('state', this.handleDaemonStateChange);
    daemon.removeListener('settings', this.handleDaemonSettingsChange);
  }
  componentWillUpdate(nextProps, nextState) {
    if (this.state.connectionState !== nextState.connectionState) {
      $(document.body).removeClass(this.state.connectionState).addClass(nextState.connectionState);
      if (nextState.connectionState === 'disconnected')
        $('#main-background').removeClass('animating');
      else
        $('#main-background').addClass('animating');
    }
  }
  render() {
    var connectCircleStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: (this.state.connectionState === 'connected' ? '1' : this.state.connectionState == 'disconnected' ? '1' : '0.75'),
      strokeWidth: '20',
      strokeDasharray: '314.16',
      strokeDashoffset: '78.54',
      strokeLinecap: 'round',
      transform: 'rotate3d(0,0,1,-45deg)',
      transformOrigin: '50% 200px',
      fill: 'none'
    }
    var connectLineStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: (this.state.connectionState === 'connected' ? '1' : this.state.connectionState == 'disconnected' ? '1' : '0.75'),
      strokeWidth: '20',
      strokeDasharray: '83',
      strokeDashoffset: (this.state.connectionState === 'connected' ? '0' : this.state.connectionState == 'disconnected' ? '127' : '83'),
      strokeLinecap: 'round',
      fill: 'none'
    }
    var connectSmallLineStyle = {
      stroke: (this.state.connectionState === 'connected' ? '#6ec90a' : this.state.connectionState == 'disconnected' ? '#cc0000' : '#d29f00'),
      opacity: '1',
      strokeWidth: '20',
      strokeDasharray: '20',
      strokeDashoffset: (this.state.connectionState === 'connected' ? '0' : this.state.connectionState == 'disconnected' ? '20' : '20'),
      strokeLinecap: 'round',
      fill: 'none'
    }
    var buttonLabel = {
      'disconnected': "Tap to protect",
      'connecting': "Connecting...",
      'connected': "You are protected",
      'disconnecting': "Disconnecting...",
    }[this.state.connectionState];

    return(
      <RouteTransition transition="reveal">
        <ReconnectButton key="reconnect"/>
        {this.props.children || null}
        <div id="connect-screen" key="self" class="screen">
          <div>
            <MainTitlebar/>
            <OneZeros/>
            {/* <MainBackground/> */}
            <div id="connect-container" onClick={this.handleConnectClick}>

              <svg viewBox="-40 40 200 240" preserveAspectRatio="xMidYMid meet" width="160px" height="200px" ref="connectButton">
                <circle class="ring" cx="60" cy="200" r="50" style={connectCircleStyle} />
                <line x1="60" y1="98" x2="60" y2="181" style={connectLineStyle} />
                <line x1="60" y1="111" x2="80" y2="111"  style={connectSmallLineStyle} />
                <line x1="60" y1="137" x2="80" y2="137"  style={connectSmallLineStyle} />
              </svg>

              {/*<i id="connect" class={"ui fitted massive power link icon" + (this.state.connectionState === 'connected' ? " green" : this.state.connectionState == 'disconnected' ? " red" : " orange disabled")} ref="connectButton" onClick={this.handleConnectClick}></i>*/}
            </div>
            <div id="connect-status" ref="connectStatus">{buttonLabel}</div>
            <div id="connection-stats" class="ui two column center aligned grid">
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.receivedBytes)}</div><div class="label">Received</div></div></div>
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.sentBytes)}</div><div class="label">Sent</div></div></div>
            </div>
            <FirewallWarning/>
            <RegionSelector/>
          </div>
        </div>
        <OverlayContainer/>
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
        // Fake a connection state for now, as the daemon is too busy to report it back
        daemon.call.disconnect().catch(() => {
          daemon.post.get('state');
        });
        break;
      case 'disconnecting':
        break;
    }
  }
  getSelectedRegion() {
    return $(this.refs.regionDropdown).dropdown('get value');
  }
}
