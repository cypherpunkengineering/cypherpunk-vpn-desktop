import React from 'react';
import ConnectButton from './ConnectButton.js';
import Header from './Header.js'

export default class Interface extends React.Component  {
  render(){
    return(
      <div>
      <Header/>
      <ConnectButton/>
      <div className="region-select">Japan
      <svg height="20" width="20">
        <polygon points="0,0 0,10 10,20 20,10, 20,0 10,2" fill="#fff"/>
      </svg>
      </div>
      </div>
    );
  }
}
