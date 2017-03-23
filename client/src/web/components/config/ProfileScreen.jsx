import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';
import analytics from '../../analytics';


export default class PrivacyScreen extends DaemonAware(React.Component)  {
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.root).find('.ui.checkbox').click(this.onClick.bind(this)).checkbox({ onChange: this.onChanged.bind(this) });
    this.onDaemonSettingsChanged();
  }
  onChanged() {
    if (this.updatingSettings) return;
    var value = $(this.refs.root).find('input[name=encryption]:checked').val();
    daemon.post.applySettings({ 'encryption': value });
    analytics.event('Setting', 'encryption', { label: value });
  }
  onClick() {
    setImmediate(() => { History.push('/configuration'); });
  }
  onDaemonSettingsChanged() {
    this.updatingSettings = true;
    $(this.refs.root).find('#encryption-' + daemon.settings.encryption).parent().checkbox('set checked');
    delete this.updatingSettings;
  }
  render() {
    return(
      <div className="panel" ref="root" id="settings-privacy-panel">
        <SecondaryTitlebar title="Tunnel Mode" back="/configuration"/>
        <div className="scrollable content">
          <div className="pane">
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="encryption" value="default" id="encryption-default"/>
                <label>Recommended Default
                <small>Good balance of speed and privacy.</small>
                <div className="encryption-details"><span data-title="Cipher">AES-128-GCM</span><span data-title="Auth">SHA-256</span><span data-title="Key">RSA-4096</span></div>
                </label>
              </div>
            </div>
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="encryption" value="none" id="encryption-none"/>
                <label>Max Speed
                <small>Fastest speed by using no additional encryption.</small>
                <div className="encryption-details"><span data-title="Cipher">NONE</span><span data-title="Auth">SHA-256</span><span data-title="Key">RSA-4096</span></div>
                </label>
              </div>
            </div>
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="encryption" value="strong" id="encryption-strong"/>
                <label>Max Privacy
                <small>Higher security by using stronger encryption.</small>
                <div className="encryption-details"><span data-title="Cipher">AES-256-GCM</span><span data-title="Auth">SHA-256</span><span data-title="Key">RSA-4096</span></div>
                </label>
              </div>
            </div>
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="encryption" value="stealth" id="encryption-stealth"/>
                <label>Max Stealth
                <small>Anti-censorship stealth encryption.</small>
                <div className="encryption-details"><span data-title="Cipher">AES-128-GCM</span><span data-title="Auth">SHA-256</span><span data-title="Key">RSA-4096</span></div>
                </label>
              </div>
            </div>
          </div>
        </div>
      </div>
    );
  }
}
