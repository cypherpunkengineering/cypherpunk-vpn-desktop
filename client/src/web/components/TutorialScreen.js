import React from 'react';
import ReactDOM from 'react-dom';
import { RouteTransition, TransitionGroup } from './Transition';
import { classList } from '../util';

const PAGES = {
  1: {
    x: 350/2,
    y: 180,
    size: 150,
    style: { left: '20px', right: '20px', top: '280px' },
    text: "Connect to the Cypherpunk network by clicking here.",
  },
  2: {
    x: 25,
    y: 65,
    size: 100,
    style: { left: '20px', top: '130px' },
    text: "Access your account here.",
  },
  3: {
    x: 325,
    y: 65,
    size: 100,
    style: { right: '20px', top: '130px' },
    text: "Configure settings here.",
  },
};

export default class TutorialOverlay extends React.Component {
  componentDidMount() {
    this.dom.showModal();
  }
  onClick() {
    if (PAGES[+this.props.params.page + 1]) {
      History.push('/tutorial/' + (+this.props.params.page + 1));
    }
  }
  render() {
    let { x = null, y = null, size = null, style = {}, text = null } = PAGES[this.props.params.page] || {};

    return (
      <dialog className="tutorial-overlay" onClick={() => this.onClick()}>
        <div key="hole" className="hole" style={{ left: (x - size / 2) + 'px', top: (y - size / 2) + 'px', width: size + 'px', height: size + 'px' }}/>
        <TransitionGroup key="transition" transition="tutorial">
          <div key={`page-${this.props.params.page}`} className="desc" style={style}>
            {text}
          </div>
        </TransitionGroup>
      </dialog>
    );
  }
}
