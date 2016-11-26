import React from 'react';
import SpinningImage from '../assets/img/bgring3.png';
import BgText1 from '../assets/img/bg-text-1.png';
import BgText2 from '../assets/img/bg-text-2.png';

export default class MainBackground extends React.Component {
  renderBgImage() {
    if (this.connectionState == 'connected') {
      return (
        <span>connected!!!</span>
      );
    }
    else {
      return (
        <span>oooh not connected?</span>
      );
    }
  }

  render() {
    return(
      <div id="onezeros">

        <img src={BgText1} id="bits-one" className="bits-img" />
        <img src={BgText2} id="bits-two" className="bits-img" />

        { this.renderBgImage() }


        {/*
        <div id="bits-1" class="bits">
          1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1
        </div>
        <div id="bits-2" class="bits">
          0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1
        </div>
        <div id="bits-3" class="bits">
          1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0 1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0
        </div>
        <div id="bits-4" class="bits">
          0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1
        </div>

        <div id="bits-5" class="bits">
          1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1
        </div>
        <div id="bits-6" class="bits">
          0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1
        </div>
        <div id="bits-7" class="bits">
          1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0 1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0
        </div>
        <div id="bits-8" class="bits">
          0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1
        </div>

        <div id="bits-9" class="bits">
          1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1
        </div>
        <div id="bits-10" class="bits">
          0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1
        </div>
        <div id="bits-11" class="bits">
          1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0 1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0
        </div>
        <div id="bits-12" class="bits">
          0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1
        </div>

        <div id="bits-13" class="bits">
          1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1
        </div>
        <div id="bits-14" class="bits">
          0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1
        </div>
        <div id="bits-15" class="bits">
          1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0 1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0
        </div>
        <div id="bits-16" class="bits">
          0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1
        </div>

        <div id="bits-17" class="bits">
          1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1 1 0 1 0 1 1 0 0 0 0 1 0 1 0 1 0 1 0 1 1
        </div>
        <div id="bits-18" class="bits">
          0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1 0 0 0 0 1 0 0 1 0 1 1 0 0 1 0 1 1 0 0 1
        </div>
        <div id="bits-19" class="bits">
          1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0 1 1 1 1 0 1 0 0 1 1 0 1 0 0 1 1 0 1 0 0
        </div>
        <div id="bits-20" class="bits">
          0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1 0 1 0 1 0 1 1 1 0 1 0 0 1 0 0 0 1 0 1 1 1
        </div>
        */}

      </div>
    );
  }
}
