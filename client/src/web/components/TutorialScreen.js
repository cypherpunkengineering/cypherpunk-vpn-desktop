import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { RouteTransition, TransitionGroup } from './Transition';
import { classList } from '../util';

const PAGES = [
  {
    x: 350/2,
    y: 180,
    size: 0,
    content:
      <div key="page-0" className="welcome">
        <h3>Welcome!</h3>
        Let's take a minute to introduce you to the basics of using the Cypherpunk Privacy app!
      </div>
  },
  {
    x: 350/2,
    y: 180,
    size: 150,
    content:
      <div key="page-1" className="desc" style={{ left: '20px', right: '20px', top: '280px' }}>
        Connect to the Cypherpunk network by clicking here.
      </div>
  },
  {
    x: 25,
    y: 65,
    size: 100,
    content:
      <div key="page-2" className="desc" style={{ left: '20px', right: '20px', top: '280px' }}>
        Access your account details here.
      </div>
  },
  {
    x: 325,
    y: 65,
    size: 100,
    content:
      <div key="page-3" className="desc" style={{ left: '20px', right: '20px', top: '280px' }}>
        Configure settings here.
      </div>
  },
];

export default class TutorialOverlay extends React.Component {
  componentDidMount() {
    this.dom.showModal();
  }
  onClick() {
    if (PAGES[+this.props.params.page + 1]) {
      History.push('/tutorial/' + (+this.props.params.page + 1));
    } else {
      History.push('/main');
    }
  }
  render() {
    let { x = null, y = null, size = null, content = null } = PAGES[this.props.params.page] || {};

    return (
      <dialog className={classList("tutorial-overlay", `page-${this.props.params.page}`)} onClick={() => this.onClick()}>
        <div key="hole" className="hole" style={{ left: (x - size / 2) + 'px', top: (y - size / 2) + 'px', width: size + 'px', height: size + 'px' }}/>
        <TransitionGroup key="transition" transition="tutorial">
          {content}
          {this.props.params.page < PAGES.length - 1 ? <Link key="skip" to="/main" class="skip" tabIndex="0">Skip tutorial</Link> : null}
        </TransitionGroup>
      </dialog>
    );
  }
}
