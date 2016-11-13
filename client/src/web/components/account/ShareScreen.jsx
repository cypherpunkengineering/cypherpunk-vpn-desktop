import React from 'react';
import { Link } from 'react-router';
const { shell } = require('electron').remote;

export default class EmailScreen extends React.Component  {
  render() {
    return(
      <div class="container__comp material">
        <div className="ui fluid inverted borderless icon menu cp_config_header titlebar">
          <Link className="nondraggable item" to="/account"><i className="angle left icon"></i></Link>
          <div className="header item center aligned">Update Email</div>
        </div>

        <div className="cp-panel__rows">
          <div className="link__share--fb row">
            <a onClick={() => { shell.openExternal('https://www.facebook.com/sharer/sharer.php?u=cypherpunk.com')}}>
              <i className="icon facebook"></i>
              Facebook
            </a>
          </div>
          <div className="link__share--twitter row">
            <a onClick={() => { shell.openExternal('https://twitter.com/home?status=Checkout%20cypherpunk.com')}}>
              <i className="icon twitter"></i>
              Twitter
              </a>
          </div>
          <div className="link__share--email row">
            <a onClick={() => { shell.openExternal('mailto:?subject=Checkout Cypherpunk.com&body=I\'m using Cypherpunk\'s VPN and security products. You should too!')}}>
              <i className="icon envelope"></i>
              Email
            </a>
          </div>
      </div>
    </div>
    );
  }
}
