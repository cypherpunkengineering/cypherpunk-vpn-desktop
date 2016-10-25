import React from 'react';
import ReactDOM from 'react-dom';
import { MainTitlebar } from './Titlebar';
import MainBackground from './MainBackground';
import OneZeros from './OneZeros';
import Dragbar from './Dragbar';
import daemon from '../daemon';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER } from '../util';
import RouteTransition from './Transition';

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

export default class ConnectScreen extends React.Component {
  constructor(props) {
    super(props);
    Object.assign(this.state, this.translateDaemonState(daemon.state));

    this.handleConnectClick = this.handleConnectClick.bind(this);
    this.handleRegionSelect = this.handleRegionSelect.bind(this);
    this.handleDaemonSettingsChange = this.handleDaemonSettingsChange.bind(this);
    this.handleDaemonStateChange = this.handleDaemonStateChange.bind(this);
  }

  state = {
    regions: daemon.config.regions,
    selectedRegion: daemon.settings.server,
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
    if (settings.server) {
      this.setState({ selectedRegion: settings.server });
      $(this.refs.regionDropdown).dropdown('set selected', settings.server);
    }
  }
  handleDaemonStateChange(state) {
    this.setState(this.translateDaemonState(state));
  }
  componentDidMount() {
    daemon.on('settings', this.handleDaemonSettingsChange);
    daemon.on('state', this.handleDaemonStateChange);
    $(this.refs.regionDropdown).dropdown({
      direction: 'upward',
      onChange: this.handleRegionSelect
    });
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
      transform: 'rotate(-45deg)',
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

    var regions = Array.flatten(
      REGION_GROUP_ORDER.map(g => ({
        name: REGION_GROUP_NAMES[g],
        locations: Object.mapToArray(this.state.regions[g], (country, locations) => locations.map(s => daemon.config.servers[s]).map(s => <div class={"item" + (s.ovDefault == "255.255.255.255" ? " disabled" : "")} data-value={s.id} key={s.id}><i class={s.country.toLowerCase() + " flag"}></i>{s.regionName}<i class="cp-fav icon"></i></div>)).filter(l => l && l.length > 0)
      })).filter(r => r.locations && r.locations.length > 0).map(r => [ <div key={"region-" + r.name} class="header">{r.name}</div> ].concat(r.locations))
    );

    return(
      <RouteTransition transition="reveal">
        {this.props.children || null}
        <div id="connect-screen" key="self" class="screen">
          <MainTitlebar/>
          <OneZeros/>
          <MainBackground/>
          <div id="connect-container" onClick={this.handleConnectClick}>

            <svg width="120" height="400" ref="connectButton">
              <circle cx="60" cy="200" r="50" style={connectCircleStyle} />
              <line x1="60" y1="98" x2="60" y2="181" style={connectLineStyle} />
              <line x1="60" y1="111" x2="80" y2="111"  style={connectSmallLineStyle} />
              <line x1="60" y1="137" x2="80" y2="137"  style={connectSmallLineStyle} />
            </svg>

            {/*<i id="connect" class={"ui fitted massive power link icon" + (this.state.connectionState === 'connected' ? " green" : this.state.connectionState == 'disconnected' ? " red" : " orange disabled")} ref="connectButton" onClick={this.handleConnectClick}></i>*/}
          </div>
          <div id="connect-status" ref="connectStatus">{buttonLabel}</div>
          <div id="region-select" class={"ui selection dropdown" + (this.state.connectionState === 'disconnected' ? "" : " disabled")} ref="regionDropdown">
            <input type="hidden" name="region" value={this.state.selectedRegion}/>
            <i class="dropdown icon"></i>
            <div class="default text">Select Region</div>
            <div class="menu">
              { regions }
            </div>
          </div>
          <div id="connection-stats" class="ui two column center aligned grid">
            <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.receivedBytes)}</div><div class="label">Received</div></div></div>
            <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.sentBytes)}</div><div class="label">Sent</div></div></div>
          </div>
        </div>
      </RouteTransition>
    );
  }
  handleConnectClick() {
    switch (this.state.connectionState) {
      case 'disconnected':
        this.setState({ connectionState: 'connecting' });
        daemon.post.applySettings({
          protocol: "udp",
          server: this.getSelectedRegion(),
        });
        daemon.post.connect();
        daemon.post.get('state');
        break;
      case 'connecting':
      case 'connected':
        this.setState({ connectionState: 'disconnecting' });
        daemon.post.disconnect();
        daemon.post.get('state');
        break;
      case 'disconnecting':
        break;
    }
  }
  getSelectedRegion() {
    return $(this.refs.regionDropdown).dropdown('get value');
  }
  handleRegionSelect(server) {
    daemon.post.applySettings({ server: server });
  }
}
