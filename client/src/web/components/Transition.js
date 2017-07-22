import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');
const ReactTransitionGroup = reactAddon(require('react-addons-transition-group'), 'TransitionGroup');





const DEFAULT_DURATION = 350;
const TRANSITION_DURATIONS = {
  'example': 350,
  'nudgeLeft': 700,
  'nudgeRight': 700,
  'swipeLeft': 350,
  'swipeRight': 350,
  'reveal': 350,
  'fadeIn': 350,
  'tutorial': 700,
};








function getComponentKey(child, index = 0) {
  if (child && typeof child === 'object') {
    if (child.props.route && child.props.route.path && (!child.key || child.key[0] != '.')) {
      return child.props.route.path;
    }
    if (child.key != null) {
      return child.key;
    }
  }
  return index.toString(36);
}
function getChildMapping(children) {
  var result = {};
  React.Children.forEach(children, (child, index) => {
    result[getComponentKey(child, index)] = child;
  });
  return result;
}
// This is a stupid carry-over from React where we rely on the practical reality that all
// ecmascript engines preserve (at least non-numerical) object properties in insertion
// order. Thus we need to do some special logic when merging two child mappings.
function mergeChildMappings(prev, next) {
  prev = prev || {};
  next = next || {};

  function value(key) {
    return next.hasOwnProperty(key) ? next[key] : prev[key];
  }
  let prevKeysPrecedingNextKeys = {};
  let pending = [];
  for (let key in prev) {
    if (prev.hasOwnProperty(key)) {
      if (next.hasOwnProperty(key)) {
        if (pending.length) {
          prevKeysPrecedingNextKeys[key] = pending;
          pending = [];
        }
      } else {
        pending.push(key);
      }
    }
  }

  let result = {};
  for (let key in next) {
    if (next.hasOwnProperty(key)) {
      if (prevKeysPrecedingNextKeys.hasOwnProperty(key)) {
        prevKeysPrecedingNextKeys[key].forEach(k => {
          result[k] = value(k);
        });
      }
    }
    result[key] = value(key);
  }
  pending.forEach(k => {
    result[k] = value(k);
  });

  return result;
}


export const TransitionContainer = ({ className, location, params, route, routes, router, routeParams, ...props } = {}) => <div className={classList('transition-container', className)} {...props}/>;


class TransitionGroupChild extends React.Component {
  componentWillUnmount() {
    this.props.onUnmount();
  }
  render() {
    return React.Children.only(this.props.children);
  }
}


export class TransitionGroup extends React.Component {
  static defaultProps = {
    component: TransitionContainer,
    transition: null,
  }
  constructor(props) {
    super(props);
    let children = getChildMapping(props.children);
    this.currentTransitions = {};
    this.pendingTransitions = {};
    Object.forEach(children, (key, child) => {
      if (child) {
        this.pendingTransitions[key] = 'appear';
      }
    });
    this.state = {
      children: children,
      transitions: this.getTransitionMap(children),
    };
    // Remove any children without appear transitions from the queue
    Object.forEach(children, (key, child) => {
      if (!this.state.transitions[key]) {
        delete this.state.transitions[key];
        delete this.pendingTransitions[key];
      }
    });

  }
  componentWillMount() {
    this.currentTransitions = {}; // { key : { type: 'appear'|'enter'|'leave', name: transition } }
    this.pendingTransitions = {}; // { key : 'appear'|'enter'|'leave' }
    this.thisFrameCallbacks = {};
    this.nextFrameCallbacks = {};
    this.nextFrameRequested = null;
    this.transitionTimeouts = {};
  }
  componentDidMount() {
    this.performPendingTransitions();
  }
  componentWillUnmount() {
    if (this.nextFrameRequested) {
      cancelAnimationFrame(this.nextFrameRequested);
      this.nextFrameRequested = null;
    }
    this.thisFrameCallbacks = {};
    this.nextFrameCallbacks = {};
    Object.forEach(this.transitionTimeouts, (key, id) => { clearTimeout(id); });
  }
  componentWillReceiveProps(nextProps) {
    // This is the target set of children that we should be transitioning towards
    let targetChildren = getChildMapping(nextProps.children);

    // For each child, its 'effective presence' is determined by 'applying' current or pending transitions
    let effectivelyPresent = (key) => (
      this.state.children.hasOwnProperty(key) && this.state.children[key] && (
        (this.currentTransitions.hasOwnProperty(key) && this.currentTransitions[key].type === 'leave') // Check if the child is currently leaving
          ? (this.pendingTransitions[key] === 'enter') // Present only if there is a pending enter transition
          : (this.pendingTransitions[key] !== 'leave') // Present unless there is a pending leave transition
      )
    );
    let shouldBePresent = (key) => (targetChildren.hasOwnProperty(key) && targetChildren[key]);

    Object.forEach(targetChildren, (key, child) => {
      if (child) {
        if (!effectivelyPresent(key)) {
          if (this.pendingTransitions[key] === 'leave') {
            // Child was scheduled to be removed but was readded again; skip transition
            delete this.pendingTransitions[key];
          } else {
            // Child is either being added, or we're queueing up an enter transition after the current leave transition finishes
            this.pendingTransitions[key] = 'enter';
          }
        }
      }
    });
    Object.forEach(this.state.children, (key, child) => {
      if (effectivelyPresent(key) && !shouldBePresent(key)) {
        if (this.pendingTransitions[key] === 'enter') {
          // Child was scheduled to be added, but was removed again; skip transition
          delete this.pendingTransitions[key];
        } else {
          // Child is being removed
          this.pendingTransitions[key] = 'leave';
        }
      }
    });

    // We're done with the comparison, so merge any new children into the map
    let newChildren = mergeChildMappings(this.state.children, targetChildren);

    // Consult the transition specification for which transition to use in this situation
    let transitionSpecification = this.getTransitionMap(newChildren);

    // If any children have a null transition, immediately apply it
    Object.forEach(newChildren, (key, child) => {
      if (!this.currentTransitions.hasOwnProperty(key) && !transitionSpecification[key]) {
        if (this.pendingTransitions[key] === 'leave') {
          delete newChildren[key];
        }
        delete transitionSpecification[key];
        delete this.pendingTransitions[key];
      }
    });

    // Apply the new state to trigger an update
    this.setState({
      children: newChildren,
      transitions: transitionSpecification,
    });
  }
  componentDidUpdate() {
    this.performPendingTransitions();
  }
  render() {
    let childrenToRender = [];
    Object.forEach(this.state.children, (key, child) => {
      if (child) {
        childrenToRender.push(React.createElement(TransitionGroupChild, { ref: key, key: key, onUnmount: this.childWillUnmount.bind(this, key) }, child));
      }
    });
    let props = Object.assign({}, this.props);
    delete props.component;
    delete props.transition;
    return React.createElement(this.props.component, props, childrenToRender);
  }


