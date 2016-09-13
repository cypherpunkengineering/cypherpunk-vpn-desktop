import React from 'react';
import { Link } from 'react-router';


export default class FirewallScreen extends React.Component  {
  componentDidMount() {
    $(this.refs.tab).find('.item').tab();
  }
  render() {
    return(
      <div>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/configuration"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Firewall</div>
        </div>
      </div>
    );
  }
}
