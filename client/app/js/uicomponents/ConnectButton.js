import React from 'react';

export default class ConnectButton extends React.Component  {
  constructor() {
    super();
    this.state = {text: "DISCONNECT", bcolor: "#89c812"};
  }

  render(){
    setTimeout(() => {
      this.setState({text: "CONNECT", bcolor: "#c8121f"})
    }, 1000)
    const vsizex = 200;
    const vsizey = 200;
    const bradout = 100
    const bradin = bradout - 10;
    return(
      <div>
      <svg height={vsizex} width={vsizex}>
        <circle cx={vsizex/2} cy={vsizey/2} r={bradout} fill={this.state.bcolor} />
        <circle cx={vsizex/2} cy={vsizey/2} r={bradin} fill="#fefefe" />
      </svg><br />
      <span>
        {this.state.text}
      </span>
      </div>
    );
  }
}
