import React from 'react';

export default class Settings extends React.Component  {
  render(){

    return(
      <div>
        <h2>Settings</h2>
        <Item title="A"/>
        <Item title="B"/>
        <Item title="C"/>
      </div>
    );
  }
}

class Item extends React.Component {
  render(){
    return(
      <div>
        <div>{this.props.title}:</div>
        <div>1</div>
      </div>
    );
  }
}
