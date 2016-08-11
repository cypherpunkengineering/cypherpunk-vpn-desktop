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
      </div>
      </div>
    );
  }
}
