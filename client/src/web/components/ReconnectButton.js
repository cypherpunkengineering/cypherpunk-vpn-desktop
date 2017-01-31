import React from 'react';
import ReactDOM from 'react-dom';
import daemon, { DaemonAware } from '../daemon';

export default class ReconnectButton extends DaemonAware(React.Component) {
  state = {
    needsReconnect: daemon.state.needsReconnect
  }
  daemonStateChanged(state) {
    if (state.hasOwnProperty('needsReconnect')) this.setState({ needsReconnect: state.needsReconnect });
  }
  render() {
    if (!this.state.needsReconnect) {
      return null;
    }
    return (
      <a className="reconnect-button"><i className="refresh icon"/><span>Reconnect</span> to apply changes</a>
    );
  }
}
