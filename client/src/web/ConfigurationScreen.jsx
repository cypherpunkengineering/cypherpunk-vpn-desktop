import React from 'react';
import { Link } from 'react-router';


export default class ConfigurationScreen extends React.Component  {
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div className="settings_screen">
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
  componentDidMount() {
    $(this.refs.reportportDropdown).dropdown();
    $(this.refs.protocolDropdown).dropdown();
  }
  render() {
    return(
      <div>
      <div class="ui padded grid">
        <div class="row">
          <div class="nine wide olive column">
            Protocol
          </div>
          <div class="seven wide olive right aligned column">
            <div className="ui olive button selection dropdown" ref="protocolDropdown">
              <input type="hidden" name="protocol" />
              <i className="dropdown icon"></i>
              <div className="default text">Automatic</div>
              <div className="menu">
                <div className="item" data-value="Aautomatic">Automatic</div>
                <div className="item" data-value="manual">Manual</div>
              </div>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Remote Port
          </div>
          <div class="seven wide olive right aligned column">
            <div className="ui olive button selection dropdown" ref="reportportDropdown">
              <input type="hidden" name="remoteport" />
              <i className="dropdown icon"></i>
              <div className="default text">Auto</div>
              <div className="menu">
                <div className="item" data-value="auto">Auto</div>
                <div className="item" data-value="47">47</div>
                <div className="item" data-value="1723">1723</div>
              </div>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="seven wide olive column">
            Local Port
            <small>Customize the port used to connect to Cypherpunk</small>
          </div>
          <div class="nine wide olive right aligned column">
            <div className="ui input">
              <input type="text"/>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Firewall
            <small>Manage all internet connectivity when you are not connected to our network</small>
          </div>
          <div class="seven wide olive right aligned column">
            <Link to="/firewall"><span id="firewall_text">Automatic</span> <i className="chevron right icon"></i></Link>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Use small packets
            <small>Optimized packet size for improved connectivity to various router and mobile networks</small>
          </div>
          <div class="seven wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="smallpackets" />
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Allow local traffic when firewall is on
          </div>
          <div class="seven wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="allowlocaltraffic"/>
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Request port forwarding
            <small>Allow incoming connections to your mobile device over an external port</small>
          </div>
          <div class="seven wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="requestportforwarding" />
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            IPv6 leak protection
            <small>Disable IPv6 while using Cypherpunk</small>
          </div>
          <div class="seven wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="ipv6leakprotection" />
              <label />
            </div>
          </div>
        </div>

        <div class="row">
          <div class="nine wide olive column">
            DNS leak protection
            <small>Prevent leaking DNS queries using Cypherpunk</small>
          </div>
          <div class="seven wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="dnsleakprotection" />
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            Encryption
            <small>Cipher AES-256 Auth SHA512 Key 4096-bit</small>
          </div>
          <div class="seven wide olive right aligned column">
            <Link to="/encryption"><span id="encryption_text">Automatic</span> <i className="chevron right icon"></i></Link>
          </div>
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


        <div class="ui equal width center aligned padded grid">
          <div class="row">
            <div class="olive column">
              <i class="spy icon"></i> Wiz
            </div>
          </div>
          <div class="row">
            <div class="olive column">
              Monthly Premium
            </div>
            <div class="olive column">
              <small>Renews On 02/02/2016</small>
            </div>
          </div>
        </div>

        <div class="ui equal width center aligned padded grid">
          <div class="row">
            <div class="column">
              <button class="ui inverted button">Upgrade</button>
            </div>
          </div>
        </div>

        <div class="ui padded grid">
          <div class="row">
            <div class="sixteen wide column">
              <h3 className="ui yellow header">ACCOUNT DETAILS</h3>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Email
            </div>
            <div class="nine wide olive right aligned column">
              wiz@cypherpunk.com <i class="chevron right icon"></i>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Password
            </div>
            <div class="nine wide olive right aligned column">
              <i class="chevron right icon"></i>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Help
            </div>
            <div class="nine wide olive right aligned column">
              <i class="chevron right icon"></i>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              Logout
            </div>
            <div class="nine wide olive right aligned column">
            </div>
          </div>
        </div>
        <div class="ui padded grid">
          <div class="row">
            <div class="sixteen wide column">
              <h3 className="ui yellow header">BASIC SETTINGS</h3>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              Start application on startup
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" name="startapponstartup" />
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              Auto-connect on launch
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" name="autoconnect" />
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              Show dekstop notifications
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" name="desktopnotifications" />
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              Show cypherpunk on in
            </div>
            <div class="five wide olive right aligned column">
              Dock Only
            </div>
          </div>
        </div>


      </div>
    );
  }
}
