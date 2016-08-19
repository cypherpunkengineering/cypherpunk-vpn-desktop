import React from 'react';

export default class ConnectButton extends React.Component  {

  handleChange(e) {
    this.props.changeConnect();
  }


  render(){
    const vsizex = 132;
    const vsizey = 132;
    const butrad = 66;
    var radred = 7;

    var buttonColour;
    if (this.props.connect_text == 'CONNECTED') {
      buttonColour = "#89c812";
      radred = 7;
    } else if (this.props.connect_text == 'CONNECTING'){
      buttonColour = "#da8400";
      radred = 7;
    } else {
      buttonColour = "#c8121f";
      radred = 7;
    }

    return(
      <div className="connect">
      <svg height={vsizex} width={vsizex} onClick={this.handleChange.bind(this)} >
        <circle cx={vsizex/2} cy={vsizey/2} r={butrad} fill={buttonColour}/>
        <circle cx={vsizex/2} cy={vsizey/2} r={butrad-radred} fill="#fefefe" />
      </svg><br />
      <span>
        {this.props.connect_text}
      </span>
      </div>
    );
  }
}
