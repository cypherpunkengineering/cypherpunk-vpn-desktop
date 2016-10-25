import React from 'react';
import ReactDOM from 'react-dom';
import { REGION_GROUP_NAMES, REGION_GROUP_ORDER } from '../util';
import daemon, { DaemonAware } from '../daemon';


export default class RegionSelector extends DaemonAware(React.Component) {

  state = {
    servers: daemon.config.servers,
    regions: daemon.config.regions,
    selected: daemon.settings.server,
    favorites: {},
    open: false,
  }

  daemonConfigChanged(config) {
    if (config.servers) this.setState({ servers: config.servers });
    if (config.regions) this.setState({ regions: config.regions });
  }
  daemonSettingsChanged(settings) {
    if (settings.server) this.setState({ selected: settings.server });
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
    var itemHeight = item.outerHeight();
    var itemTop = item[0].offsetTop;
    console.log(item[0].offsetParent);
    var list = this.$.children('.list');
    var listHeight = list.height();
    var listScroll = list.scrollTop();
    console.log(itemTop, itemHeight, listScroll, listHeight);
    if (itemTop + itemHeight > listScroll + listHeight) {
      list.scrollTop(itemTop + itemHeight - listHeight);
    } else if (itemTop + itemHeight <= listHeight) {
      list.scrollTop(0);
    } else if (itemTop < listScroll) {
      list.scrollTop(itemTop);
    }
  }
  scrollIfNeeded() {
    console.log("scrollIfNeeded", this.state.open, this.state.selected);
    if (!this.state.open) {
      this.$list.scrollTop(0);
    } else if (this.state.selected) {
      this.ensureVisible(this.state.selected);
    }
  }

  onServerClick(server) {
    daemon.post.applySettings({ server: server });
    this.setState({ selected: server })
    this.close();
  }
  onServerFavoriteClick(server) {
    console.log("favorite", server);
    this.setState({ favorites: Object.assign({}, this.state.favorites, { [server]: !this.state.favorites[server] }) });
  }
  onKeyDown(event) {
    console.log(event.key);
    switch (event.key) {
      case "ArrowUp":
      case "ArrowDown":
        // Leave keyboard navigation for later
    }
  }

  makeServer(server, clickable) {
    var classes = [ 'region' ];
    if (server.ovDefault == "255.255.255.255") {
      classes.push('disabled');
    }
    if (this.state.selected === server.id) {
      classes.push('selected');
    }
    if (this.state.favorites[server.id]) {
      classes.push('favorite');
    }
    var onclick = event => {
      console.dir(event);
      var value = event.currentTarget.getAttribute('data-value');
      if (event.target.className.indexOf('cp-fav') != -1) {
        this.onServerFavoriteClick(value);
      } else if (event.currentTarget.className.indexOf('disabled') == -1) {
        this.onServerClick(value);
      }
    };
    return(
      <div className={classes.join(' ')} data-value={clickable ? server.id : null} key={clickable ? server.id : null} onClick={clickable ? onclick : null}>
        <i className={server.country.toLowerCase() + " flag"}></i><span>{server.regionName}</span><i className="cp-fav icon"></i>
      </div>
    );
  }

  makeRegionList(regions, servers) {
    return Array.flatten(
      REGION_GROUP_ORDER.map(g => ({
        name: REGION_GROUP_NAMES[g],
        locations: Object.mapToArray(regions[g], (country, locations) => locations.map(s => servers[s]).map(s => this.makeServer(s, true))).filter(l => l && l.length > 0)
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
  $item(server) {
    return this.$list.children(`[data-value="${server}"]`);
  }

  componentDidMount() {
    super.componentDidMount();
    this.scrollIfNeeded();
  }
  componentDidUpdate(prevProps, prevState) {
    if (this.state.open != prevState.open || this.state.selected != prevState.selected) {
      this.scrollIfNeeded();
    }
  }

  render() {
    console.log("rerendering");
    console.dir(this.state.favorites);
    var classes = [ 'region-selector2' ];
    if (this.state.open) {
      classes.push('open');
    }
    // Leave keyboard navigation for later
    //tabIndex="0"
    //onFocus={event => { console.dir(event.target, event.relatedTarget); this.open(); }}
    //onBlur={event => { if (document.activeElement !== this.dom) this.close(); }}
    return(
      <div className={classes.join(' ')}
        onKeyDown={event => this.onKeyDown(event)}
        >
        <div className="bar" onClick={() => this.state.open ? this.close() : this.open()}>
          { (this.state.selected && this.state.servers[this.state.selected]) ? this.makeServer(this.state.servers[this.state.selected], false) : "Select Region" }
        </div>
        <div className="list">
          { this.makeRegionList(this.state.regions, this.state.servers) }
        </div>
      </div>
    );
  }
}
