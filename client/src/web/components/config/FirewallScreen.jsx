import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';


export default class FirewallScreen extends DaemonAware(React.Component)  {
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.root).find('.ui.checkbox').click(this.onClick.bind(this)).checkbox({ onChange: this.onChange.bind(this) });
    this.onDaemonSettingsChanged();
  }
  onChange() {
    if (this.updatingSettings) return;
    var value = $(this.refs.root).find('input[name=firewall]:checked').val();
    daemon.post.applySettings({ 'firewall': value });
  }
  onClick() {
    setImmediate(() => { History.push('/configuration'); });
  }
  onDaemonSettingsChanged() {
    this.updatingSettings = true;
    $(this.refs.root).find('#firewall-' + daemon.settings.firewall).parent().checkbox('set checked');
    delete this.updatingSettings;
  }
  render() {
    return(
      <div className="panel" ref="root" id="settings-firewall-panel">
        <SecondaryTitlebar title="Internet Killswitch" back="/configuration"/>
        <div className="scrollable content">
          <div className="pane" style={{ padding: '1em' }}>
            Prevent accidental privacy leaks by disabling all internet connectivity if you are disconnected from the Cypherpunk Privacy Network.
          </div>
          <div className="pane">
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="firewall" value="auto" id="firewall-auto"/>
                <label>Automatic (recommended)
                <small>Enable the killswitch while connecting, protecting you in case of accidental disconnections.</small>
                </label>
              </div>
            </div>
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="firewall" value="off" id="firewall-off"/>
                <label>Off
                <small>Don't use the killswitch feature.</small>
                </label>
              </div>
            </div>
          </div>
          <div className="pane">
            <div className="setting">
              <div className="ui left top radio checkbox">
                <input type="radio" name="firewall" value="on" id="firewall-on"/>
                <label>Always On
                <small>Only allow internet access through the Cypherpunk Privacy Network.</small>
                <small style={{ marginTop: '20px', /*borderTop: '1px solid rgba(255,255,255,0.25)', paddingTop: '10px',*/ marginBottom: '5px' }}><i class="warning sign icon"></i>WARNING:</small>
                <small>You will not be able to access the internet while disconnected!</small>
                </label>
              </div>
            </div>
          </div>
        </div>
      </div>
    );
      /*
      <div ref="root" class="container__comp">
        <div className="ui fluid inverted borderless icon menu cp_config_header">
          <Link className="nondraggable item" to="/configuration"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Internet Killswitch</div>
        </div>
        <div className="ui inverted padded grid">
          <div className="row">
            <div className="olive column">
              Prevent accidental privacy leaks by disabling all internet connectivity if you are disconnected from the Cypherpunk Privacy Network.
            </div>
          </div>
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="firewall-auto" type="radio" name="firewall" value="auto" />
                <label for="firewall-auto">Automatic (recommended)
                  <small>
                    Enable the killswitch while connecting to Cypherpunk Privacy, protecting you in case of accidental disconnections.
                  </small>
                </label>
              </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="firewall-off" type="radio" name="firewall" value="off" />
              <label for="firewall-off">Off
                <small>
                  Don't use the killswitch feature.
                </small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="firewall-on" type="radio" name="firewall" value="on" />
                <label for="firewall-on">Always On
                  <small>
                    Keep the killswitch permanently on.
                  </small>
                  <small>
                    <i class="warning icon"></i>WARNING:
                  </small>
                  <small>
                    You will not be able to access the internet while disconnected!
                  </small>
                </label>
              </div>
            </div>
          </div>
        </div>

      </div>
    );*/
  }
}
