import React from 'react';

export default class GeneralAdvanced extends React.Component  {
  constructor(props) {
    super(props);
    this.changeTab = this.changeTab.bind(this);
    const tabs = ['General', 'Advanced'];
    this.state = {
      class1: 'show settn', class2: 'hide settn'
    }
  }

  changeTab(e) {
    if (e.target.innerText == "General"){
      this.setState({class1: 'show settn', class2: 'hide settn'})
    } else {
      this.setState({class1: 'hide settn', class2: 'show settn'})
    };
  }

  render() {
    return(
      <div>
        <Tabs changeTab={this.changeTab} tabs={['General', 'Advanced']} />
        <div className={this.state.class1}>

        <ul>
          <li>start application on login</li>
          <li>connect on startup</li>
<li>show desktop notifications</li>
<li>show dock icon, show tray icon, show both dock and tray icons</li>
<li>ip address original</li>
<li>ip address new</li>
</ul>
        </div>
        <div className={this.state.class2}>
        <ul>
        <li><h3>Network Settings</h3></li>
        <li>protocol: OpenVPN UDP, OpenVPN TCP, IKEv1, IKEv2, Cisco IPsec</li>
        <li>remote port: (auto, 1198, 8080, 9201, 53)</li>
        <li>local port: (blank text box)</li>
        <li>firewall settings slider: (off) ---- (auto) ---- (always on)</li>
        <li><h3>More info</h3></li>
        <li> use small packets</li>
        <li>the steve setting</li>
        <li>request port forwarding</li>
        <li>ipv6 leak protection</li>
        <li>dns leak protection</li>
        <li>encryption: (max speed / none) ---- (default / medium) ---- (max privacy / strong) ---- (max freedom / strong + openvpn anti-censorship tweak)</li>
        <li>encryption (display only)</li>
        <li>authentication (display only)</li>
        <li>handshake (display only)</li>
        </ul>

        </div>
      </div>
    );
  }
}

class Tabs extends React.Component {
  constructor(props) {
    super(props);
  }
  render(){
    return(
      <nav>
        {this.props.tabs.map((tab, i) => {
          return <span onClick={this.props.changeTab} key={i}>{tab}</span>;
        })}
      </nav>
    );
  }
}

class SettingsItem extends React.Component {
  render(){
    return(
      <div>
        <span>{this.props.setting_name}</span>
        <span>{this.props.setting_value}</span>
      </div>
    );
  }
}
