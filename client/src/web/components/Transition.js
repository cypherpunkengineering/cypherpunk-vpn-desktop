import React from 'react';
import ReactDOM from 'react-dom';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');


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
  'reveal': 350,
}


// Component to wrap ReactCSSTransitionGroup with convenience functionality
// to fit inside a Router or just with children that are inserted by Routes.
// The transition to use is specified with the 'transition' prop, which is
// a function(from, to) or map[from][to] where from/to are the route path
// components associated with the children entering/leaving.

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
    if (from !== to && (from || to)) {
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
          if ('' in map) {
            return map[''];
          }
          if (null in map) {
            return map[null];
          }
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
    return child.props.route && child.props.route.path || null /*|| child.props.id || child.props.ref || child.props.name*/;
  }
  getCurrentKey(props) {
    var children = React.Children.toArray(props.children);
    for (var i = 0; i < children.length; i++) {
      if (typeof (children[i]) === 'object') {
        return this.getChildKey(children[0]);
      }
    }
  }
  componentWillReceiveProps(nextProps) {
    this.setState({ oldKey: this.getCurrentKey(this.props) });
  }
  render() {
    var { children, location } = this.props;
    var from = this.state.oldKey;
    var to = this.getCurrentKey(this.props);
    var transition = this.determineTransition(from, to);
    var transitionTime = transitionDurations[transition] || 350;
    if ((from || to) && from !== to && transition) {
      console.log(`Transitioning from ${from} to ${to} using ${JSON.stringify(transition)} (${transitionTime}ms)`);
    }
    return(
      <ReactCSSTransitionGroup
        component={this.props.component || TransitionContainer}
        transitionName={transition}
        transitionEnter={!!transition}
        transitionLeave={!!transition}
        transitionAppear={!!transition}
        transitionEnterTimeout={transitionTime}
        transitionLeaveTimeout={transitionTime}
        transitionAppearTimeout={transitionTime}
        >
        {React.Children.map(children, c => c ? React.cloneElement(c, { key: this.getChildKey(c) }) : null)}
      </ReactCSSTransitionGroup>
    );
  }
}
