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
      case 'blockDNS': daemon.post.applySettings({ blockDNS: value }); break;
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
    if (settings.blockDNS !== undefined) {
      $(this.refs.blockDNS).parent().checkbox('set ' + (settings.blockDNS ? 'checked' : 'unchecked'));
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
          <div className={"pane" + (process.platform == 'linux' ? " hidden" : "")} data-title="Privacy Settings">
            <div class="setting">
              <Link to="/configuration/firewall" tabIndex="0" ref="firewall">Internet Killswitch</Link>
            </div>
            <div className="setting">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="allowLAN" id="allowLAN" ref="allowLAN"/>
                <label>Always Allow LAN Traffic</label>
              </div>
            </div>
            {/* hide until feature works correctly */}
            <div className="setting hidden">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockIPv6" id="blockIPv6" ref="blockIPv6"/>
                <label>Block IPv6 Traffic</label>
              </div>
            </div>
            <div className="setting hidden">
              <div className="ui toggle checkbox">
                <input type="checkbox" name="blockDNS" id="blockDNS" ref="blockDNS"/>
                <label>Use Cypherpunk DNS</label>
              </div>
            </div>
          </div>

          <div className={"pane" + ((daemon.account.account.type === 'developer' || process.platform === 'darwin') ? "" : " hidden")} data-title="Routing Settings">
            <div className={"setting" + (daemon.account.account.type === 'developer' ? "" : " hidden")}>
              <div className="ui toggle checkbox">
                <input type="checkbox" name="routeDefault" id="routeDefault" ref="routeDefault"/>
                <label>Route Internet Traffic via VPN</label>
              </div>
            </div>
            <div className={"setting" + (process.platform === 'darwin' ? "" : " hidden")}>
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
