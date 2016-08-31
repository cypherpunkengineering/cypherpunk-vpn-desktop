import React from 'react';
import { Link } from "react-router";

export default class Header extends React.Component  {
  render(){
    return(
      <header>
        <AccountButton />
        <div><span>Cypherpunk</span></div>
        <SettingsButton />
      </header>
    );
  }
}

class SettingsButton extends React.Component {
  render(){
    const linkedTo =  location.hash.match(/settings/) ? "/interface" : "/settings";
    return (
        <div>
          <Link className="clicky" to={linkedTo}><img src="./img/icon_settings.svg" /></Link>
        </div>
    );
  }
}

class AccountButton extends React.Component {
  render(){
    const linkedTo = location.hash.match(/account/) ? "/interface" : "/account";
    return (
        <div>
          <Link className="clicky" to={linkedTo}><img src="./img/account_info_icon.svg" height="23px" /></Link>
        </div>
    );
  }
}
