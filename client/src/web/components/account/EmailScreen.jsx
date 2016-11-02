import React from 'react';
import { Link } from 'react-router';

export default class EmailScreen extends React.Component  {
  render() {
    return(
      <div class="container__comp content material">
        <div className="ui fluid inverted borderless icon menu cp_config_header titlebar">
          <Link className="item" to="/account"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Update Email</div>
        </div>

        <div className="cp-form form-email">
          <div className="form-email__new row">
              <div className="form__field">
                <i className="icon envelope"></i>
                <input type="text" placeholder="New email address" />
              </div>
          </div>
          <div class="form-email__confirm row">
            <div className="form__field">
              <i className="icon envelope"></i>
              <input type="text" placeholder="Confirm email address" />
            </div>
          </div>
          <div class="form-email__password row isolate">
            <div className="form__field">
              <i className="icon unlock"></i>
              <input type="password" placeholder="Current Password" />
            </div>
          </div>
          <div class="actions__form">
            <button id="update" class="ui button">Update</button>
          </div>
        </div>

        {/*<div class="ui padded grid">
          <div class="row">
            <div class="seven wide olive column">
              New Email
            </div>
            <div class="nine wide olive right aligned column">
              <div className="ui input">
                <input type="text" placeholder="verify@example.com" />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Confirm
            </div>
            <div class="nine wide olive right aligned column">
              <div className="ui input">
                <input type="text" placeholder="verify@example.com" />
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
