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
          <div class="nine wide olive column">
            Local Port
            <small>Customize the port used to connect to Cypherpunk</small>
          </div>
          <div class="seven wide olive right aligned column">
            <div className="ui input">
              <input type="text"/>
            </div>
          </div>
        </div>
        <div class="row">
          <div class="ten wide olive column">
            Firewall
            <small>Manage all internet connectivity when you are not connected to our network</small>
          </div>
          <div class="six wide olive right aligned column">
            <Link to="/firewall"><span id="firewall_text">Automatic</span> <i className="chevron right icon"></i></Link>
          </div>
        </div>
        <div class="row">
          <div class="thirteen wide olive column">
            <label for="smallpackets">Use small packets
            <small>Optimized packet size for improved connectivity to various router and mobile networks</small>
            </label>
          </div>
          <div class="three wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="smallpackets" id="smallpackets"/>
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="thirteen wide olive column">
            <label for="allowlocaltraffic">Allow local traffic when firewall is on</label>
          </div>
          <div class="three wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="allowlocaltraffic" id="allowlocaltraffic"/>
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="thirteen wide olive column">
            <label for="requestportforwarding">Request port forwarding
            <small>Allow incoming connections to your mobile device over an external port</small>
            </label>
          </div>
          <div class="three wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="requestportforwarding" id="requestportforwarding"/>
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="thirteen wide olive column">
            <label for="ipv6leakprotection">IPv6 leak protection
            <small>Disable IPv6 while using Cypherpunk</small>
            </label>
          </div>
          <div class="three wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="ipv6leakprotection" id="ipv6leakprotection"/>
              <label />
            </div>
          </div>
        </div>

        <div class="row">
          <div class="thirteen wide olive column">
            <label for="dnsleakprotection">DNS leak protection
            <small>Prevent leaking DNS queries using Cypherpunk</small>
            </label>
          </div>
          <div class="three wide olive right aligned column">
            <div class="ui checkbox">
              <input type="checkbox" name="dnsleakprotection" id="dnsleakprotection" />
              <label />
            </div>
          </div>
        </div>
        <div class="row">
          <div class="nine wide olive column">
            <Link to="/encryption">Encryption
            <small>Cipher AES-256 Auth SHA512 Key 4096-bit</small>
            </Link>
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
  componentDidMount() {
    $(this.refs.showinDropdown).dropdown();
  }
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
              <Link to="/email">
              wiz@cypherpunk.com <i class="chevron right icon"></i>
              </Link>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              <Link to="/password">Password</Link>
            </div>
            <div class="nine wide olive right aligned column">
              <Link to="/password"><i class="chevron right icon"></i></Link>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              <Link to="/help">Help</Link>
            </div>
            <div class="nine wide olive right aligned column">
              <Link to="/help"><i class="chevron right icon"></i></Link>
            </div>
          </div>
          <div class="row">
            <div class="seven wide olive column">
              <Link to="/login">Logout</Link>
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
              <label for="startapponstartup">Start application on startup</label>
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" name="startapponstartup" id="startapponstartup"/>
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              <label for="autoconnect">Auto-connect on launch</label>
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" name="autoconnect" id="autoconnect"/>
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eleven wide olive column">
              <label for="desktopnotifications">Show dekstop notifications</label>
            </div>
            <div class="five wide olive right aligned column">
              <div class="ui checkbox">
                <input type="checkbox" id="desktopnotifications" name="desktopnotifications" />
                <label />
              </div>
            </div>
          </div>
          <div class="row">
            <div class="eight wide olive column">
              Show cypherpunk on in
            </div>
            <div class="eight wide olive right aligned column">
            <div className="ui olive button selection dropdown" ref="showinDropdown">
              <input type="hidden" name="showin" />
              <i className="dropdown icon"></i>
              <div className="default text">Dock Only</div>
              <div className="menu">
                <div className="item" data-value="dockonly">Dock Only</div>
                <div className="item" data-value="menuonly">Menu Only</div>
                <div className="item" data-value="dockmenu">Dock & Menu</div>
              </div>
            </div>
            </div>
          </div>
        </div>


      </div>
    );
  }
}
