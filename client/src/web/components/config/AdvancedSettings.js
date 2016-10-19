import React from 'react';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';

export default class AdvancedSettings extends DaemonAware(React.Component)  {
  constructor(props) {
    super(props);
  }
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.protocol).dropdown({ onChange: value => { daemon.post.applySettings({ protocol: value }); }});
    $(this.refs.remotePort).dropdown({ onChange: value => { daemon.post.applySettings({ remotePort: value }); }});
    $(this.refs.localPort).val(daemon.settings.localPort || "");
  }
  daemonSettingsChanged(settings) {
    if (settings.protocol !== undefined) {
      $(this.refs.protocol).dropdown('set selected', settings.protocol);
    }
    if (settings.remotePort !== undefined) {
      $(this.refs.remotePort).dropdown('set selected', settings.remotePort);
    }
    if (settings.localPort !== undefined) {
      $(this.refs.localPort).val(settings.localPort || "");
    }
  }
  render() {
    return(
      <div className="config__settings--advanced">
        <div className="ui padded grid">
          <div className="header">
            Advanced Settings
          </div>
          <div className="row cp_row">
            <div className="eleven wide olive column">
              Protocol
            </div>
            <div className="five wide olive right aligned column">
              <div className="ui olive button selection dropdown" ref="protocol">
                <input type="hidden" name="protocol" value={daemon.settings.protocol}/>
                <i className="dropdown icon"></i>
                <div className="default text">UDP</div>
                <div className="menu">
                  <div className="item" data-value="udp">UDP</div>
                  <div className="item" data-value="tcp">TCP</div>
                </div>
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="nine wide olive column">
              Remote port
            </div>
            <div className="seven wide olive right aligned column">
              <div className="ui olive button selection dropdown" ref="remotePort">
                <input type="hidden" name="remoteport" />
                <i className="dropdown icon"></i>
                <div className="default text">Auto</div>
                <div className="menu">
                  <div className="item" data-value="auto">Auto</div>
                  <div className="item" data-value="7133">7133</div>
                </div>
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="eleven wide olive column">
              Local port
              <small>Customize the port used to connect to Cypherpunk</small>
            </div>
            <div className="five wide olive right aligned column">
              <div className="ui input">
                <input type="text" size="5" name="localport" ref="localPort" onChange={(function(event) { daemon.post.applySettings({ localPort: parseInt(event.target.value, 10) }); return true; })} />
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="ten wide olive column">
              Firewall
              <small>Manage all internet connectivity when you are not connected to our network</small>
            </div>
            <div className="six wide olive right aligned column">
              <Link to="/firewall"><span id="firewall_text">Automatic</span> <i className="chevron right icon"></i></Link>
            </div>
          </div>
          <div className="row cp_row">
            <div className="thirteen wide olive column">
              <label for="smallpackets">Use small packets
              <small>Optimized packet size for improved connectivity to various router and mobile networks</small>
              </label>
            </div>
            <div className="three wide olive right aligned column">
              <div className="ui checkbox toggle">
                <input type="checkbox" name="smallpackets" id="smallpackets"/>
                <label />
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="thirteen wide olive column">
              <label for="allowlocaltraffic">Allow local traffic when firewall is on</label>
            </div>
            <div className="three wide olive right aligned column">
              <div className="ui checkbox toggle">
                <input type="checkbox" name="allowlocaltraffic" id="allowlocaltraffic"/>
                <label />
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="thirteen wide olive column">
              <label for="requestportforwarding">Request port forwarding
              <small>Allow incoming connections to your mobile device over an external port</small>
              </label>
            </div>
            <div className="three wide olive right aligned column">
              <div className="ui checkbox toggle">
                <input type="checkbox" name="requestportforwarding" id="requestportforwarding"/>
                <label />
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="thirteen wide olive column">
              <label for="ipv6leakprotection">IPv6 leak protection
              <small>Disable IPv6 while using Cypherpunk</small>
              </label>
            </div>
            <div className="three wide olive right aligned column">
              <div className="ui checkbox toggle">
                <input type="checkbox" name="ipv6leakprotection" id="ipv6leakprotection"/>
                <label />
              </div>
            </div>
          </div>

          <div className="row cp_row">
            <div className="thirteen wide olive column">
              <label for="dnsleakprotection">DNS leak protection
              <small>Prevent leaking DNS queries using Cypherpunk</small>
              </label>
            </div>
            <div className="three wide olive right aligned column">
              <div className="ui checkbox toggle">
                <input type="checkbox" name="dnsleakprotection" id="dnsleakprotection" />
                <label />
              </div>
            </div>
          </div>
          <div className="row cp_row">
            <div className="nine wide olive column">
              <Link to="/encryption">Encryption
              <small>Cipher AES-256 Auth SHA512 Key 4096-bit</small>
              </Link>
            </div>
            <div className="seven wide olive right aligned column">
              <Link to="/encryption"><span id="encryption_text">Automatic</span> <i className="chevron right icon"></i></Link>
            </div>
          </div>

        </div>
      </div>
    );
  }
}
