window.$ = window.jQuery = require('jquery');

import './assets/css/app.less';
import 'semantic';

import React from 'react';
import ReactDOM from 'react-dom';
import { Router, Route, IndexRoute, IndexRedirect, Redirect, Link, hashHistory as History } from 'react-router';

import SpinningImage from './assets/img/bgring3.png';
import CypherPunkLogo from './assets/img/cp_logo1.png';

import daemon from './daemon.js';

import SettingsScreen from './SettingsScreen.jsx';

daemon.call.ping().then(() => {
  History.push('/main');
});


function humanReadableSize(count) {
  if (count >= 1024 * 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024 / 1024) / 10).toFixed(1) + "T";
  } else if (count >= 1024 * 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024 / 1024) / 10).toFixed(1) + "G";
  } else if (count >= 1024 * 1024) {
    return parseFloat(Math.round(count * 10 / 1024 / 1024) / 10).toFixed(1) + "M";
  } else if (count >= 1024) {
    return parseFloat(Math.round(count * 10 / 1024) / 10).toFixed(1) + "K";
  } else {
    return parseFloat(count).toFixed(0);
  }
}

function getCertificateAuthority() {
  return [
    "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",
    "dsj1LEFcf8DWaj19Dz099BxQgw=="
  ];
}
function getCertificate() {
  return [
    "MIIEwzCCAqugAwIBAgICEAMwDQYJKoZIhvcNAQEFBQAwbjELMAkGA1UEBhMCSlAx",
    "DjAMBgNVBAgMBVRva3lvMQ8wDQYDVQQHDAZNaW5hdG8xDDAKBgNVBAoMA3dpejEb",
    "MBkGA1UECwwSbmV0d29yayBvcGVyYXRpb25zMRMwEQYDVQQDDAp3aXogVlBOIENB",
    "MB4XDTE2MDUxNDA3MjU1N1oXDTI2MDUxMjA3MjU1N1owXjELMAkGA1UEBhMCSlAx",
    "DjAMBgNVBAgMBVRva3lvMQwwCgYDVQQKDAN3aXoxGzAZBgNVBAsMEm5ldHdvcmsg",
    "b3BlcmF0aW9uczEUMBIGA1UEAwwLa2ltQHdpei5iaXowggEiMA0GCSqGSIb3DQEB",
    "AQUAA4IBDwAwggEKAoIBAQDEWicZ0zyR0eZ9T87i4A/Qjff9eAXPRkVaF4cORfpE",
    "cgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUgBkgHq45yEzP4W5TS2XozxhP6lqfZ",
    "Mg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAzs0n8EjtBVH23oHbx3R9EV7nLhknJ",
    "OdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kxAu+iWecCJ8N+IGdNgHQKzNiZyvhK",
    "imDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl9hOWsg4qe9kS1KPzHz9wh9cJZ+6c",
    "SCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpDKUS/8gxjAgMBAAGjezB5MAkGA1Ud",
    "EwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmlj",
    "YXRlMB0GA1UdDgQWBBRvfK5HvBuCcx39T9FWtGYOcLB46TAfBgNVHSMEGDAWgBSv",
    "ozGO00BQe0sdO46u9CukB0fyDTANBgkqhkiG9w0BAQUFAAOCAgEAiVAMfBL8xzj/",
    "O07VxbpDExCGrk5F4T7ecFqrXpq+ALSY0Zh1bQU5yo8Dq7Q5CZxkbf12gCeRja6K",
    "9De+A5xkjZGUS9f6+UEgN344I/WuttAJvGOE5WNnDiXWtG9e0rPmc67jF4DhvL+I",
    "rq8Wvq7tFO2yTblsYI/ZYe4aTYRI6L20TRM/MN+zIm+mnQHLJZknz/j6joX0J7Qb",
    "8ZNq+5ZaelMI1HE7XejZSY0VNKR+qgJcQ3LEi81ixi/RRdzdzGR6b16zZH571b24",
    "Id5pgvp9as3MTruSAmiiAUIVLqAJIRG+Q/zelfwGTe4WEQRZyR3t0lQlFw5ROirb",
    "kjSHq+Wx4TearhOki1QGEMvP9Ao6EGtaADklbGgEGpPrWeI+bx4OzJTBi4Qg8yrj",
    "rE0ASfrhdMTIONnqZh65rAhq2/QsEzg5nLYOSYDltdoz9GxBqyy4J+HIzjywhza2",
    "PT3PJ6gArTE7XAEcXGNdijV4DxHkOS1np9ObPFEYAA+L0ZLQiO1H0I2t6EeiPynP",
    "1pJfPVXMbplvzFrnOEKPaUPdW4n8jbG1bBdhJRprGK5oV/AU5clrRBW3bH3UMrkC",
    "uol9Iro4CpErNVop5ylL3P54ByCojpou/aKiQN3LL2q7EghUQfjVHppYGfRje+DI",
    "y25N8OB8yRP9OAb8rI+k1Zxj8Y8Pcgs="
  ];
}
function getPrivateKey() {
  return [
    "MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDEWicZ0zyR0eZ9",
    "T87i4A/Qjff9eAXPRkVaF4cORfpEcgZbdNqFpXQNWlB/UvMmckL5V6sT7mFB/XUg",
    "BkgHq45yEzP4W5TS2XozxhP6lqfZMg2L1PA2nJqkOFdeLNapIVf8ksP4szs+drAz",
    "s0n8EjtBVH23oHbx3R9EV7nLhknJOdqAf7u+two3iaG9oSq3zasnYc7jCGRpa7kx",
    "Au+iWecCJ8N+IGdNgHQKzNiZyvhKimDfqy/5JxWcS0xSyp6OzIe5XfTXIwEKNptl",
    "9hOWsg4qe9kS1KPzHz9wh9cJZ+6cSCmGBUR0KQi8mOoHb37k04cDJa5WN6UK0kpD",
    "KUS/8gxjAgMBAAECggEBALb0LaTBj5lrlDFFEMeS8Qlpjx3NHNSybGJys7PX/kaS",
    "XFwROJ/4t3bNpV3N46P6KW99gXmTz2mWifDqCWmkL5kZTX5njvccDuJ4+RqwD/uv",
    "yLF3GtA4AVts5/NnIij7WamM8y8jids86hdyQkiukCniWTWlPc9FMyIR/5ulJ9Fn",
    "BUEnByNryY9cVx1CSWWQebVu/9tYJVs5Tn71Rpj0h3j9hOYeI+NROD/j4iN0LDJp",
    "uagg89F2PrhiOjM7mPJQmWbNME3whxgoN29AGo8NbbYZ2VqXeoWz3M/WxlsvP+mT",
    "W3STGxmatuObojhix43tjPn0jSt3yGrxtFE4SNVuP5kCgYEA7N2K2spxlY4NfxGm",
    "vZ0/H0z7qBk8lvFsZf1hKNZBT+lQsmxKpXH/q3dKknfitBTxpZtms81tDDM+77XG",
    "PCfOI/ELXwJG9GFao0PillWH58V0NnrPte8/cfl8Lx4138IagDHStk4PuldiQRWe",
    "F2s124wZZzQrw8/+rf0RLudB9icCgYEA1DbIWxOtASTUb1VBbXo7pLFOyoLYrJD+",
    "NPQWMoMWYQ1w/5Kd3stt9CZouv14ml/fIsyXxueFzO4k6dGxkvQDwLtC4XJWjiwX",
    "n2B/HRZ4hV4xsJ6H0M2A1mZkNUX42aDNUsR8Em6zen8cbxVBTuwY9/RsagMxDZF2",
    "lU59F+GA+WUCgYALyElr8L4Nrm9Fbt9Yd0X4jJ/IENlOuNunhx8aJO5Cx1xYQ8LC",
    "0BTjtp9jAcupIZGTp1NIhmNyQ+pRij0+KMy8RPVH2Jkm9uDHVk0jJUYJZW0OeLV0",
    "W15QkRR4U4xigQlIbzIIF4H4xvgAPM8MYyzequ1okNPMfcAxb3E3YBGL6QKBgBez",
    "EocRWHXTPiI83DS0vOp0nr8BA9+pxan2RHBZsWsfTCpOnnDeOSZWD8YqPojHAi1p",
    "ud2Nx6SOR/MQ5wrpU233u81frojsJas35JpEAyupzFTUL4jDGotXHgPRD6yGR8fh",
    "h5WrZUHd5jgFoKiGt3chheYE+zpvr1WXUWMUXQn9AoGBANkkLXfDDBjcteH5pUwD",
    "zIuLaKXwJp2Iu7Z+Vj7xcb61W/nFmqNpVJNE/QP/i0Hz3+iDiJ+xFkh/a3ceEQTa",
    "/Mt3yRZkX6JBLk8IiPL65KTwf4wkeuXQ+0LhvzyxYXhpc2BQnbFaYkWqF+C2Tt7G",
    "6sn+c32sDV/5PgCJpWUowBLy"
  ];
}



