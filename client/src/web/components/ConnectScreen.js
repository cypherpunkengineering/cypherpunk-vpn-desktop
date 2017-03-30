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

const transitionMap = {
    '/tutorial/*': {
      null: 'fadeIn',
    },
    '*': {
      '/tutorial/*': 'fadeIn',
      '*': 'reveal',
    },
};

const AccountIcon = { [1]: require('../assets/img/account_icon.png'), [2]: require('../assets/img/account_icon@2x.png') };

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
    onClick: function() {}
  }
  render() {
    return (
      <div className={classList("connect-button", { 'on': this.props.on, 'off': !this.props.on }, this.props.connectionState)} onClick={this.props.onClick}>
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
            <OneZeros/>
            <ConnectButton on={this.state.connect} connectionState={this.state.connectionState} onClick={() => this.handleConnectClick()}/>

            <Link className="left account page-link" to="/account" tabIndex="0" data-tooltip="My Account" data-position="bottom left"><RetinaImage src={AccountIcon}/></Link>
            <Link className="right settings page-link" to="/configuration" tabIndex="0" data-tooltip="Configuration" data-position="bottom right"><i className="settings icon"/></Link>
            
            <div id="connection-stats" class="ui two column center aligned grid">
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesReceived)}</div><div class="label">Received</div></div></div>
              <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.bytesSent)}</div><div class="label">Sent</div></div></div>
            </div>
            <FirewallWarning/>
            <QuickPanel
              expanded={this.state.locationListOpen}
              onOtherClick={() => this.setState({ locationListOpen: true })}
              onListCloseClick={() => this.setState({ locationListOpen: false })}
              onLocationClick={value => this.onLocationClick(value)}
              onLocationFavoriteClick={value => this.onLocationFavoriteClick(value)}
              />
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
