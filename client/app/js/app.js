require('../less/app.less');

import React from 'react';
import ReactDOM from 'react-dom';

class ConnectButton extends React.Component {
  constructor() {
    super();
    this.state = {
      clicked: false
    };
    this.handleClick = this.handleClick.bind(this);
  }
  handleClick() {
    this.setState({clicked: !this.state.clicked});
  }
  render() {
    const text = this.state.clicked ? 'CONNECTING...' : 'DISCONNECTED';
    const color = this.state.clicked ? '#da8400' : '#c8121f';
    return (
      <div onClick={this.handleClick}>
      <svg width="120" height="120">
        <circle cx={60} cy={60} r={58} fill={color} />
        <circle cx={60} cy={60} r={50} fill="#fbfbfb" />
      </svg>

      <div >
         {text}
      </div>
      </div>
    );
  }
}

ReactDOM.render(
  <ConnectButton />,
  document.getElementById('content')
);