class MainBackground extends React.Component {
  render() {
    return(
      <div id="main-background">
        <div>
          <img class="r1" src={SpinningImage}/>
          <img class="r2" src={SpinningImage}/>
          <img class="r3" src={SpinningImage}/>
          <img class="r4" src={SpinningImage}/>
          <img class="r5" src={SpinningImage}/>
          <img class="r6" src={SpinningImage}/>
          <img class="r7" src={SpinningImage}/>
        </div>
      </div>
    );
  }
}

class Titlebar extends React.Component {
  componentDidMount() {
    $(this.refs.dropdown).dropdown({ action: 'hide' });
  }
  render() {
    return(
      <div id="titlebar" className="ui three item fixed inverted borderless icon menu">
        <Link className="item" to="/account"><i className="info icon"></i></Link>
        <div className="header item">Cypherpunk VPN</div>
        <Link className="item" to="/settings"><i className="setting icon"></i></Link>
      </div>
    );
  }
}

class LoadDimmer extends React.Component {
  render() {
    return(
      <div id="load-screen" class="full screen ui active dimmer" style={{visibility: 'visible', zIndex: 20}}>
        <div class="ui big text loader">Loading</div>
      </div>
    );
  }
}

class ConnectScreen extends React.Component {
  constructor(props) {
    super(props);

    // Listen for OpenVPN state changes (FIXME: API will change later)
    daemon.on('state', (timestamp, state, desc, local, remote) => {
      console.log("state", timestamp, state, desc, local, remote);
      var newState = {
        'CONNECTED': 'connected',
        'EXITING': 'disconnected'
      }[state];
      if (newState)
        this.setState({ connectionState: newState });
    });
    // Subscribe to the traffic counter
    daemon.on('bytecount', (inCount, outCount) => {
      console.log("bytecount", inCount, outCount);
      this.setState({ receivedBytes: inCount, sentBytes: outCount });
    });

    this.handleConnectClick = this.handleConnectClick.bind(this);
    this.handleRegionSelect = this.handleRegionSelect.bind(this);
    this.handleFirewallChange = this.handleFirewallChange.bind(this);
  }
  state = {
    regions: [
      { remote: "208.111.52.1 7133", country: "jp", name: "Tokyo 1, Japan" },
      { remote: "208.111.52.2 7133", country: "jp", name: "Tokyo 2, Japan" },
      { remote: "199.68.252.203 7133", country: "us", name: "Honolulu, HI, USA" },
    ],
    selectedRegion: "208.111.52.1 7133",
    receivedBytes: 0,
    sentBytes: 0,
    connectionState: "disconnected",
  }
  componentDidMount() {
    $(this.refs.regionDropdown).dropdown({
      onChange: this.handleRegionSelect
    });
  }
  shouldComponentUpdate(nextProps, nextState) {
    return true;
  }
  componentWillUpdate(nextProps, nextState) {
    if (this.state.connectionState !== nextState.connectionState) {
      $(document.body).removeClass(this.state.connectionState).addClass(nextState.connectionState);
      if (nextState.connectionState === 'disconnected')
        $('#main-background').removeClass('animating');
      else
        $('#main-background').addClass('animating');
    }
  }
  render() {
    var buttonLabel = {
      'disconnected': "Tap to protect",
      'connecting': "Connecting...",
      'connected': "You are protected",
      'disconnecting': "Disconnecting...",
    }[this.state.connectionState];

    return(
      <div id="connect-screen" class="full screen" style={{visibility: 'visible'}}>
        <Titlebar/>
        <div id="connect-container">
          <i id="connect" class={"ui fitted massive power link icon" + (this.state.connectionState === 'connected' ? " green" : this.state.connectionState == 'disconnected' ? " red" : " orange disabled")} ref="connectButton" onClick={this.handleConnectClick}></i>
        </div>
        <div id="connect-status" ref="connectStatus">{buttonLabel}</div>
        <div id="region-select" class={"ui selection dropdown" + (this.state.connectionState === 'disconnected' ? "" : " disabled")} ref="regionDropdown">
          <input type="hidden" name="region" value={this.state.selectedRegion}/>
          <i class="dropdown icon"></i>
          <div class="default text">Select Region</div>
          <div class="menu">
            { this.state.regions.map(r => <div class="item" data-value={r.remote} key={r.remote}><i class={r.country + " flag"}></i>{r.name}</div>) }
          </div>
        </div>
        <div id="connection-stats" class="ui two column center aligned grid">
          <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.receivedBytes)}</div><div class="label">Received</div></div></div>
          <div class="column"><div class="ui mini statistic"><div class="value">{humanReadableSize(this.state.sentBytes)}</div><div class="label">Sent</div></div></div>
        </div>
        <div class="ui center aligned grid" style={{ marginTop: "30px" }}>
          <div class="ui toggle checkbox">
            <input id="firewall" type="checkbox" onChange={this.handleFirewallChange}/>
            <label for="firewall" style={{ color: "#ddd !important" }}>Enable Firewall</label>
          </div>
        </div>
      </div>
    );
  }
  handleConnectClick() {
    switch (this.state.connectionState) {
      case 'disconnected':
        this.setState({ connectionState: 'connecting' });
        daemon.call.connect({
          proto: "udp",
          //remote: "208.111.52.1 7133",
          //remote: "208.111.52.2 7133",
          //remote: "199.68.252.203 7133",
          remote: this.getSelectedRegion(),
          "redirect-gateway": "def1",
          ca: getCertificateAuthority(),
          cert: getCertificate(),
          key: getPrivateKey(),
        });
        break;
      case 'connecting':
      case 'connected':
        this.setState({ connectionState: 'disconnecting' });
        daemon.post.disconnect();
        break;
      case 'disconnecting':
        break;
    }
  }
  getSelectedRegion() {
    return $(this.refs.regionDropdown).dropdown('get value');
  }
  handleRegionSelect(remote) {
  }
  handleFirewallChange(e) {
    daemon.call.setFirewall({ "mode": e.target.checked ? "on" : "off" });
  }
}

