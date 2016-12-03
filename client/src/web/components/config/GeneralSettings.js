import React from 'react';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';

export default class GeneralSettings extends DaemonAware(React.Component)  {
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
    var self = this;
    $(this.refs.root).find('.ui.dropdown').dropdown({ onChange: function(value) { self.onChange(this.children[0].name, value); } });
    $(this.refs.root).find('.ui.checkbox').checkbox({ onChange: function() { self.onChange(this.name, this.checked); } });
    $(this.refs.root).find('.ui.input').change(event => self.onChange(event.target.name, event.target.value)).parent().click(event => event.currentTarget.children[0].children[0].focus());
    this.daemonSettingsChanged(daemon.settings);
  }
  componentWillUnmount() {
    super.componentWillUnmount();
    ipc.removeListener('autostart-value', this.listeners.autostart);
  }
  onChange(name, value) {
    if (this.updatingSettings) return;
    console.log(JSON.stringify(name) + " changed to " + JSON.stringify(value));
    switch (name) {
      case 'runonstartup': ipc.send('autostart-set', value); break;
      case 'autoconnect': daemon.post.applySettings({ autoConnect: value }); break;
      case 'desktopnotifications': daemon.post.applySettings({ showNotifications: value }); break;
    }
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.hasOwnProperty('autoConnect')) { $(this.refs.autoconnect).parent().checkbox('set ' + (settings.autoConnect ? 'checked' : 'unchecked')); }
    if (settings.hasOwnProperty('showNotifications')) { $(this.refs.desktopnotifications).parent().checkbox('set ' + (settings.showNotifications ? 'checked' : 'unchecked'))};
    this.updatingSettings = false;
  }
  onAutoStartSettingChanged(enabled) {
    $(this.refs.runonstartup).parent().checkbox('set ' + (enabled ? 'checked' : enabled === null ? 'indeterminate' : 'unchecked'));
  }
  render() {
    return(
      <div className="pane" data-title="Basic Settings" ref="root">
        {process.platform.match(/^(win32|darwin)$/)?<div className="setting"><Link to="/configuration/privacy" tabIndex="0">Privacy Mode</Link></div>:null}
        <div className="setting">
          <div className="ui toggle checkbox">
            <input type="checkbox" name="runonstartup" id="runonstartup" ref="runonstartup"/>
            <label>Launch on startup</label>
          </div>
        </div>
        <div className="setting">
          <div class="ui checkbox toggle">
            <input type="checkbox" name="autoconnect" id="autoconnect" ref="autoconnect"/>
            <label>Auto-connect on launch</label>
          </div>
        </div>
        {/*<div className="setting"><Link to="/configuration/trustednetworks">Auto-connect on untrusted networks</Link></div>*/}
        <div className="setting">
          <div class="ui checkbox toggle">
            <input type="checkbox" id="desktopnotifications" name="desktopnotifications" ref="desktopnotifications"/>
            <label>Show desktop notifications</label>
          </div>
        </div>
        {/*<div class="setting">
          <div class="ui selection button dropdown" ref="showinDropdown">
            <input type="hidden" id="showin" name="showin"/>
            <i class="dropdown icon"></i>
            <div class="text">Dock &amp; Menu</div>
            <div className="menu">
              <div class="item" data-value="dockonly">Dock Only</div>
              <div class="item" data-value="menuonly">Menu Only</div>
              <div class="item" data-value="dockmenu">Dock &amp; Menu</div>
            </div>
          </div>
          <label>Show app in</label>
        </div>*/}
      </div>
    );
  }
}
