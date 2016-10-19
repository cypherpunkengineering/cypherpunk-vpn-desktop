import React from 'react';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, createMemoryHistory, hashHistory as History } from 'react-router';

export default class Dragbar extends React.Component {
  constructor(props) {
    super(props);
    this._handler= this._handler.bind(this);
  }

  _handler(props) {
    // handler scope doesn't know what this is unless you call bind
    console.log(props);
  }

  componentDidMount() {
    $(this.refs.dropdown).dropdown({ action: 'hide' });
  }
  render() {
    return(
      <div id="dragbar" className="">
      </div>
    );
  }
}
