import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';
import { Subpanel, PanelContent } from '../Panel';
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
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title="Encryption" back="/configuration"/>
          <div className="scrollable content" ref="root">
            <div className="pane">
              <div className="setting">
                <div className="ui left top radio checkbox">
                  <input type="radio" name="encryption" value="default" id="encryption-default"/>
                  <label>Balanced<span class="recommended">Recommended</span>
                  <small>Good balance of speed and privacy.</small>
                  <div className="encryption-details">128-bit AES</div>
                  </label>
                </div>
              </div>
              <div className="setting">
                <div className="ui left top radio checkbox">
                  <input type="radio" name="encryption" value="none" id="encryption-none"/>
                  <label>Max Speed
                  <small>Fastest speed by disabling encryption.</small>
                  <div className="encryption-details" data-position="bottom left" data-tooltip="Your traffic will be sent unencrypted and could be intercepted by hostile parties."><i className="fitted warning sign icon"/> UNENCRYPTED</div>
                  </label>
                </div>
              </div>
              <div className="setting">
                <div className="ui left top radio checkbox">
                  <input type="radio" name="encryption" value="strong" id="encryption-strong"/>
                  <label>Max Privacy
                  <small>Use strongest available encryption.</small>
                  <div className="encryption-details">256-bit AES</div>
                  </label>
                </div>
              </div>
              <div className="setting">
                <div className="ui left top radio checkbox">
                  <input type="radio" name="encryption" value="stealth" id="encryption-stealth"/>
                  <label>Max Stealth
                  <small>Anti-censorship for hostile networks.</small>
                  <div className="encryption-details">128-bit AES + XOR on HTTPS port</div>
                  </label>
                </div>
              </div>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
