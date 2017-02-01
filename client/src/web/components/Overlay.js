import React from 'react';
import ReactDOM from 'react-dom';

const overlayElements = {};
const overlayStack = [];

// Magic container (or base class) for elements that need to always be in
// the topmost Modal (<dialog>) element.
export class Overlay extends React.Component {
  componentWillReceiveProps(props) {
    if (props.name !== this.props.name) {
      delete overlayElements[this.props.name];
    }
  }
  componentWillUnmount() {
    delete overlayElements[this.props.name];
  }
  render() {
    overlayElements[this.props.name] = React.Children.only(this.props.children);
    return null;
  }
}

export class OverlayContainer extends React.Component {
  componentWillMount() {
    // First time mounting
    this.stackIndex = overlayStack.length;
    overlayStack.push(this);
  }
  componentWillUnmount() {
    if (this.stackIndex !== overlayStack.length - 1) {
      console.error("React unmount order mismatch");
    } else {
      overlayStack.pop();
      this.stackIndex = null;
      for (var i = overlayStack.length - 1; i >= 0; i--) {
        if (true || overlayStack[i].isMounted()) {
          overlayStack[i].forceUpdate();
          break;
        }
      }
    }
  }
  render() {
    if (this.stackIndex === null) {
      console.error("React mount order mismatch");
    } else if (this.stackIndex === overlayStack.length - 1) {
      return <div className="overlay-container">{Object.mapToArray(overlayElements, (k, v) => React.cloneElement(v, { key: k }))}</div>;
    }
    return null;
  }
}

export default Overlay;
