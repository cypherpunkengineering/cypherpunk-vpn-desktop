import React from 'react';
import { Link } from 'react-router';

export default class HelpScreen extends React.Component  {
  render() {
    return(
      <div class="container__comp">
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/configuration"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Help</div>
        </div>
        <div class="ui padded grid">
          <div class="row">
            <div class="sixteen wide olive column">
              Write Help Information Here.
            </div>
          </div>
        </div>
      </div>
    );
  }
}
