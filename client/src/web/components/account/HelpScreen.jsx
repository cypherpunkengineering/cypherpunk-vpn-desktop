import React from 'react';
import { Link } from 'react-router';
import { Subpanel, PanelContent } from '../Panel';

export default class HelpScreen extends React.Component  {
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <div className="ui fluid inverted borderless icon menu cp_config_header" ref="root">
            <Link className="nondraggable item" to="/account"><i className="angle left icon"></i></Link>
            <div className="header item center aligned">Help</div>
          </div>
          <div class="ui padded grid">
            <div class="row">
              <div class="sixteen wide olive column">
                Write Help Information Here.
              </div>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
