import React from 'react';
import ReactDOM from 'react-dom';
import LoginScreen from '../components/LoginScreen';
import ConnectScreen from '../components/ConnectScreen';
import ReactCSSTransitionGroup from 'react-addons-css-transition-group';

export default class Root extends React.Component {
  render() {
    var { children, location } = this.props;

    var path = location.pathname;
    var segment = path.split('/')[1] || 'root';
    return(
      <div>
        <ReactCSSTransitionGroup
          component="div"
          transitionName="pageSlider"
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
