import React from 'react';
import ReactDOM from 'react-dom';
import daemon, { DaemonAware } from '../daemon';
import { Overlay } from './Overlay';

export default class ReconnectButton extends DaemonAware(React.Component) {
  state = {
    needsReconnect: daemon.state.needsReconnect
  }
  daemonStateChanged(state) {
    if (state.hasOwnProperty('needsReconnect')) this.setState({ needsReconnect: state.needsReconnect });
  }
  onClick() {
    daemon.post.connect();
  }
  render() {
    if (!this.state.needsReconnect) {
      return null;
    }
    return (
      <Overlay name="reconnect"><a className="reconnect-button" onClick={() => this.onClick()}><i className="refresh icon"/><span>Reconnect</span> to apply changes</a></Overlay>
    );
  }
}
