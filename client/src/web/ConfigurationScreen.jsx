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
      <div className="cp">
        <div className="ro">
          <div className="co">Protocol</div>
          <div className="co">
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
        <div className="ro">
          <div className="co">Remote Port</div>
          <div className="co">
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
        <div className="ro">
          <div className="co">
          Local Port
          <small>Customize the port used to connect to Cypherpunk</small>
          </div>
          <div className="co">
            <div className="ui input"><input type="text"/>
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">Firewall
            <small>Manage all internet connectivity when you are not connected to our network</small>
          </div>
          <div className="co">
            <Link to="/firewall"><span id="firewall_text">Automatic</span> <i className="chevron right icon"></i></Link>
          </div>
        </div>
        <div className="ro">
          <div className="co">Use small packets
            <small>Optimized packet size for improved connectivity to various router and mobile networks</small>
          </div>
          <div className="co">
            <div class="ui checkbox">
              <input type="checkbox" name="smallpackets" />
              <label />
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">Allow local traffic when firewall is on</div>
          <div className="co">
            <div class="ui checkbox">
              <input type="checkbox" name="allowlocaltraffic"/>
              <label />
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">Request port forwarding
            <small>Allow incoming connections to your mobile device over an external port</small>
          </div>
          <div className="co">
            <div class="ui checkbox">
              <input type="checkbox" name="requestportforwarding" />
              <label />
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">IPv6 leak protection
            <small>Disable IPv6 while using Cypherpunk</small>
          </div>
          <div className="co">
            <div class="ui checkbox">
              <input type="checkbox" name="ipv6leakprotection" />
              <label />
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">DNS leak protection
            <small>Prevent leaking DNS queries using Cypherpunk</small>
          </div>
          <div className="co">
            <div class="ui checkbox">
              <input type="checkbox" name="dnsleakprotection" />
              <label />
            </div>
          </div>
        </div>
        <div className="ro">
          <div className="co">Encryption
            <small>Cipher AES-256 Auth SHA512 Key 4096-bit</small>
          </div>
          <div className="co">
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
        <div className="cp">
          <div className="ro">
            <div className="co">Monthly Premium</div>
            <div className="co">Renews On 02/02/2016</div>
          </div>
        </div>
        <div className="ui centered padded grid">
          <div className="row">
          <button class="ui inverted button">Upgrade</button>
          </div>
        </div>
        <h3 className="ui yellow header">ACCOUNT DETAILS</h3>
        <div className="cp">
          <div className="ro">
            <div className="co">Email</div>
            <div className="co">wiz@cypherpunk.com <i class="chevron right icon"></i></div>
          </div>
          <div className="ro">
            <div className="co">Password</div>
            <div className="co"><i class="chevron right icon"></i></div>
          </div>
          <div className="ro">
            <div className="co">Help</div>
            <div className="co"><i class="chevron right icon"></i></div>
          </div>
          <div className="ro">
            <div className="co">Logout</div>
            <div className="co"></div>
          </div>
        </div>
        <h3 className="ui yellow header">BASIC SETTINGS</h3>
        <div className="cp">
          <div className="ro">
            <div className="co">Start application on startup</div>
            <div className="co">
              <div class="ui checkbox">
                <input type="checkbox" name="startapponstartup" />
                <label />
              </div>
            </div>
          </div>
          <div className="ro">
            <div className="co">Auto-connect on launch</div>
            <div className="co">
              <div class="ui checkbox">
                <input type="checkbox" name="autoconnect" />
                <label />
              </div>
            </div>
          </div>
          <div className="ro">
            <div className="co">Show dekstop notifications</div>
            <div className="co">
              <div class="ui checkbox">
                <input type="checkbox" name="desktopnotifications" />
                <label />
              </div>
            </div>
          </div>
          <div className="ro">
            <div className="co">Show cypherpunk on in</div>
            <div className="co">
              Dock Only
            </div>
          </div>
        </div>
      </div>
    );
  }
}
