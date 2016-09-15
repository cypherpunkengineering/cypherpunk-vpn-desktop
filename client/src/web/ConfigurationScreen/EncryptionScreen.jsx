import React from 'react';
import { Link } from 'react-router';

export default class EncryptionScreen extends React.Component  {
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Encryption</div>
        </div>
        <div className="ui inverted padded grid">
          <div className="row">
            <div className="olive column">
              <div className="ui radio checkbox">
                <input id="encryption-automatic" type="radio" name="encryption" />
                <label for="encryption-automatic">Automatic
                <small>
                  Best combination of speed and security.
                </small>
                </label>
              </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-none" type="radio" name="encryption" />
              <label for="encryption-none">None
              <small>
                Fastest speed but lower security.
              </small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-strong" type="radio" name="encryption" />
              <label for="encryption-strong">Strong
              <small>Higher security but slower speed.</small>
              </label>
            </div>
            </div>
          </div>
          <div className="row">
            <div className="olive column">
            <div className="ui radio checkbox">
              <input id="encryption-stealth" type="radio" name="encryption" />
              <label for="encryption-stealth">Stealth
              <small>Proprietary Encryption best for bypassing restrictive network firewalls such as The Great Firewall of China.</small>
              </label>
            </div>
            </div>
          </div>
        </div>
      </div>
    );
  }
}
