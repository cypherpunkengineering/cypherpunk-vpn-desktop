import React from 'react';
import ReactDOM from 'react-dom';
import LoginScreen from '../components/LoginScreen';
import ConnectScreen from '../components/ConnectScreen';
import ReactCSSTransitionGroup from 'react-addons-css-transition-group';

export default class Root extends React.Component {
  constructor(props) {
    super(props);
    this.cloneChildren = this.determineTransition.bind(this);
  }
  componentWillReceiveProps() {
    window.previousLocation = this.props.location
  }
  determineTransition(segment) {
    var transitionName = 'example'
    if (segment === 'login' || segment === 'root') {
      transitionName = '';
    }
    else if (window.previousLocation.pathname === '/login') {
      transitionName = 'example'
    }
    else if (segment === 'connect' || window.previousLocation.pathname === '/email' || window.previousLocation.pathname === '/password' || window.previousLocation.pathname === '/encryption' || window.previousLocation.pathname === '/firewall' || window.previousLocation.pathname === '/help') {
      transitionName = 'reversePageSwap'
    }
    else {
      transitionName = 'pageSwap';
    }
    return transitionName;
  }
  render() {
    var { children, location } = this.props;

    var path = location.pathname;
    var segment = path.split('/')[1] || 'root';

    return(
      <div>
        <ReactCSSTransitionGroup
          component="div"
          transitionName={this.determineTransition(segment)}
          transitionEnterTimeout={600}
          transitionLeaveTimeout={600}
        >
          {React.cloneElement(children, {
            key: segment
          })}
        </ReactCSSTransitionGroup>
      </div>
    );
  };
}
