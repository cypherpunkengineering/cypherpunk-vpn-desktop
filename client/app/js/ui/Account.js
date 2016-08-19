import React from 'react';
import { Link } from "react-router";

export default class Account extends React.Component  {
  render(){
    return(
      <div>
          <Link className="clicky" to="/">Logout</Link>
      </div>
    );
  }
}
