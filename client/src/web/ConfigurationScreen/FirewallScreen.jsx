import React from 'react';
import { Link } from 'react-router';


export default class FirewallScreen extends React.Component  {
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Firewall</div>
        </div>
        <div className="ui inverted padded grid">
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="firewall-automatic" type="radio" name="firewall" />
                <label for="firewall-automatic">Automatic
                <small>
                  (Default) - Firewall will be enabled when you connect and disables when you disconnect from a location. It will remain on if your connection suddenly drops
                </small>
                </label>
              </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="firewall-alwayson" type="radio" name="firewall" />
              <label for="firewall-alwayson">Always on
              <small>
                Firewall is always on, and cannot be disabled unless
                You change this setting. You will not have any Internet access when you’re disconnected from Cypherpunk Network.
              </small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="firewall-off" type="radio" name="firewall" />
              <label for="firewall-off">
              Off
              <small>Firewall is off.</small>
              </label>
            </div>
            </div>
          </div>
        </div>

      </div>
    );
  }
}
