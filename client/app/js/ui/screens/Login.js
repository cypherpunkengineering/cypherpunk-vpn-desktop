import React from 'react';
import { Link } from 'react-router';

export default class Login extends React.Component  {
  render(){

    const logintext1 = "Username/Email";
    const logintext2 = "Password";

    return(
      <div className="startup">
        <img src="./img/punk.svg" />
        <h1>Cypherpunk</h1>
        <div><input className="cp_input01" value={logintext1} /></div>
        <div><input className="cp_input01" value={logintext2} /></div>
        <div><Link className="cp_button01" to="/interface">Log in</Link></div>

          <div className="forgot_password">Forgot password?</div>
          <div className="sign_up">Sign Up</div>
      </div>
    );
  }
}