class LoginScreen extends React.Component {
  render() {
    return (
      <div className="ui container cp">
        <h1 className="ui center aligned header"><img className="logo" src={CypherPunkLogo}/></h1>
        <form className="login_screen">
        <div>
          <input defaultValue="Username/Email" />
        </div>
        <div>
          <input defaultValue="Password" />
        </div>
        <Link className="login" to="/connect">Log in</Link>
        <div className="forgot">Forgot password?</div>
        <div className="signup">Sign Up</div>
        </form>
      </div>
    );
  }
}

class AccountScreen extends React.Component  {
  render() {
    return(
      <div className="full screen" style={{visibility: 'visible'}}>
        <div className="ui fluid inverted borderless icon menu">
          <Link className="item" to="/connect"><i className="arrow left icon"></i></Link>
          <div className="header item center aligned">Account</div>
        </div>
        <div className="ui container">
          <div>display username</div>
          <div>display current plan</div>
          <div>upgrade account</div>
          <div>change password</div>
          <div>change email</div>
          <div>help</div>
          <Link className="clicky" to="/">Logout</Link>
        </div>
      </div>
    );
  }
}

class RootContainer extends React.Component {
  render() {
    function WithTitleBar(inner) {
      return [ <Titlebar/>, ]
    }
    return(
      <div class="full screen" style={{visibility: 'visible'}}>
        <MainBackground/>
        <ConnectScreen/>
      </div>
    );
  }
}

class CypherPunkApp extends React.Component {
  render() {
    return(
      <Router history={History}>
        <Route path="/connect" component={RootContainer}></Route>
        <Route path="/account" component={AccountScreen}></Route>
        <Route path="/settings" component={SettingsScreen}></Route>
        <Route path="/" component={LoginScreen}></Route>
        <Route path="*" component={LoginScreen}></Route>
      </Router>
    );
  }
}

ReactDOM.render(<CypherPunkApp />, document.getElementById('root-container'));
