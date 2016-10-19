import React from 'react';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from './daemon.js';


class Dragbar extends React.Component {
  render() {
    return (
      <div id="dragbar" class="dark"></div>
    )
  }
}

export default class ConfigurationScreen extends React.Component  {
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div>
        <Dragbar/>
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Configuration</div>
        </div>
        {/* <div className="ui two item tabular menu cp_config_tabs" ref="tab">
          <a className="item active" data-tab="general">General</a>
          <a className="item" data-tab="advanced">Advanced</a>
        </div>
        <div className="ui tab active tabscroll" data-tab="general">
          <GeneralSettings />
        </div>
        <div className="ui tab tabscroll" data-tab="advanced">
          <AdvancedSettings />
        </div> */}
        <div className="settingsContainer">
          <div className="" data-tab="general">
            <GeneralSettings />
          </div>
          <div className="" data-tab="advanced">
            <AdvancedSettings />
          </div>
        </div>
      </div>
    );
  }
}



class AdvancedSettings extends DaemonAware(React.Component)  {
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
      <div className="cp-settings">
      <div className="ui padded grid">
        <div class="header row">Advanced settings</div>
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
            <div className="ui toggle checkbox">
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
            <div className="ui toggle checkbox">
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
            <div className="ui toggle checkbox">
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
            <div className="ui toggle checkbox">
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
            <div className="ui toggle checkbox">
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

class GeneralSettings extends DaemonAware(React.Component)  {
  constructor(props) {
    super(props);
    this.listeners = {
      autostart: (event, enabled) => this.onAutoStartSettingChanged(enabled),
    };
  }
  componentWillMount() {
    ipc.on('autostart-value', this.listeners.autostart);
    ipc.send('autostart-get');
  }
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.showinDropdown).dropdown();
    //$(this.refs.root).find('.ui.checkbox').checkbox();
    $(this.refs.runonstartup).parent().checkbox({ onChange: function() { ipc.send('autostart-set', this.checked); } });
    $(this.refs.autoconnect).parent().checkbox({ onChange: function() { daemon.post.applySettings({ autoConnect: this.checked }); }});
    $(this.refs.desktopnotifications).parent().checkbox({ onChange: function() { daemon.post.applySettings({ showNotifications: this.checked }); }});
    this.daemonSettingsChanged(daemon.settings);
  }
  componentWillUnmount() {
    super.componentWillUnmount();
    ipc.removeListener('autostart-value', this.listeners.autostart);
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('autoConnect')) { $(this.refs.autoconnect).parent().checkbox('set ' + (settings.autoConnect ? 'checked' : 'unchecked')); }
    if (settings.hasOwnProperty('showNotifications')) { $(this.refs.desktopnotifications).parent().checkbox('set ' + (settings.showNotifications ? 'checked' : 'unchecked'))};
  }
  onAutoStartSettingChanged(enabled) {
    $(this.refs.runonstartup).parent().checkbox('set ' + (enabled ? 'checked' : enabled === null ? 'indeterminate' : 'unchecked'));
  }
  render() {
    return(
      <div class="cp-settings" ref="root">
        <div className="ui equal width center aligned padded grid ">
          <div className="row cp_row">
            <div className="olive column cp_account_avatar">
              <i className="spy icon"></i> Wiz
            </div>
          </div>
          <div className="row cp_row">
            <div className="olive column">
              Monthly Premium
            </div>
            <div className="olive column">
              <span className="cp_renew_date">Renews On 02/02/2016</span>
            </div>
          </div>
        </div>

        <div className="ui equal width center aligned padded grid">
          <div className="row cp_row">
            <div className="column">
              <button id="upgrade" className="ui inverted button cp_button">Upgrade</button>
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
              wiz@cypherpunk.com <i className="chevron right icon"></i>
              </Link>
            </div>
          </div>
          <div className="row cp_row">
            <div className="seven wide olive column">
              <Link to="/password">Password</Link>
            </div>
            <div className="nine wide olive right aligned column">
              <Link to="/password"><i className="chevron right icon"></i></Link>
            </div>
          </div>
          <div className="row cp_row">
            <div className="seven wide olive column">
              <Link to="/help">Help</Link>
            </div>
            <div className="nine wide olive right aligned column">
              <Link to="/help"><i className="chevron right icon"></i></Link>
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

        <div class="ui fluid vertical cp-settings menu">
          <div class="header">Basic settings</div>
          <div class="cp-setting clickable item">
            <div class="ui toggle checkbox">
              <input type="checkbox" name="runonstartup" id="runonstartup" ref="runonstartup"/>
              <label>Launch on startup</label>
            </div>
          </div>
          <div class="cp-setting clickable item">
            <div class="ui toggle checkbox">
              <input type="checkbox" name="autoconnect" id="autoconnect" ref="autoconnect"/>
              <label>Auto-connect on launch</label>
            </div>
          </div>
          <div class="cp-setting clickable item">
            <div class="ui toggle checkbox">
              <input type="checkbox" id="desktopnotifications" name="desktopnotifications" ref="desktopnotifications"/>
              <label>Show desktop notifications</label>
            </div>
          </div>
          <div style={{display:'none'}}>
          <div class="cp-setting item">
            <div class="ui olive button selection dropdown" ref="showinDropdown">
              <input type="hidden" id="showin" name="showin"/>
              <i class="dropdown icon"></i>
              <div class="default text">Dock Only</div>
              <div className="menu">
                <div class="item" data-value="dockonly">Dock Only</div>
                <div class="item" data-value="menuonly">Menu Only</div>
                <div class="item" data-value="dockmenu">Dock &amp; Menu</div>
              </div>
            </div>
            <label>Show Cypherpunk icon in</label>
          </div>
          </div>
        </div>

      </div>
    );
  }
}
