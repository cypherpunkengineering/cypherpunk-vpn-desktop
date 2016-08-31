import React from 'react';
import { Link } from 'react-router';
import { Motion, spring } from 'react-motion';

export default class Login extends React.Component  {
  render(){

    const logintext1 = "Username/Email";
    const logintext2 = "Password";

    return(
<div>
      <Motion defaultStyle={{opacity: 0}} style={{opacity: spring(1)}}>
      {interpolatingStyle =>
      <div style={interpolatingStyle} className="startup">

        <img src="./img/punk.svg" />
        <h1>Cypherpunk!</h1>
        <div><input className="cp_input01" defaultValue={logintext1} /></div>
        <div><input className="cp_input01" defaultValue={logintext2} /></div>
        <div><Link className="cp_button01" to="/connect">Log in</Link></div>

          <div className="forgot_password">Forgot password?</div>
          <div className="sign_up">Sign Up</div>

      </div>
      }
      </Motion>

<span> hello </span> 
    </div>

    );
  }
}
