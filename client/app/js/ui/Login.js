import React from 'react';
import { Link } from 'react-router';

export default class Login extends React.Component  {
  render(){
    return(
      <div className="startup">
        <img src="./img/punk.svg" />
        <h1>Cypherpunk</h1>
        <div><input value="email" /></div>
        <div><input value="password" /></div>
        <div><Link to="vpn">Log in</Link></div>
        <div>SIGN UP</div>
      </div>
    );
  }
}
