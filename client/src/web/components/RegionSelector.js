import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';
import daemon, { DaemonAware } from '../daemon';

// Always use this stub to import standard React addons, as we will either use
// their node module (development) or dig them out of react.min.js (production).
function reactAddon(module, name) {
  return module.addons ? module.addons[name] : module;
}

const ReactCSSTransitionGroup = reactAddon(require('react-addons-css-transition-group'), 'CSSTransitionGroup');


export const Location = ({ location, className, selected = false, favorite = null, ping = null, hideTag = false, onClick, ...props } = {}) => {
  if (!location) return null;
  let classes = [ 'location' ];
  let tag = null;
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
  if (selected) {
    classes.push('selected');
  }
  if (favorite) {
    classes.push('favorite');
  }
  if (ping) {
    if (!ping.replies) return null;
    ping = (ping.average * 1000).toFixed(0);
    ping = (ping === "0") ? "<1ms" : (ping + "ms");
  }
  var flag = (dpi = '') => `../assets/img/flags/24/${location.country.toLowerCase()}${dpi}.png`;
  return (
    <div className={classList(classes, className)} data-value={location.id} onClick={onClick} {...props}>
      {location.country ? <img className="flag" src={flag()} srcSet={`${flag()} 1x, ${flag('@2x')} 2x`} alt=""/> : null}
      <span data-tag={hideTag ? null : tag}>{location.name}</span>
      {ping ? <span className="ping-time">{ping}</span> : null}
      {favorite !== null ? <i className="cp-fav icon"></i> : null}
    </div>
  );
}




export default class RegionSelector extends DaemonAware(React.Component) {

  state = {
    locations: daemon.config.locations,
    regions: daemon.config.regions,
    countryNames: daemon.config.countryNames,
    regionNames: daemon.config.regionNames,
    regionOrder: daemon.config.regionOrder,
    selected: daemon.settings.location,
    favorites: Array.toDict(daemon.settings.favorites, f => f, f => true),
    recent: daemon.settings.recent,
    pingStats: daemon.state.pingStats,
    open: false,
  }

  daemonConfigChanged(config) {
    if (config.locations) this.setState({ locations: config.locations });
    if (config.regions) this.setState({ regions: config.regions });
    if (config.countryNames) this.setState({ countryNames: config.countryNames });
    if (config.regionNames) this.setState({ regionNames: config.regionNames });
    if (config.regionOrder) this.setState({ regionOrder: config.regionOrder });
  }
  daemonSettingsChanged(settings) {
    if (settings.hasOwnProperty('location')) this.setState({ selected: settings.location });
    if (settings.hasOwnProperty('favorites')) this.setState({ favorites: Array.toDict(settings.favorites, f => f, f => true) });
    if (settings.hasOwnProperty('recent')) this.setState({ recent: settings.recent });
  }
  daemonStateChanged(state) {
    if (state.hasOwnProperty('pingStats')) this.setState({ pingStats: daemon.state.pingStats });
  }

  open() {
    //this.$dom.addClass('open');
    this.setState({ open: true });
  }
  close() {
    //this.$dom.removeClass('open');
    //this.$('.list').scrollTop(0);
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
    var list = this.$('.list');
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
    const key = type + '-' + location.id;
    const ping = this.state.pingStats && this.state.pingStats[location.id];
    var onclick = event => {
      var value = event.currentTarget.getAttribute('data-value');
      if (event.target.className.indexOf('cp-fav') != -1) {
        this.onLocationFavoriteClick(value);
      } else if (event.currentTarget.className.indexOf('disabled') == -1) {
        this.onLocationClick(value);
      }
    };
    if (clickable)
      return <Location location={location} key={key} className="region" onClick={onclick} selected={this.state.selected === location.id} favorite={!!this.state.favorites[location.id]} ping={ping}/>;
    else
      return <Location location={location} key={key} className="region"/>;
  }

  makeRegionList(regions, locations) {
    var items = Array.flatten(
      this.state.regionOrder.map(g => ({
        id: g,
        name: this.state.regionNames[g],
        locations: Array.flatten(Object.mapToArray(regions[g], (c,l) => [c,l]).sort((a, b) => this.state.countryNames[a[0]].localeCompare(this.state.countryNames[b[0]])).map(([country, locs]) => locs.map(l => locations[l]).sort((a, b) => a.name.localeCompare(b.name)).map(l => this.makeLocation(l, 'location'))).filter(l => l && l.length > 0))
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

  get $list() {
    return this.$('.list');
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
