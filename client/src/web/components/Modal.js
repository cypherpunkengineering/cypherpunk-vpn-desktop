import React from 'react';
import ReactDOM from 'react-dom';

export default class Modal extends React.Component {
  showIfNeeded(self) {
    if (!self.open) {
      console.log("showModal");
      self.showModal();
    }
  }
  componentDidMount() {
    var self = ReactDOM.findDOMNode(this);
    self.addEventListener('click', event => {
      var rect = self.getBoundingClientRect();
      if (event.clientY >= rect.top && event.clientY < rect.top + rect.height &&
          event.clientX >= rect.left && event.clientX < rect.left + rect.width) {
        // in bounds; forward if needed
        if (this.props.onClick) {
          this.props.onClick(event);
        }
      } else {
        event.preventDefault();
        if (this.props.closable) {
          self.close();
        }
      }
    });
    self.addEventListener('cancel', event => {
      if (!this.props.closable) {
        event.preventDefault();
      }
    });
    self.addEventListener('close', event => {
      if (this.props.onClose) {
        this.props.onClose();
      }
    });
    self.showModal();
  }
  /*
  componentDidUpdate(prevProps, prevState) {
    var self = ReactDOM.findDOMNode(this);
    showIfNeeded(self);
  }
  */
  render() {
    var props = Object.assign({}, this.props);
    delete props['closable'];
    delete props['onClose'];
    delete props['onClick'];
    delete props['children'];
    // The extra <div> wrapper below is needed as the dialog sits in the top-level
    // layer while modal and can't be clipped, so instead we animate the div inside
    // the dialog for transition effects. 
    return (
      <dialog {...props}>
        <div>
          {this.props.children}
        </div>
      </dialog>
    )
  }
}
Modal.defaultProps = { closable: true };
