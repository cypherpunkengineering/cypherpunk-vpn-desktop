import React from 'react';

// The Dragbar is a minimal component that sits at the top of the frameless
// window and provides space for the OS controls and acts as a draggable
// region to be able to move the window. On OS X, the minimize/close buttons
// are automatically provided, whereas we must draw them ourselves on Windows.

export default class Dragbar extends React.Component {
  constructor(props) {
    super(props);
  }
  render() {
    var className = "cp dragbar";
    var styles={};
    if (this.props.className) {
      className += " " + this.props.className;
    }
    if (this.props.height) {
      styles.height = this.props.height;
    }
    return(
      <div id="dragbar" className={className} style={styles}>
      </div>
    );
  }
}
