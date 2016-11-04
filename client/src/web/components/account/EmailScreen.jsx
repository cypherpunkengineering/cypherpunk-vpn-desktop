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
      </div>
    );
  }
}
