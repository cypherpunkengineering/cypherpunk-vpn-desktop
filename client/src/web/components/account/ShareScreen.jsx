import React from 'react';
import { Link } from 'react-router';
import { SecondaryTitlebar } from '../Titlebar';
import { Subpanel, PanelContent } from '../Panel';
const { shell } = require('electron').remote;

const FACEBOOK_HREF = 'https://www.facebook.com/sharer/sharer.php?u=cypherpunk.com';
const TWITTER_HREF = 'https://twitter.com/home?status=Checkout%20cypherpunk.com';
const EMAIL_HREF = 'mailto:?subject=Checkout Cypherpunk.com&body=I\'m using Cypherpunk\'s VPN and security products. You should too!';

export default class ShareScreen extends React.Component  {
  render() {
    return(
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title="Share" back="/account"/>
          <div className="content" ref="root">
            <div className="pane" style={{ padding: '1em', fontSize: '16px' }}>
              Like Cypherpunk Privacy?<br/>
              Please share with your friends!
            </div>
            <div className="share pane">
              <a onClick={() => { shell.openExternal(FACEBOOK_HREF)}}>
                <i className="facebook icon"/>
                Facebook
              </a>
              <a onClick={() => { shell.openExternal(TWITTER_HREF)}}>
                <i className="twitter icon"/>
                Twitter
              </a>
              <a onClick={() => { shell.openExternal(EMAIL_HREF)}}>
                <i className="envelope icon"/>
                Email
              </a>
            </div>
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
