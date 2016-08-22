import React from 'react';
import { Link } from 'react-router';

export default class Login extends React.Component  {
  render(){



    return(
      <div className="startup">
        <img src="./img/punk.svg" />
        <h1>Cypherpunk</h1>
        <div><input /></div>
        <div><input /></div>
        <div><Link className="button" to="/interface">Log in</Link></div>

          <span className="button">Forgot Password?</span>
          <span className="button">Create Free Account</span>
      </div>
    );
  }
}
