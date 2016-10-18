import React from 'react';
import SpinningImage from '../assets/img/bgring3.png';

export default class MainBackground extends React.Component {
  render() {
    return(
      <div id="main-background">
        <div>
          <img class="r1" src={SpinningImage}/>
          <img class="r2" src={SpinningImage}/>
          <img class="r3" src={SpinningImage}/>
          <img class="r4" src={SpinningImage}/>
          <img class="r5" src={SpinningImage}/>
          <img class="r6" src={SpinningImage}/>
          <img class="r7" src={SpinningImage}/>
        </div>
      </div>
    );
  }
}

