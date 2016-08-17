import React from 'react';

class SettingsButton extends React.Component {
  render(){
    return (
      <div>
        <img src="./img/icon_settings.svg" />
      </div>
    );
  }
}

export default class Header extends React.Component  {
  render(){
    return(
      <header>
        <div><img src="./img/icon_account.svg" /></div>
        <div><img src="./img/logo_gray.svg" /><span>Cypherpunk</span></div>
        <SettingsButton />
      </header>
    );
  }
}
