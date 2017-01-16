import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';
import '../../util.js';

const REMOTE_PORT_ALTERNATIVES = [ [ 'udp', [ 7133, 5060, 53 ] ], [ 'tcp', [ 7133, 443 ] ] ];

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
      <div className="panel" ref="root" id="settings-remotePort-panel">
        <SecondaryTitlebar title="Remote Port" back="/configuration"/>
        <div className="scrollable content">
          {
            REMOTE_PORT_ALTERNATIVES.map(([protocol, ports]) =>
              <div className="pane" data-title={protocol.toUpperCase()} key={protocol}>
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
      </div>
    );
  }
}
