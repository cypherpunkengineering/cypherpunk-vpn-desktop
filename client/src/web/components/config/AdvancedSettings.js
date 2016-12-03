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
    var self = this;
    $(this.refs.root).find('.ui.dropdown').dropdown({ onChange: function(value) { self.onChange(this.children[0].name, value); } });
    $(this.refs.root).find('.ui.checkbox').checkbox({ onChange: function() { self.onChange(this.name, this.checked); } });
    $(this.refs.root).find('.ui.input').change(event => self.onChange(event.target.name, event.target.value)).parent().click(event => event.currentTarget.children[0].children[0].focus());
    //$(this.refs.firewall).dropdown({ onChange: value => { daemon.post.applySettings({ firewall: value }); }});
    //$(this.refs.protocol).dropdown({ onChange: value => { daemon.post.applySettings({ protocol: value }); }});
    //$(this.refs.remotePort).dropdown({ onChange: value => { daemon.post.applySettings({ remotePort: value }); }});
    //$(this.refs.localPort).val(daemon.settings.localPort || "");
    this.daemonSettingsChanged(daemon.settings);
  }
  onChange(name, value) {
    if (this.updatingSettings) return;
    console.log(JSON.stringify(name) + " changed to " + JSON.stringify(value));
    switch (name) {
      case 'firewall': daemon.post.applySettings({ firewall: value }); break;
      case 'protocol': daemon.post.applySettings({ protocol: value }); break;
      case 'remotePort': daemon.post.applySettings({ remotePort: value }); break;
      case 'localPort': daemon.post.applySettings({ localPort: parseInt(value, 10) }); break;
      case 'blockIPv6': break;
      case 'blockDNS': break;
      case 'allowLAN': break;
      case 'exemptApple': break;
      case 'forwardPort': break;
    }
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.firewall !== undefined) {
      $(this.refs.firewall).dropdown('set selected', settings.firewall);
    }
    if (settings.protocol !== undefined) {
      $(this.refs.protocol).dropdown('set selected', settings.protocol);
    }
    if (settings.remotePort !== undefined) {
      $(this.refs.remotePort).dropdown('set selected', settings.remotePort);
    }
    if (settings.localPort !== undefined) {
      $(this.refs.localPort).val(settings.localPort || "");
    }
    delete this.updatingSettings;
  }
  render() {
    return(
      <div className="collapsible" ref="root">
        <div className="collapsible-title">Advanced Settings</div>
        <div className="collapsible-content">

          <div className="pane" data-title="Privacy Firewall">
            <div class="setting">
              {/*
              <div class="ui selection button dropdown" ref="firewall">
                <input type="hidden" id="firewall" name="firewall"/>
                <i class="dropdown icon"></i>
                <div class="default text">Yes</div>
                <div className="menu">
                  <div class="item" data-value="on">Always</div>
                  <div class="item" data-value="auto">Auto</div>
                  <div class="item" data-value="off">No</div>
                </div>
              </div>
              <label>Block Non-VPN Traffic</label>
              */}
              <Link to="/configuration/firewall" tabIndex="0">Internet Killswitch</Link>
            </div>
            <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockIPv6" id="blockIPv6" ref="blockIPv6"/>
                <label>Block IPv6 Traffic</label>
              </div>
            </div>
            <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockDNS" id="blockDNS" ref="blockDNS"/>
                <label>Use Only Cypherpunk DNS</label>
              </div>
            </div>
          </div>

          <div className="pane" data-title="Compatibility">
            <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="allowLAN" id="allowLAN" ref="allowLAN"/>
                <label>Always Allow LAN Traffic</label>
              </div>
            </div>
            {/*(process.platform === 'darwin') ? <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="exemptApple" id="exemptApple" ref="exemptApple"/>
                <label>Exempt Apple Services from VPN</label>
              </div>
            </div> : null*/}
          </div>

          <div className="pane" data-title="VPN Settings">
            <div class="setting">
              <div class="ui selection button dropdown" ref="remotePort">
                <input type="hidden" id="remotePort" name="remotePort"/>
                <i class="dropdown icon"></i>
                <div class="default text">Select...</div>
                <div className="menu">
                  <div class="item" data-value="udp:7133">UDP 7133</div>
                  <div class="item" data-value="udp:5060">UDP 5060</div>
                  <div class="item" data-value="udp:53">UDP 53</div>
                  <div class="item" data-value="tcp:7133">TCP 7133</div>
                  <div class="item" data-value="tcp:443">TCP 443</div>
                </div>
              </div>
              <label>Remote Port</label>
            </div>
            <div className="setting">
              <div className="ui input">
                <input type="text" size="5" name="localPort" id="localPort" ref="localPort"/>
              </div>
              <label>Local Port</label>
            </div>
            {/*<div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="forwardPort" id="forwardPort" ref="forwardPort"/>
                <label>Request Port Forwarding</label>
              </div>
            </div>*/}
          </div>
        </div>
      </div>
    );
  }
}
