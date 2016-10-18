import React from 'react';
import ReactDOM from 'react-dom';
import LoginScreen from '../components/LoginScreen';
import ConnectScreen from '../components/ConnectScreen';
import ReactCSSTransitionGroup from 'react-addons-css-transition-group';

export default class Root extends React.Component {
  render() {
    var { children, location } = this.props;
    return(
      <div>
        <ReactCSSTransitionGroup
          component="div"
          transitionName="example"
          transitionEnterTimeout={500}
          transitionLeaveTimeout={500}
        >
          {React.cloneElement(children, {
            key: location.pathname
          })}
        </ReactCSSTransitionGroup>
      </div>
    );
  };
}
