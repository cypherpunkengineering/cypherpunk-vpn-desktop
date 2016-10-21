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
      <div class="cp-settings config__settings--basic" ref="root">
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
              <Link to="/configuration/email">
              wiz@cypherpunk.com <i className="chevron right icon"></i>
              </Link>
            </div>
          </div>
          <div className="row cp_row">
            <div className="seven wide olive column">
              <Link to="/configuration/password">Password</Link>
            </div>
            <div className="nine wide olive right aligned column">
              <Link to="/configuration/password"><i className="chevron right icon"></i></Link>
            </div>
          </div>
          <div className="row cp_row">
            <div className="seven wide olive column">
              <Link to="/configuration/help">Help</Link>
            </div>
            <div className="nine wide olive right aligned column">
              <Link to="/configuration/help"><i className="chevron right icon"></i></Link>
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
            <div class="ui checkbox toggle">
              <input type="checkbox" name="autoconnect" id="autoconnect" ref="autoconnect"/>
              <label>Auto-connect on launch</label>
            </div>
          </div>
          <div class="cp-setting clickable item">
            <div class="ui checkbox toggle">
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
