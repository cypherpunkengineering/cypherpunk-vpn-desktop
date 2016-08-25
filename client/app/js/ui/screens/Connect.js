import React from 'react';
import Connectbutton from './Connect/Connectbutton.js';
import Regionselect from './Connect/Regionselect.js';

export default class Connect extends React.Component  {
  constructor(props) {
    super(props);
    this.state = { connect_text: "Tap to Connect"};
  }

  changeConnect() {
    this.setState( {connect_text: "Connecting..."} );
    setTimeout(() => {
        this.setState( {connect_text: "You are protected"} );
    }, 2000);
    setTimeout(() => {
        this.setState( {connect_text: "Tap to protect"} );
    }, 6000);
  }

  render(){
    return(
      <div>
        {this.props.maintitle}
        <Connectbutton changeConnect={this.changeConnect.bind(this)} connect_text={this.state.connect_text} />
        <Regionselect />
      </div>
    );
  }
}
