import React from 'react';
import { Link } from 'react-router';

export default class AccountScreen extends React.Component  {
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/connect"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Account</div>
        </div>
        Hello World! This is my account.
      </div>
    );
  }
}
