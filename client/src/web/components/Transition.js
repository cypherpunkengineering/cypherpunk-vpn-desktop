import React from 'react';
import ReactDOM from 'react-dom';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');


export class FirstChild extends React.Component {
  render() {
    var children = React.Children.toArray(this.props.children);
    return children[0] || null;
  }
}

export class TransitionContainer extends React.Component {
  render() {
    return(
      <div className="transition-container">
        {this.props.children}
      </div>
    );
  }
}

const transitionDurations = {
  'example': 350,
  'nudgeLeft': 700,
  'nudgeRight': 700,
  'swipeLeft': 350,
  'swipeRight': 350,
}

export default class RouteTransition extends React.Component {
  constructor(props) {
    super(props);
  }
  state = {
    oldKey: null,
  }
  getProp(name, def) {
    if (this.props.hasOwnProperty(name))
      return this.props[name];
    else if (this.props.route) // fallback; can read out of <Route component={RouteTransition}> props too
      return this.props.route[name];
    else
      return def;
  }
  determineTransition(from, to) {
    if (from !== to) {
      var transition = this.getProp('transition');
      if (typeof transition === 'string') {
        // Transition specified as simple constant string
        return transition;
      } else if (typeof transition === 'function') {
        // Transition specified via delegate function
        return transition(from, to) || '';
      } else if (typeof transition === 'object') {
        // Transition specified via two-level glob lookup map
        function lookupMap(key, map) {
          do {
            if (key in map) {
              return map[key];
            }
            key = key.replace(/[^/]*\/?$/, '*');
            if (key in map) {
              return map[key];
            }
            key = key.replace(/\/?\*$/, '');
          } while (key);
        }
        var map = lookupMap(from || '', transition);
        if (map) {
          var name = lookupMap(to || '', map);
          if (name) {
            return name;
          }
        }
      }
    }
    return '';
  }
  getChildKey(child) {
    return child.props.route && child.props.route.path || child.props.id || child.props.key;
  }
  getCurrentKey(props) {
    var children = React.Children.toArray(props.children);
    if (children.length > 0) {
      return this.getChildKey(children[0]);
    }
  }
  componentDidMount() {
    console.log("componentDidMount");
  }
  componentWillUnmount() {
    console.log("componentWillUnmount");
  }
  componentWillReceiveProps(nextProps) {
    console.log("Saving old location " + this.getCurrentKey(this.props));
    this.setState({ oldKey: this.getCurrentKey(this.props) });
    //var currentKey = this.getFirstChildKey(this.props);
    //var newKey = this.getFirstChildKey(nextProps);
    //if (newKey !== currentKey) {
    //  this.setState({ oldKey: currentKey });
    //}
  }
  render() {
    var { children, location } = this.props;
    var from = this.state.oldKey;
    var to = this.getCurrentKey(this.props);
    var transition = this.determineTransition(from, to);
    var transitionTime = transitionDurations[transition] || 350;
    if (from !== to) {
      console.log(`Transitioning from ${from} to ${to} using ${JSON.stringify(transition)} (${transitionTime}ms)`);
    }
    return(
      <ReactCSSTransitionGroup
        component={this.props.component || TransitionContainer}
        transitionName={transition}
        transitionEnter={!!transition}
        transitionLeave={!!transition}
        transitionEnterTimeout={transitionTime}
        transitionLeaveTimeout={transitionTime}
        >
        {React.Children.map(children, c => React.cloneElement(c, { key: this.getChildKey(c) }))}
      </ReactCSSTransitionGroup>
    );
  }
}
