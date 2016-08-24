import React from 'react';

export default class Configs extends React.Component  {
  constructor(props) {
    super(props);
  }

  render(){
    return(
      <div>
        <Header location={this.props}/>
        {this.props.children}
      </div>
    );
  }
}
