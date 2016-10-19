import React from 'react';
import { Link } from 'react-router';

export default class PasswordScreen extends React.Component  {
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Update Password</div>
        </div>
        <div class="ui padded grid">
          <div class="row">
            <div class="seven wide olive column">
              New
            </div>
            <div class="nine wide olive right aligned column">
              <div className="ui input">
                <input type="password" placeholder="New Password" />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Confirm
            </div>
            <div class="nine wide olive right aligned column">
              <div className="ui input">
                <input type="password" placeholder="Confirm" />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Password
            </div>
            <div class="nine wide olive right aligned column">
              <div className="ui input">
                <input type="password" placeholder="Current Password" />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="column">
              <button id="update" class="ui inverted button">Update</button>
            </div>
          </div>
        </div>
      </div>
    );
  }
}
