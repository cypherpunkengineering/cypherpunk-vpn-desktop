import React from 'react';
import { Link } from 'react-router';


export default class SettingsScreen extends React.Component  {
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div className="full screen" style={{visibility: 'visible'}}>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Configuration</div>
        </div>
        <div className="ui two item tabular menu" ref="tab">
          <a class="item active" data-tab="general">General</a>
          <a class="item" data-tab="advanced">Advanced</a>
        </div>
        <div class="ui tab active" data-tab="general">
          <GeneralSettings />
        </div>
        <div class="ui tab" data-tab="advanced">
          <AdvancedSettings />
        </div>
      </div>
    );
  }
}

class AdvancedSettings extends React.Component  {
  render() {
    return(
      <div>
      <div className="cp">
        <div className="ro">
          <div className="co">Protocol</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Remote Port</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Local Port</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Firewall</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Use small packets</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Allow local traffic when firewall is on</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Request port forwarding</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">IPv6 leak protection</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">DNS leak protection</div>
          <div className="co">Yo</div>
        </div>
        <div className="ro">
          <div className="co">Encryption</div>
          <div className="co">Yo</div>
        </div>
      </div>
      </div>
    );
  }
}

class GeneralSettings extends React.Component  {
  render() {
    return(
      <div>
        <div className="cp">
          <div className="ro">
            <div className="co">Monthly Premium</div>
            <div className="co">Yo</div>
          </div>
        </div>
        <button>Upgrade</button>
        <h3>ACCOUNT DETAILS</h3>
        <div className="cp">
          <div className="ro">
            <div className="co">Email</div>
            <div className="co">wiz@cypherpunk.com</div>
          </div>
          <div className="ro">
            <div className="co">Password</div>
            <div className="co">Yo</div>
          </div>
          <div className="ro">
            <div className="co">Help</div>
            <div className="co"> </div>
          </div>
          <div className="ro">
            <div className="co">Logout</div>
            <div className="co"></div>
          </div>
        </div>
        <h3>BASIC SETTINGS</h3>
        <div className="cp">
          <div className="ro">
            <div className="co">Start application on startup</div>
            <div className="co"><input type="checkbox" /></div>
          </div>
          <div className="ro">
            <div className="co">Auto-connect on launch</div>
            <div className="co"><input type="checkbox" /></div>
          </div>
          <div className="ro">
            <div className="co">Show dekstop notifications</div>
            <div className="co"><input type="checkbox" /></div>
          </div>
          <div className="ro">
            <div className="co">Show cypherpunk on in</div>
            <div className="co"></div>
          </div>
        </div>
      </div>
    );
  }
}