  getTransitionMap(children) {
    let result;
    switch (typeof this.props.transition) {
      case 'function':
        // Call callback with a { key: type } map of all current or pending transitions
        let allChildren = Object.assign(Object.mapValues(this.currentTransitions, (k, v) => v.type), this.pendingTransitions);
        if (Object.keys(allChildren).length > 0) {
          //console.log("getTransition in:", allChildren);
          result = this.props.transition(allChildren);
          //console.log("getTransition out:", result);
          if (!result || typeof result === 'string') {
            result = { '*': result };
          }
        } else {
          result = { '*': null };
        }
        break;
      case 'string':
        result = { '*': this.props.transition };
        break;
      case 'object':
        result = this.props.transition ? Object.assign({}, this.props.transition) : { '*': null };
        break;
      default:
        result = { '*': null };
        break;
    }
    var fallback = result['*'] || null;
    delete result['*'];
    Object.forEach(children, (key, type) => {
      if (!result.hasOwnProperty(key)) result[key] = fallback;
    });
    return result;
  }
  performPendingTransitions() {
    // Copy over any pending transitions we can apply (i.e. elements that are not already transitioning)
    let pendingTransitions = {};
    for (let key of Object.keys(this.pendingTransitions)) {
      if (!this.currentTransitions.hasOwnProperty(key)) {
        pendingTransitions[key] = this.pendingTransitions[key];
        delete this.pendingTransitions[key]; // only safe because we use Object.keys() above
      }
    }
    Object.forEach(pendingTransitions, (key, type) => {
      let transitionName = this.state.transitions[key];
      let transition = { type, name: transitionName };
      let duration = TRANSITION_DURATIONS[transitionName] || DEFAULT_DURATION;
      this.currentTransitions[key] = transition;
      let component = this.refs[key];

      //console.log("Beginning transition for", key, transition);

      let className = `${transitionName}-${type}`
      let activeClassName = `${className}-active`;

      //console.log(`Applying class ${className} to ${key}`);
      component.dom.classList.add(className);
      this.queueNextFrameCallback(key, () => {
        //console.log(`Applying class ${activeClassName} to ${key}`);
        component.dom.classList.add(activeClassName);
      });
      this.transitionTimeouts[key] = setTimeout(() => {
        delete this.transitionTimeouts[key];
        delete this.currentTransitions[key];
        //console.log(`Removing classes ${className} and ${activeClassName} from ${key}`);
        component.dom.classList.remove(className, activeClassName);
        if (this.pendingTransitions[key]) {
          // Queue a dummy function that appropriately triggers a new round of transitions
          this.queueNextFrameCallback(key, function() {});
        } else if (type === 'leave') {
          this.setState((state) => {
            let children = Object.assign({}, state.children);
            delete children[key];
            return { children };
          });
        }
      }, duration);
    });
  }
  handleTransitionDone(key) {
    let component = this.refs[key];
    if (component.componentDidTransition) {
      component.componentDidTransition();
    }
  }
  queueNextFrameCallback(key, cb) {
    this.nextFrameCallbacks[key] = cb;
    if (!this.nextFrameRequested) {
      let onFrame = () => {
        let callbacks = this.thisFrameCallbacks;
        this.thisFrameCallbacks = this.nextFrameCallbacks;
        this.nextFrameCallbacks = {};
        Object.forEach(callbacks, (key, cb) => cb());
        this.performPendingTransitions();
        if (Object.keys(this.thisFrameCallbacks).length > 0 || Object.keys(this.nextFrameCallbacks).length > 0) {
          requestAnimationFrame(onFrame);
        } else {
          this.nextFrameRequested = null;
        }
      };
      this.nextFrameRequested = requestAnimationFrame(onFrame);
    }
  }
  childWillUnmount(key) {
    delete this.thisFrameCallbacks[key];
    delete this.nextFrameCallbacks[key];
    if (this.transitionTimeouts[key]) clearTimeout(this.transitionTimeouts[key]);
    delete this.transitionTimeouts[key];
    delete this.currentTransitions[key];
    delete this.pendingTransitions[key];
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

// Determine which transition should be used based on a removed child keys,
// added child keys, and a transition specification (string, map or callback).
// Returns a map of { key: transitionName } for any items in either input to
// receive a transition effect, or a single string to apply the same transition
// to all items.
export function determineTransition(from, to, spec) {
  if (Array.isArray(from) != Array.isArray(to)) {
    throw new Error("Either both source and destination need to be arrays, or neither");
  }
  if (Array.isArray(from)) {
    if (from.length == to.length) {
      if (from.every((v, i) => v === to[i])) {
        return '';
      }
      if (typeof spec === 'string') {
        return spec;
      } else if (typeof spec === 'function') {
        return spec(from, to) || '';
      }
      if (from.length == 1) {
        return determineTransition(from[0], to[0], spec);
      }
    }
  }

  if (Array.isArray(from)) {
    for (var i = 0; i < from.length; i++) {
      var t = determineTransition(from[i], to, spec);
      if (t) return t;
    }
    return '';
  }
  if (Array.isArray(to)) {
    for (var i = 0; i < to.length; i++) {
      var t = determineTransition(from, to[i], spec);
      if (t) return t;
    }
    return '';
  }

  if (from !== to && (from || to)) {
    if (typeof spec === 'string') {
      // Transition specified as simple constant string
      return spec;
    } else if (typeof spec === 'function') {
      // Transition specified via delegate function
      return spec(from, to) || '';
    } else if (typeof spec === 'object') {
      // Transition specified via two-level glob lookup map
      function lookupMap(key, map) {
        if (key === '' || key === null || key === undefined) {
          if ('' in map) {
            return map[''];
          }
          if (null in map) {
            return map[null];
          }
        } else {
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
        if ('*' in map) {
          return map['*'];
        }
      }
      var map = lookupMap(from || '', spec);
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


// Component to wrap ReactCSSTransitionGroup with convenience functionality
// to fit inside a Router or just with children that are inserted by Routes.
// The transition to use is specified with the 'transition' prop, which is
// a function(from, to) or map[from][to] where from/to are the route path
// components associated with the children entering/leaving.

export class RouteTransition extends React.Component {
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

  getChildKey(child) {
    return child.props.route && child.props.route.path || null /*|| child.props.id || child.props.ref || child.props.name*/;
  }
  getCurrentKey(props) {
    var children = React.Children.toArray(props.children);
    return(
      children.map(c => typeof c === 'object' && c.props.route && c.props.route.path).find(r => r) ||
      null);
  }
  componentWillReceiveProps(nextProps) {
    this.setState({
      oldKeys: React.Children.map(this.props.children, child => (child && (child.key || (child.props.route && child.props.route.path))) || null),
      oldKey: this.getCurrentKey(this.props)
    });
  }
  render() {
    var { children, location } = this.props;
    var from = this.state.oldKey;
    var to = this.getCurrentKey(this.props);
    var transition = determineTransition(from, to, this.getProp('transition'));
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
        {
          React.Children.map(children, child => {
            if (child) {
              var Type = child.type;
              return <Type {...child.props} key={child.props.route && child.props.route.path || child.key || null}/>;
            }
            return null;
          })
        }
      </ReactCSSTransitionGroup>
    );
  }
}

export default RouteTransition;

export { ReactCSSTransitionGroup, ReactTransitionGroup };
