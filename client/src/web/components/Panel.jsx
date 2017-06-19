import React from 'react';
import ReactDOM from 'react-dom';
import { TransitionGroup } from './Transition';
import { classList } from '../util';

export function defaultTransition(diff) {
  return 'reveal';
}

export const PanelContainer = ({ className, ...props }) => <div className={classList("transition-container", "panel", className)} {...props}/>;

export const Panel = ({ className, ...props }) => <TransitionGroup className={className} component={PanelContainer} transition={defaultTransition} {...props}/>;

export const Subpanel = ({ className, direction = "right", ...props }) => <Panel className={classList("subpanel", direction)} {...props}/>;

export class PanelContent extends React.Component {
  render() {
    return (
      <div {...this.props} className={classList("panel-content", this.props.className)}/>
    );
  }
}

export const PanelOverlay = ({ className, ...props }) => <div className={classList("overlay", className)} {...props}/>;
