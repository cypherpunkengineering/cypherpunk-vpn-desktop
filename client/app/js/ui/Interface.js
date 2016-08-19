import React from 'react';
import Header from './Interface/Header.js';

export default class Interface extends React.Component  {
  render(){

    return(
      <div>
        <Header location={this.props}/>
        {this.props.children}
      </div>
    );
  }
}
