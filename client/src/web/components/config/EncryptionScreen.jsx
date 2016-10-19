import React from 'react';
import { Link } from 'react-router';
import daemon from '../../daemon.js';


export default class EncryptionScreen extends React.Component  {
  constructor() {
    super();
    this.onDaemonSettingsChanged = this.onDaemonSettingsChanged.bind(this);
  }
  componentDidMount() {
    $(this.refs.root).find('.ui.checkbox').checkbox({ onChange: this.onChanged.bind(this) });
    this.onDaemonSettingsChanged();
    daemon.on('settings', this.onDaemonSettingsChanged);
  }
  componentWillUnmount() {
    daemon.removeListener('settings', this.onDaemonSettingsChanged);
  }
  onChanged() {
    var value = $(this.refs.root).find('input[name=encryption]:checked').val();
    daemon.post.applySettings({ 'encryption': value });
  }
  onDaemonSettingsChanged() {
    $(this.refs.root).find('#encryption-' + daemon.settings.encryption).parent().checkbox('set checked');
  }
  render() {
    return(
      <div ref="root">
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Encryption</div>
        </div>
        <div className="ui inverted padded grid">
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="encryption-default" type="radio" name="encryption" value="default" />
                <label for="encryption-default">Automatic
                <small>
                  Good combination of speed and security.
                </small>
                </label>
              </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-none" type="radio" name="encryption" value="none" />
              <label for="encryption-none">None
              <small>
                Fastest speed but lower security.
              </small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-strong" type="radio" name="encryption" value="strong" />
              <label for="encryption-strong">Strong
              <small>Higher security but slower speed.</small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-stealth" type="radio" name="encryption" value="stealth" />
              <label for="encryption-stealth">Stealth
              <small>Suitable for restrictive network environments.</small>
              </label>
            </div>
            </div>
          </div>
        </div>
      </div>
    );
  }
}
