import React from 'react';
import { Link } from 'react-router';
import { Subpanel, PanelContent } from '../Panel';

export default class TrustedNetworksScreen extends React.Component  {
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <div className="ui fluid inverted borderless icon menu cp_config_header titlebar">
            <Link className="nondraggable item" to="/configuration"><i className="angle left icon"></i></Link>
            <div className="header item center aligned">Manage Trusted Networks</div>
          </div>

          <div className="cp-panel__rows">
            <div className="setting">
              <div className="ui toggle checkbox">
                <h1 className="title">Auto-secure</h1>
                <div className="descrip">Cypherpunk can automatically secure connections to untrusted networks.</div>
                <input type="checkbox" name="autosecure" id="autosecure" ref="autosecure"/>
                <label>Auto secure connections</label>
              </div>
            </div>
            <div className="setting">
              <div className="ui toggle checkbox">
                <h1 className="title">Other Networks</h1>
                <input type="checkbox" name="autosecure-other" id="autosecure-other" ref="autosecure-other"/>
                <label>Auto secure connections</label>
              </div>
            </div>
            <div className="fieldgroup">
              <div className="setting">
                <div className="ui toggle checkbox">
                  <h1 className="title">WiFi Networks</h1>
                  <div className="descrip">Add the networks you trust so Cypherpunk will know when connections should be secured.</div>
                  <input type="checkbox" name="autosecure-wifi-1" id="autosecure-wifi-1" ref="autosecure-wifi-1"/>
                  <label>WiFi 1</label>
                </div>
              </div>
              <div className="setting">
                <div className="ui toggle checkbox">
                  <input type="checkbox" name="autosecure-wifi-1" id="autosecure-wifi-1" ref="autosecure-wifi-1"/>
                  <label>WiFi 2</label>
                </div>
              </div>
              <div className="setting">
                <div className="ui toggle checkbox">
                  <input type="checkbox" name="autosecure-wifi-1" id="autosecure-wifi-1" ref="autosecure-wifi-1"/>
                  <label>WiFi 3</label>
                </div>
              </div>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
