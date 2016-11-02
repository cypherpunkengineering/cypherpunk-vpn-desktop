import React from 'react';
import { Link } from 'react-router';

export default class PasswordScreen extends React.Component  {
  render() {
    return(
      <div class="container__comp material">
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/account"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Update Password</div>
        </div>

        <div className="cp-form form-password">
          <div className="form-password__new row">
              <div className="form__field">
                <i className="icon key"></i>
                <input type="text" placeholder="New Password" />
              </div>
          </div>
          <div class="form-password__confirm row">
            <div className="form__field">
              <i className="icon key"></i>
              <input type="text" placeholder="Confirm Password" />
            </div>
          </div>
          <div class="form-password__password row isolate">
            <div className="form__field">
              <i className="icon unlock"></i>
              <input type="password" placeholder="Current Password" />
            </div>
          </div>
          <div class="actions__form">
            <button id="update" class="ui button">Update</button>
          </div>
        </div>

        {/* <div class="ui padded grid">
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
              <button id="update" class="ui button">Update</button>
            </div>
          </div>
        </div> */}
      </div>
    );
  }
}
