import React from 'react';
import { Link } from 'react-router';

export default class AccountScreen extends React.Component  {
  render() {
    return(
      <div>
          <div className="container__comp account">
            <div className="ui fluid borderless icon menu cp_config_header">
              <Link className="item" to="/connect"><i className="angle left icon"></i></Link>
              <div className="header item center aligned">Account</div>
            </div>
            <div className="ui equal width center aligned padded grid ">
              <div className="row cp_row">
                <div className="column cp_account_avatar">
                  <i className="spy icon"></i> Wiz
                </div>
                <div className="column cp_account_stats">
                  Monthly Premium
                  <span className="cp_renew_date">Renews On 02/02/2016</span>
                </div>
              </div>
            </div>

            <div className="ui equal width center aligned padded grid">
              <div className="row cp_row">
                <div className="column">
                  <button id="upgrade" className="ui button">Upgrade</button>
                </div>
              </div>
            </div>

            <div className="ui padded grid">
              <div className="row cp_row">
                <div className="sixteen wide column">
                  <h3 className="ui yellow header cp_h3">ACCOUNT DETAILS</h3>
                </div>
              </div>
              <div className="row cp_row">
                <div className="four wide olive column">
                  Email
                </div>
                <div className="twelve wide olive right aligned column">
                  <Link to="/email">
                  wiz@cypherpunk.com <i className="angle right icon"></i>
                  </Link>
                </div>
              </div>
              <div className="row cp_row">
                <div className="seven wide olive column">
                  <Link to="/password">Password</Link>
                </div>
                <div className="nine wide olive right aligned column">
                  <Link to="/password"><i className="angle right icon"></i></Link>
                </div>
              </div>
              <div className="row cp_row">
                <div className="sixteen wide column">
                  <h3 className="ui yellow header cp_h3">More</h3>
                </div>
              </div>
              <div className="row cp_row">
                <div className="seven wide olive column">
                  <Link to="/help">Help</Link>
                </div>
                <div className="nine wide olive right aligned column">
                  <Link to="/help"><i className="angle right icon"></i></Link>
                </div>
              </div>
              <div className="row cp_row">
                <div className="seven wide olive column">
                  <Link to="/login">Logout</Link>
                </div>
                <div className="nine wide olive right aligned column">
                </div>
              </div>
            </div>
          </div>
      </div>
    );
  }
}
