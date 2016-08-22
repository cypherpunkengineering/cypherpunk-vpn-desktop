import React from 'react';
import ConnectButton from './Interface/ConnectButton.js';

export default class ConnectUI extends React.Component  {
  constructor() {
    super();
    this.state = { connect_text: "CONNECT"};
  }

  changeConnect() {
    this.setState( {connect_text: "CONNECTING"} );
    setTimeout(() => {
        this.setState( {connect_text: "CONNECTED"} );
    }, 2000);
    setTimeout(() => {
        this.setState( {connect_text: "CONNECT"} );
    }, 6000);
  }

  render(){
    return(
      <div>
        <ConnectButton changeConnect={this.changeConnect.bind(this)} connect_text={this.state.connect_text} />
        <Regionselect />
      </div>
    );
  }
}

class Regionselect extends React.Component {
  render(){
    return(
      <div className="region-select">
        <span className="flag-icon flag-icon-jp" />
            Region Select
      </div>
    );
  }
}
