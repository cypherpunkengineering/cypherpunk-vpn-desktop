import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';

export default class AdvancedSettings extends DaemonAware(React.Component)  {
  state = {
    advanced: !!daemon.settings.showAdvancedSettings
  }
  componentDidMount() {
    super.componentDidMount();
    var self = this;
    $(this.refs.root).find('.ui.checkbox').checkbox({ onChange: function() { self.onChange(this.name, this.checked); } });
    $(this.refs.root).find('.ui.input').change(event => self.onChange(event.target.name, event.target.value)).parent().click(event => event.currentTarget.children[0].children[0].focus());
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
      case 'blockIPv6': daemon.post.applySettings({ blockIPv6: value }); break;
      case 'overrideDNS': daemon.post.applySettings({ overrideDNS: value }); break;
      case 'blockAds': daemon.post.applySettings({ blockAds: value }); break;
      case 'blockMalware': daemon.post.applySettings({ blockMalware: value }); break;
      case 'allowLAN': daemon.post.applySettings({ allowLAN: value }); break;
      case 'routeDefault': daemon.post.applySettings({ routeDefault: value }); break;
      case 'exemptApple': daemon.post.applySettings({ exemptApple: value }); break;
      case 'forwardPort': break;
    }
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.protocol !== undefined) {
      $(this.refs.protocol).dropdown('set selected', settings.protocol);
    }
    if (settings.remotePort !== undefined) {
      $(ReactDOM.findDOMNode(this.refs.remotePort)).attr('data-value', settings.remotePort.replace(':', ' ').toUpperCase());
    }
    if (settings.localPort !== undefined) {
      $(this.refs.localPort).val(settings.localPort || "");
    }
    if (settings.blockIPv6 !== undefined) {
      $(this.refs.blockIPv6).parent().checkbox('set ' + (settings.blockIPv6 ? 'checked' : 'unchecked'));
    }
    if (settings.overrideDNS !== undefined) {
      $(this.refs.overrideDNS).parent().checkbox('set ' + (settings.overrideDNS ? 'checked' : 'unchecked'));
    }
    if (settings.blockAds !== undefined) {
      $(this.refs.blockAds).parent().checkbox('set ' + (settings.blockAds ? 'checked' : 'unchecked'));
    }
    if (settings.blockMalware !== undefined) {
      $(this.refs.blockMalware).parent().checkbox('set ' + (settings.blockMalware ? 'checked' : 'unchecked'));
    }
    if (settings.allowLAN !== undefined) {
      $(this.refs.allowLAN).parent().checkbox('set ' + (settings.allowLAN ? 'checked' : 'unchecked'));
    }
    if (settings.firewall !== undefined) {
      $(ReactDOM.findDOMNode(this.refs.firewall)).attr('data-value', ({ 'on': "Always On", 'auto' : "Auto", 'off': "Off" })[settings.firewall]);
    }
    if (settings.exemptApple !== undefined) {
      $(this.refs.exemptApple).parent().checkbox('set ' + (settings.exemptApple ? 'checked' : 'unchecked'));
    }
    if (settings.routeDefault !== undefined) {
      $(this.refs.routeDefault).parent().checkbox('set ' + (settings.routeDefault ? 'checked' : 'unchecked'));
    }
    if (settings.showAdvancedSettings !== undefined) {
      this.setState({ advanced: settings.showAdvancedSettings });
    }
    delete this.updatingSettings;
  }
  onAdvancedClick() {
    var advanced = !this.state.advanced;
    this.setState({ advanced: advanced });
    daemon.post.applySettings({ showAdvancedSettings: advanced });
  }
  render() {
    return(
      <div className={"collapsible" + (this.state.advanced ? " open" : "")} ref="root">
        <div className="collapsible-title" onClick={e => this.onAdvancedClick(e)}>Advanced Settings</div>
        <div className="collapsible-content">
          <div className="pane" data-title="Privacy Settings">
            <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="overrideDNS" id="overrideDNS" ref="overrideDNS"/>
                <label>Use Cypherpunk DNS</label>
              </div>
            </div>
            <div className="setting indented">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockAds" id="blockAds" ref="blockAds"/>
                <label>Block Ads</label>
              </div>
            </div>
            <div className="setting indented">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockMalware" id="blockMalware" ref="blockMalware"/>
                <label>Block Malware</label>
              </div>
            </div>
            <div class="setting">
              <Link to="/configuration/firewall" tabIndex="0" ref="firewall">Internet Killswitch</Link>
            </div>
            <div className="setting indented">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="allowLAN" id="allowLAN" ref="allowLAN"/>
                <label>Always Allow LAN Traffic</label>
              </div>
            </div>
            {/* hide until feature works correctly */}
            <div className="setting hidden indented">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockIPv6" id="blockIPv6" ref="blockIPv6"/>
                <label>Block IPv6 Traffic</label>
              </div>
            </div>
          </div>

          <div className={"pane" + ((true || daemon.account.account.type === 'developer' || process.platform === 'darwin') ? "" : " hidden")} data-title="Routing Settings">
            <div className={"setting" + (true || daemon.account.account.type === 'developer' ? "" : " hidden")}>
              <div className="ui toggle checkbox">
                <input type="checkbox" name="routeDefault" id="routeDefault" ref="routeDefault"/>
                <label>Route Internet Traffic via VPN</label>
              </div>
            </div>
            <div className={"setting indented" + (process.platform === 'darwin' ? "" : " hidden")}>
              <div className="ui toggle checkbox">
                <input type="checkbox" name="exemptApple" id="exemptApple" ref="exemptApple"/>
                <label>Exempt Apple Services</label>
              </div>
            </div>
          </div>

          <div className="pane" data-title="Connection Settings">
            <div class="setting">
              <Link to="/configuration/remoteport" tabIndex="0" ref="remotePort">Remote Port</Link>
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
