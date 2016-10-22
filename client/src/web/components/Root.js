import React from 'react';
import ReactDOM from 'react-dom';
import LoginScreen from '../components/LoginScreen';
import ConnectScreen from '../components/ConnectScreen';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');

export default class Root extends React.Component {
  constructor(props) {
    super(props);
    this.cloneChildren = this.determineTransition.bind(this);
  }
  componentWillReceiveProps() {
    window.previousLocation = this.props.location
  }
  determineTransition(segment) {
    var transitionName = 'fadeIn'
    if (segment === 'login' || segment === 'root') {
      transitionName = '';
    }
    else if (window.previousLocation.pathname === '/login') {
      transitionName = 'fadeIn'
    }
    else if (window.previousLocation.pathname === '/account') {
      transitionName = 'pageSwap';
    }
    else if (segment === 'connect' || segment === 'account' || window.previousLocation.pathname === '/email' || window.previousLocation.pathname === '/password' || window.previousLocation.pathname === '/encryption' || window.previousLocation.pathname === '/firewall' || window.previousLocation.pathname === '/help') {
      transitionName = 'reversePageSwap';
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
        <ReactCSSTransitionGroup
          component="div"
          transitionName={this.determineTransition(segment)}
          transitionEnterTimeout={350}
          transitionLeaveTimeout={350}
        >
          {React.cloneElement(children, {
            key: segment
          })}
        </ReactCSSTransitionGroup>
    );
  };
}
