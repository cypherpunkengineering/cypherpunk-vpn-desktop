import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';
import '../../util.js';
import { Subpanel, PanelContent } from '../Panel';
import analytics from '../../analytics';

const REMOTE_PORT_ALTERNATIVES = [ [ 'udp', [ 7133, 5060, 53 ] ], [ 'tcp', [ 7133, 5060, 53 ] ] ];
const PROTOCOL_DESCRIPTIONS = {
  'udp': "Usually gives the best speeds.",
  'tcp': "Less likely to be blocked.",
}

export default class RemotePortScreen extends DaemonAware(React.Component)  {
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.root).find('.ui.checkbox').click(this.onClick.bind(this)).checkbox({ onChange: this.onChanged.bind(this) });
    this.onDaemonSettingsChanged();
  }
  onChanged() {
    if (this.updatingSettings) return;
    var value = $(this.refs.root).find('input[name=remotePort]:checked').val();
    daemon.post.applySettings({ 'remotePort': value });
    analytics.event('Setting', 'remotePort', { label: value });
  }
  onClick() {
    setImmediate(() => { History.push('/configuration'); });
  }
  onDaemonSettingsChanged() {
    this.updatingSettings = true;
    $(this.refs.root).find('#remotePort-' + daemon.settings.remotePort.replace(':', '-')).parent().checkbox('set checked');
    delete this.updatingSettings;
  }
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title="Remote Port" back="/configuration"/>
          <div className="scrollable content" ref="root">
            {
              REMOTE_PORT_ALTERNATIVES.map(([protocol, ports]) =>
                <div className="pane" data-title={protocol.toUpperCase()} key={protocol}>
                  <div className="setting description">{PROTOCOL_DESCRIPTIONS[protocol]}</div>
                  {
                    ports.map(port =>
                      <div className="setting" key={port}>
                        <div className="ui left top radio checkbox">
                          <input type="radio" name="remotePort" value={`${protocol}:${port}`} id={`remotePort-${protocol}-${port}`}/>
                          <label>{port}</label>
                        </div>
                      </div>
                    )
                  }
                </div>
              )
            }
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
