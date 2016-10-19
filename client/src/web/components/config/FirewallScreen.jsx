import React from 'react';
import { Link } from 'react-router';
import daemon from '../../daemon.js';


export default class FirewallScreen extends React.Component  {
  constructor() {
    super();
    this.onDaemonSettingsChanged = this.onDaemonSettingsChanged.bind(this);
  }
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
    $(this.refs.root).find('.ui.checkbox').checkbox({ onChange: this.onChanged.bind(this) });
    this.onDaemonSettingsChanged();
    daemon.on('settings', this.onDaemonSettingsChanged);
  }
  componentWillUnmount() {
    daemon.removeListener('settings', this.onDaemonSettingsChanged);
  }
  onChanged() {
    var value = $(this.refs.root).find('input[name=firewall]:checked').val();
    daemon.post.applySettings({ 'firewall': value });
  }
  onDaemonSettingsChanged() {
    $(this.refs.root).find('#firewall-' + daemon.settings.firewall).parent().checkbox('set checked');
  }
  render() {
    return(
      <div ref="root">
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Firewall</div>
        </div>
        <div className="ui inverted padded grid">
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="firewall-auto" type="radio" name="firewall" value="auto" />
                <label for="firewall-auto">Automatic
                <small>
                  (Default) - Firewall will be enabled when you connect and disables when you disconnect from a location. It will remain on if your connection suddenly drops
                </small>
                </label>
              </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="firewall-on" type="radio" name="firewall" value="on" />
              <label for="firewall-on">Always on
              <small>
                Firewall is always on, and cannot be disabled unless
                You change this setting. You will not have any Internet access when you’re disconnected from Cypherpunk Network.
              </small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="firewall-off" type="radio" name="firewall" value="off" />
              <label for="firewall-off">
              Off
              <small>Firewall is off.</small>
              </label>
            </div>
            </div>
          </div>
        </div>

      </div>
    );
  }
}
