import React from 'react';
import ConnectStore from '../../stores/ConnectStore';
import * as ConnectAction from '../../actions/ConnectAction';

export default class Connectbutton extends React.Component  {
  constructor(props) {
    super();
    this.getConnectState = this.getConnectState.bind(this);
    this.state = {
      connectState: ConnectStore.getState()
    };
  }

  handleChange() {
    switch(ConnectStore.getState()) {
      case 'disconnected': {
        ConnectAction.updateConnect('connecting');
      break;
      }
      case 'connected':{
        ConnectAction.updateConnect('disconnected');
      break;
      }
      case 'connecting':{
        ConnectAction.updateConnect('connected');
      break;
      }
    }

  }

  componentWillMount() {
    ConnectStore.on("change", this.getConnectState);
  }

  componentWillUnmount() {
    ConnectStore.removeListener("change", this.getConnectState);
  }


  getConnectState() {
    this.setState({
      connectState: ConnectStore.getState(),
    })
  }



  render(){

    const vsizex = 140;
    const vsizey = 140;
    const butrad = 66;
    var radred = 7;

    var buttonColour;
    var buttonText;

    if (this.state.connectState == 'connected') {
      buttonColour = "#89c812";
      buttonText = "You are protected";
      radred = 7;
    } else if (this.state.connectState == 'connecting'){
      buttonColour = "#da8400";
      buttonText = "Connecting...";
      radred = 7;
    } else {
      buttonColour = "#c8121f";
      buttonText = "Tap to protect";
      radred = 7;
    }

    return(
      <div className="connect">
      <svg height={vsizex} width={vsizex} onClick={this.handleChange.bind(this)} >
        <circle cx={vsizex/2} cy={vsizey/2} r={butrad} fill={buttonColour}/>
        <circle cx={vsizex/2} cy={vsizey/2} r={butrad-radred} fill="#fefefe" />
      </svg><br />
      <span>
        {buttonText}
      </span>
      </div>
    );
  }
}
