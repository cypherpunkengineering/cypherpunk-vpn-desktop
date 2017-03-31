import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { RouteTransition, TransitionGroup } from './Transition';
import { classList } from '../util';
import analytics from '../analytics';

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
    size: 170,
    content:
      <div key="page-1" className="desc top center" style={{ left: '60px', right: '60px', top: '280px' }}>
        Connect to the Cypherpunk network by clicking here.
      </div>
  },
  {
    x: 25,
    y: 65,
    size: 100,
    content:
      <div key="page-2" className="desc top left" style={{ left: '20px', top: '125px' }}>
        Access your account details here.
      </div>
  },
  {
    x: 325,
    y: 65,
    size: 100,
    content:
      <div key="page-3" className="desc top right" style={{ right: '20px', top: '125px' }}>
        Configure settings here.
      </div>
  },
  {
    x: 50,
    y: 430,
    size: 150,
    content:
      <div key="page-4" className="desc bottom left" style={{ left: '20px', bottom: '185px', width: '270px' }}>
        Connect with CypherPlay to automatically select the best route to your favorite content!
      </div>
  },
];

export default class TutorialOverlay extends React.Component {
  componentDidMount() {
    this.dom.showModal();
    analytics.event('Tutorial', 'begin');
  }
  onClick(event) {
    switch (event.target.className) {
      //case 'hole':
      //case 'welcome':
      //case 'desc':
      //case 'next':
      default:
        if (PAGES[+this.props.params.page + 1]) {
          History.push('/tutorial/' + (+this.props.params.page + 1));
        } else {
          analytics.event('Tutorial', 'finish');
          History.push('/main');
        }
        break;
      case 'dragbar': break;
      case 'prev': break;
      case 'skip': break;
    }
  }
  render() {
    let { x = null, y = null, size = null, content = null } = PAGES[this.props.params.page] || {};

    return (
      <dialog className={classList("tutorial-overlay", `page-${this.props.params.page}`)} onClick={e => this.onClick(e)}>
        <div key="hole" className="hole" style={{ left: (x - size / 2) + 'px', top: (y - size / 2) + 'px', width: size + 'px', height: size + 'px' }}/>
        <TransitionGroup key="transition" transition="tutorial">
          {content}
          {/*this.props.params.page > 0 ? <div key="prev" className="prev"/> : null*/}
          {this.props.params.page < PAGES.length ? <div key="next" className="next"/> : null}
          {this.props.params.page < PAGES.length - 1 ? <Link key="skip" to="/main" class="skip" tabIndex="0" onClick={() => analytics.event('Tutorial', 'skip')}>Skip tutorial</Link> : null}
        </TransitionGroup>
        <div className="dragbar"/>
      </dialog>
    );
  }
}
