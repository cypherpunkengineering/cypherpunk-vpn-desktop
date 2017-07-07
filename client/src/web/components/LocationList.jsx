import React from 'react';
import ReactDOM from 'react-dom';
import { classList } from '../util';
import daemon, { DaemonAware } from '../daemon';
import { RetinaImage } from './Image';

const CypherPlayIcon = { [1]: require('../assets/img/icon_cypherplay.png'), [2]: require('../assets/img/icon_cypherplay@2x.png') };

const FLAG_PATH = '../assets/img/flags/24/';

export const Flag = ({ country, className = null, ...props }) => {
  country = country.toLowerCase();
  return (<RetinaImage className={classList("flag", className)} src={{ [1]: `${FLAG_PATH}${country}.png`, [2]: `${FLAG_PATH}${country}@2x.png` }} alt=""/>);
}

export const Location = ({ location, className, selected = false, favorite = null, ping = null, hideTag = false, name = null, onClick, ...props } = {}) => {
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
    if (ping.replies) {
      ping = (ping.average * 1000).toFixed(0);
      ping = (ping === "0") ? "<1ms" : (ping + "ms");
      ping = <span className="ping-time">{ping}</span>;
    } else if (ping.timeouts) {
      ping = <span className="ping-time"><i className="warning icon"/></span>;
    } else {
      ping = null;
    }
  }
  return (
    <div className={classList(classes, className)} data-value={location.id} onClick={onClick} {...props}>
      {location.country ? <Flag country={location.country}/> : null}
      <span data-tag={hideTag ? null : tag}>{name || location.name}</span>
      {ping ? <span className="ping-time">{ping}</span> : null}
      {favorite !== null ? <i className="cp-fav icon"></i> : null}
    </div>
  );
}


const Header = ({ name, count = null, ...props }) => <div className="header" data-count={count} {...props}>{name}</div>;
const mapSorter = (mapper) => (a, b) => mapper(a).localeCompare(mapper(b));

export function groupLocationsByRegion(locations, regions, regionOrder, countryNames) {
  // note: javascript objects retain property order
  let result = {};
  regionOrder.forEach(r => {
    result[r] = {};
  });
  // Place all locations into a region/country hierarchy
  Object.forEach(locations, (id, location) => {
    let countries = result[location.region] || (result[location.region] = {});
    let country = countries[location.country] || (countries[location.country] = { country: location.country, locations: [] });
    country.locations.push(location);
  });
  // Sort inside each country and delete any empty regions/countries
  for (let k in result) {
    if (!result.hasOwnProperty(k)) continue;
    let countries = result[k];
    for (let c in countries) {
      if (!countries.hasOwnProperty(c)) continue;
      let country = countries[c];
      if (!country.locations.length) {
        delete countries[c];
      } else {
        country.locations.sort(mapSorter(v => v.name));
      }
    }
    result[k] = Object.values(countries).sort(mapSorter(v => countryNames[v.country]));
    if (!result[k].length) {
      delete result[k];
    }
  }
  return result;
}


const CYPHERPLAY = "CypherPlay\u2122";

export const CypherPlayItem = ({ selected = false, disabled = false, hideTag = false, ...props }) => {
  let title = hideTag ? CYPHERPLAY : "Fastest Location";
  let suffix = hideTag ? null : <span><span>with</span> {CYPHERPLAY}</span>;
  return (
    <div
      key="cypherplay"
      className={classList("cypherplay", { "selected": selected, "disabled": disabled, "taggable" : !hideTag })}
      {...props}
      >
      <RetinaImage src={CypherPlayIcon}/>{title}{suffix}
    </div>
  );
};

export const FastestItem = ({ location, ...props }) =>
  <Location className="fastest" location={location} name="Fastest Location" hideTag={true} {...props}/>;

export class LocationList extends DaemonAware(React.Component) {
  static defaultProps = {
    onClick: function(location) {},
    onHover: function(location) {},
    onBack: function() {},
    open: true,
    selected: null,
  }
  constructor(props) {
    super(props);
    this.daemonSubscribeState({
      config: { locations: true, regions: true, regionOrder: true, regionNames: true, countryNames: true },
      settings: { lastConnected: true },
      state: { pingStats: true },
    });
    this.state.fastest = this.recalculateFastestServer(this.state);
  }
  componentWillReceiveProps(props) {
    if (props.open && !this.props.open) {
      this.refs.list.scrollTop = 0;
    }
  }
  daemonDataChanged(state) {
    if (state.pingStats !== this.state.pingStats || state.locations !== this.state.locations) {
      return { fastest: this.recalculateFastestServer(state) };
    }
  }
  recalculateFastestServer(state) {
    const ping = l => (state.pingStats[l] && state.pingStats[l].replies > 0 && state.pingStats[l].average || 999);
    const last = l => (state.lastConnected[l] || 0);
    let fastest = Object.values(this.state.locations)
      .filter(l => l.enabled && !l.disabled && l.region !== 'DEV')
      .map(l => l.id)
      .sort((a, b) => (ping(a) - ping(b) || last(b) - last(a)));
    return fastest.length ? fastest[0] : null;
  }
  render() {
    let grouping = groupLocationsByRegion(this.state.locations, this.state.regions, this.state.regionOrder, this.state.countryNames);
    return (
      <div className={classList("location-list", { "hidden": !this.props.open })}>
        <div className="header">
          { (this.props.selected) && <div className="title">Connected to</div> }
          { (this.props.selected) && (this.props.selected === 'cypherplay' ? <CypherPlayItem hideTag={true}/> : <Location location={this.state.locations[this.props.selected]} hideTag={true}/>) }
          { (this.props.selected) && <div className="title">Switch to</div> }
          { (!this.props.selected) && <div className="title">Connect to</div> }
        </div>
        <div ref="list" className="list" onMouseLeave={() => { this.props.onHover(this.props.selected || null); }}>
          <CypherPlayItem key="cypherplay" disabled={!this.state.fastest} selected={this.props.selected === 'cypherplay'} onMouseEnter={() => this.props.onHover('cypherplay')} onClick={this.state.fastest ? () => this.props.onClick('cypherplay:' + this.state.fastest) : null}/>
          <FastestItem key="fastest" disabled={!this.state.fastest} location={this.state.fastest ? this.state.locations[this.state.fastest] : null} selected={false} onMouseEnter={this.state.fastest ? () => this.props.onHover(this.state.fastest) : null} onClick={this.state.fastest ? () => this.props.onClick(this.state.fastest) : null}/>
          {
            Array.flatten(Object.mapToArray(grouping, (region, countries) => 
              [ <Header key={`header-${region}`} name={this.state.regionNames[region]}/> ]
                .concat(Array.flatten(countries.map(c => c.locations.map(l =>
                  <Location
                    key={`location-${l.id}`}
                    location={l}
                    selected={this.props.selected === l.id}
                    ping={this.state.pingStats ? this.state.pingStats[l.id] : null}
                    onMouseEnter={() => this.props.onHover(l.id)}
                    onClick={() => !l.disabled && this.props.onClick(l.id)}
                  />
                ))))
            ))
          }
        </div>
        <div className="footer">
          <span className="back" onClick={() => this.props.onBack()}><i className="chevron left icon"/>Back</span>
        </div>
      </div>
    );
  }
}

export default LocationList;
