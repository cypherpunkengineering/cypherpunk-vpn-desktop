import React from 'react';
import { Motion, spring } from 'react-motion';
//import Dropdown from 'react-toolbox/lib/dropdown';
import { Button } from 'react-toolbox/lib/button';


export default class Regionselect extends React.Component {

  render () {
    return (
       <Button icon='inbox' label='Inbox' flat />
      //<Motion defaultStyle={{opacity: 0}} style={{opacity: spring(1)}}>
      //       {interpolatingStyle => <div style={interpolatingStyle} className="region-select">Hong Kong</div>}
      //</Motion>
    );
  }
}
