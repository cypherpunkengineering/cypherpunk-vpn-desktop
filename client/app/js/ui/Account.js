import React from 'react';
import { Link } from "react-router";

export default class Account extends React.Component  {
  render(){
    return(
      <div>
<div>display username</div>
<div>display current plan</div>
<div>upgrade account</div>
<div>change password</div>
<div>change email</div>
<div>help</div>



          <Link className="clicky" to="/">Logout</Link>
      </div>
    );
  }
}
