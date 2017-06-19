import React from 'react';
import { Link } from 'react-router';
import { SecondaryTitlebar } from '../Titlebar';
import { Subpanel, PanelContent } from '../Panel';

export default class EmailScreen extends React.Component  {
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title="Update Email" back="/account"/>
          <div className="content" ref="root">
            <div className="input pane">
              <div class="ui left icon input">
                <input type="text" ref="newEmail" placeholder="New email address" />
                <i className="envelope icon"/>
              </div>
              <div class="ui left icon input">
                <input type="text" ref="confirmEmail" placeholder="Confirm email address" />
                <i className="envelope icon"/>
              </div>
            </div>
            <div className="input pane">
              <div class="ui left icon input">
                <input type="text" ref="password" placeholder="Current password" />
                <i className="unlock icon"/>
              </div>
            </div>
            <div style={{ flex: 'auto' }} />
            <div className="input pane">
              <button id="updateEmail">Update</button>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
