import React from 'react';
import { Link } from 'react-router';
import WorldMap from './assets/img/map_wh.svg';

export default class StatusScreen extends React.Component  {

  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Status</div>
        </div>
        <div id="status-screen">
          <div className="ui centered padded grid">
            <div className="row"><img src={WorldMap} /></div>
            <div className="row">
              <div className="ui inverted olive segment">
                connected/disconnecte
              </div>
            </div>
          </div>
        </div>
      </div>
    );
  }
}
