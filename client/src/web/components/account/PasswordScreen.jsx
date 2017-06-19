import React from 'react';
import { Link } from 'react-router';
import { SecondaryTitlebar } from '../Titlebar';
import { Subpanel, PanelContent } from '../Panel';

export default class PasswordScreen extends React.Component  {
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title="Change Password" back="/account"/>
          <div className="content" ref="root">
            {/*<div className="pane" style={{ padding: '1em' }}>
              
            </div>*/}
            <div className="input pane">
              <div class="ui left icon input">
                <input type="text" ref="newPassword" placeholder="New password" />
                <i className="key icon"/>
              </div>
              <div class="ui left icon input">
                <input type="text" ref="confirmPassword" placeholder="Confirm new password" />
                <i className="key icon"/>
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
              <button id="updatePassword">Update</button>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
