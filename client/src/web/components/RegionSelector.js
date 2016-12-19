import React from 'react';
import ReactDOM from 'react-dom';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER, COUNTRY_NAMES } from '../util';
import daemon, { DaemonAware } from '../daemon';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');


export default class RegionSelector extends DaemonAware(React.Component) {

  state = {
    locations: daemon.config.locations,
    regions: daemon.config.regions,
    selected: daemon.settings.location,
    favorites: Array.toDict(daemon.settings.favorites, f => f, f => true),
    recent: daemon.settings.recent,
    open: false,
  }

  daemonConfigChanged(config) {
    if (config.locations) this.setState({ locations: config.locations });
    if (config.regions) this.setState({ regions: config.regions });
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('location')) this.setState({ selected: settings.location });
    if (settings.hasOwnProperty('favorites')) this.setState({ favorites: Array.toDict(settings.favorites, f => f, f => true) });
    if (settings.hasOwnProperty('recent')) this.setState({ recent: settings.recent });
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
        if (daemon.state.state === 'DISCONNECTED' || daemon.state.needsReconnect) {
          daemon.post.connect();
        }
      });
    this.setState({ selected: location });
    this.close();
  }
  onLocationFavoriteClick(location) {
    var favorites = Object.assign({}, this.state.favorites);
    if (this.state.favorites[location]) {
      delete favorites[location];
    } else {
      favorites[location] = true;
    }
    this.setState({ favorites: favorites });
    daemon.post.applySettings({ favorites: Object.keys(favorites).filter(f => favorites[f]) });
  }
  onKeyDown(event) {
    switch (event.key) {
      case "ArrowUp":
      case "ArrowDown":
        // Leave keyboard navigation for later
    }
  }

  makeLocationHeader(id, name) {
    return <div key={id} className="header">{name}</div>;
  }
  makeLocation(location, type = 'location') {
    const clickable = type !== 'header';
    var classes = [ 'region' ];
    var tag = null;
    if (location.disabled) {
      classes.push('disabled');
      tag = 'UNAVAILABLE';
    }
    switch (location.level) {
      case 'free':
        if (!location.disabled) {
          classes.push('free');
          tag = 'FREE';
        }
        break;
      case 'premium':
        if (!location.authorized || !location.disabled) {
          classes.push('premium');
          tag = 'PREMIUM';
        }
        break;
      case 'developer':
        classes.push('developer');
        tag = 'DEV';
        break;
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
    var flag = (dpi = '') => `../assets/img/flags/24/${location.country.toLowerCase()}${dpi}.png`;
    return(
      <div className={classes.join(' ')} data-value={clickable ? location.id : null} key={type + '-' + location.id} onClick={clickable ? onclick : null}>
        {/* <i className={location.country.toLowerCase() + " flag"}></i> */}
        <img className="flag" src={flag()} srcSet={`${flag()} 1x, ${flag('@2x')} 2x`} alt=""/>
        <span data-tag={tag}>{location.name}</span><i className="cp-fav icon"></i>
      </div>
    );
  }

  makeRegionList(regions, locations) {
    var items = Array.flatten(
      REGION_GROUP_ORDER.map(g => ({
        id: g,
        name: REGION_GROUP_NAMES[g],
        locations: Array.flatten(Object.mapToArray(regions[g], (c,l) => [c,l]).sort((a, b) => COUNTRY_NAMES[a[0]].localeCompare(COUNTRY_NAMES[b[0]])).map(([country, locs]) => locs.map(l => locations[l]).sort((a, b) => a.name.localeCompare(b.name)).map(l => this.makeLocation(l, 'location'))).filter(l => l && l.length > 0))
      })).filter(r => r.locations && r.locations.length > 0).map(r => [ this.makeLocationHeader('region-' + r.id.toLowerCase(), r.name) ].concat(r.locations))
    );
    var recent = this.state.recent.filter(r => !this.state.favorites[r] && locations[r]);
    if (recent.length > 0) {
      // Prepend recent list
      items = [ this.makeLocationHeader('recent', "Recent") ].concat(recent.map(l => this.makeLocation(locations[l], 'recent')), items);
    }
    var favorites = Object.keys(this.state.favorites).filter(f => this.state.favorites[f] && locations[f]);
    if (favorites.length > 0) {
      // Prepend favorites list
      items = [ this.makeLocationHeader('favorites', "Favorites") ].concat(favorites.sort((a, b) => locations[a].name.localeCompare(locations[b].name)).map(l => this.makeLocation(locations[l], 'favorite')), items);
    }
    return items;
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
          { (this.state.selected && this.state.locations[this.state.selected]) ? this.makeLocation(this.state.locations[this.state.selected], 'header') : "Select Region" }
        </div>
        <ReactCSSTransitionGroup component="div" className="list" transitionName="fadeIn" transitionEnterTimeout={350} transitionLeaveTimeout={350}>
          { this.makeRegionList(this.state.regions, this.state.locations) }
        </ReactCSSTransitionGroup>
      </div>
    );
  }
}
