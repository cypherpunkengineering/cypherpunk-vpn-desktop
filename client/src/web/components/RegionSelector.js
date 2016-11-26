import React from 'react';
import ReactDOM from 'react-dom';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER } from '../util';
import daemon, { DaemonAware } from '../daemon';


export default class RegionSelector extends DaemonAware(React.Component) {

  state = {
    locations: daemon.config.locations,
    regions: daemon.config.regions,
    selected: daemon.settings.location,
    favorites: {},
    open: false,
  }

  daemonConfigChanged(config) {
    if (config.locations) this.setState({ locations: config.locations });
    if (config.regions) this.setState({ regions: config.regions });
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('location')) this.setState({ selected: settings.location });
  }

  open() {
    //this.$.addClass('open');
    this.setState({ open: true });
  }
  close() {
    //this.$.removeClass('open');
    //this.$.children('.list').scrollTop(0);
    this.setState({ open: false });
  }
  ensureVisible(item) {
    if (typeof item === 'string') {
      item = this.$item(item);
    } else {
      item = $(item);
    }
    if (!item || !item[0]) {
      return;
    }
    var itemHeight = item.outerHeight();
    var itemTop = item[0].offsetTop;
    var list = this.$.children('.list');
    var listHeight = list.height();
    var listScroll = list.scrollTop();
    if (itemTop + itemHeight > listScroll + listHeight) {
      list.scrollTop(itemTop + itemHeight - listHeight);
    } else if (itemTop + itemHeight <= listHeight) {
      list.scrollTop(0);
    } else if (itemTop < listScroll) {
      list.scrollTop(itemTop);
    }
  }
  scrollIfNeeded() {
    if (this.state.selected) {
      this.ensureVisible(this.state.selected);
    } else {
      this.$list.scrollTop(0);
    }
  }

  onLocationClick(location) {
    daemon.call.applySettings({ location: location })
      .then(() => {
        if (daemon.state.needsReconnect) {
          daemon.post.connect();
        }
      });
    this.setState({ selected: location });
    this.close();
  }
  onLocationFavoriteClick(location) {
    this.setState({ favorites: Object.assign({}, this.state.favorites, { [location]: !this.state.favorites[location] }) });
  }
  onKeyDown(event) {
    switch (event.key) {
      case "ArrowUp":
      case "ArrowDown":
        // Leave keyboard navigation for later
    }
  }

  makeLocation(location, clickable) {
    var classes = [ 'region' ];
    if (location.disabled) {
      classes.push('disabled');
    }
    if (this.state.selected === location.id) {
      classes.push('selected');
    }
    if (this.state.favorites[location.id]) {
      classes.push('favorite');
    }
    var onclick = event => {
      var value = event.currentTarget.getAttribute('data-value');
      if (event.target.className.indexOf('cp-fav') != -1) {
        this.onLocationFavoriteClick(value);
      } else if (event.currentTarget.className.indexOf('disabled') == -1) {
        this.onLocationClick(value);
      }
    };
    return(
      <div className={classes.join(' ')} data-value={clickable ? location.id : null} key={clickable ? location.id : null} onClick={clickable ? onclick : null}>
        {/* <i className={location.country.toLowerCase() + " flag"}></i> */}
        <img className={location.country.toLowerCase() + " flag"} src={'../assets/img/flags-24/' + location.country.toLowerCase() + '.png'}  alt=""/>
        <span>{location.name}</span><i className="cp-fav icon"></i>
      </div>
    );
  }

  makeRegionList(regions, locations) {
    return Array.flatten(
      REGION_GROUP_ORDER.map(g => ({
        name: REGION_GROUP_NAMES[g],
        locations: Object.mapToArray(regions[g], (country, locs) => locs.map(l => locations[l]).map(l => this.makeLocation(l, true))).filter(l => l && l.length > 0)
      })).filter(r => r.locations && r.locations.length > 0).map(r => [ <div key={"region-" + r.name} className="header">{r.name}</div> ].concat(r.locations))
    );
  }

  get dom() {
    return ReactDOM.findDOMNode(this);
  }
  get $() {
    return $(this.dom);
  }
  get $list() {
    return this.$.children('.list');
  }
  $item(location) {
    return this.$list.children(`[data-value="${location}"]`);
  }

  componentDidMount() {
    super.componentDidMount();
    this.scrollIfNeeded();
  }
  componentDidUpdate(prevProps, prevState) {
    if (this.state.open != prevState.open || this.state.selected != prevState.selected) {
      this.scrollIfNeeded();
    }
    if (this.state.open != prevState.open) {
      this.onTransitionStart();
    }
  }

  onTransitionStart(event) {
    var self = this;
    var frame = {
      callback: function(now) { frame.id = false; self.onTransitionFrame(now); if (frame.id === false) frame.next(); },
      next: function() { this.id = window.requestAnimationFrame(this.callback); },
      cancel: function() { if (this.id) window.cancelAnimationFrame(this.id); delete this.id; delete self.frame; }
    };
    this.frame = frame;
    this.frame.next();
  }
  onTransitionFrame(now) {
    this.scrollIfNeeded();
  }
  onTransitionEnd(event) {
    if (this.frame) this.frame.cancel();
    this.scrollIfNeeded();
  }

  render() {
    var classes = [ 'region-selector2' ];
    if (this.state.open) {
      classes.push('open');
    }
    // Leave keyboard navigation for later
    //tabIndex="0"
    //onFocus={event => { this.open(); }}
    //onBlur={event => { if (document.activeElement !== this.dom) this.close(); }}
    return(
      <div className={classes.join(' ')}
        onKeyDown={event => this.onKeyDown(event)}
        onTransitionEnd={event => this.onTransitionEnd(event)}
        >
        <div className="bar" onClick={() => this.state.open ? this.close() : this.open()}>
          { (this.state.selected && this.state.locations[this.state.selected]) ? this.makeLocation(this.state.locations[this.state.selected], false) : "Select Region" }
        </div>
        <div className="list">
          { this.makeRegionList(this.state.regions, this.state.locations) }
        </div>
      </div>
    );
  }
}
