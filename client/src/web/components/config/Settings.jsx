import React from 'react';
import ReactDOM from 'react-dom';
import { Link } from 'react-router';
import { ipcRenderer as ipc } from 'electron';
import daemon, { DaemonAware } from '../../daemon.js';

import { classList } from '../../util.js';

const SettingLine = ({ className = null, hidden = false, indented = false, disabled = false, ...props } = {}) => <div className={classList('setting', { 'hidden': hidden, 'indented': indented, 'disabled': disabled })} {...props}/>;

const Setting = DaemonAware(React.Component);

export class CheckboxSetting extends Setting {
  static defaultProps = {
    onChange: function() {},
    on: true,
    off: false,
    align: null,
  }
  componentDidMount() {
    super.componentDidMount();
    var self = this;
    $(this.refs.ui).checkbox({ onChange: function() { self.onChange(this.checked); } }).checkbox('set ' + (this.props.disabled ? 'disabled' : 'enabled'));
    this.daemonSettingsChanged(daemon.settings);
  }
  componentWillReceiveProps(props) {
    $(this.refs.ui).checkbox('set ' + (props.disabled ? 'disabled' : 'enabled'));
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.hasOwnProperty(this.props.name)) { $(this.refs.ui).checkbox('set ' + (settings[this.props.name] != this.props.off ? 'checked' : 'unchecked')); }
    delete this.updatingSettings;
  }
  onChange(value) {
    if (this.updatingSettings) return;
    daemon.post.applySettings({ [this.props.name]: value ? this.props.on : this.props.off });
    this.props.onChange(value);
  }
  render() {
    return (
      <SettingLine className={this.props.className} hidden={this.props.hidden} indented={this.props.indented} disabled={this.props.disabled}>
        <div className="ui toggle checkbox" ref="ui">
          <input type="checkbox" name={this.props.name} id={this.props.name} ref={this.props.name} ref="input"/>
          <label>{this.props.label}{this.props.caption ? <small>{this.props.caption}</small> : null}{this.props.children}</label>
        </div>
      </SettingLine>
    );
  }
}

export class RadioSetting extends Setting {
  static defaultProps = {
    onChange: function(value) {},
    onClick: function() {},
    value: true,
    align: null,
  }
  componentDidMount() {
    super.componentDidMount();
    var self = this;
    $(this.refs.ui).click(this.props.onClick).checkbox({ onChange: this.onChange.bind(this) }).checkbox('set ' + (this.props.disabled ? 'disabled' : 'enabled'));
    this.daemonSettingsChanged(daemon.settings);
  }
  componentWillReceiveProps(props) {
    $(this.refs.ui).checkbox('set ' + (props.disabled ? 'disabled' : 'enabled'));
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.hasOwnProperty(this.props.name) && settings[this.props.name] === this.props.value) { $(this.refs.ui).checkbox('set checked'); }
    delete this.updatingSettings;
  }
  onChange(value) {
    if (this.updatingSettings) return;
    if ($(this.refs.ui).checkbox('is checked')) {
      daemon.post.applySettings({ [this.props.name]: this.props.value });
      this.props.onChange(value);
    }
  }
  render() {
    return (
      <SettingLine className={this.props.className} hidden={this.props.hidden} indented={this.props.indented} disabled={this.props.disabled}>
        <div className={classList("ui radio checkbox", this.props.align)} ref="ui">
          <input type="radio" name={this.props.name} value={this.props.value} id={`${this.props.name}-${(this.props.value+'').replace(/[^A-Za-z0-9]+/, '-')}`} ref="input"/>
          <label>{this.props.label}{this.props.caption ? <small>{this.props.caption}</small> : null}{this.props.children}</label>
        </div>
      </SettingLine>
    );
  }
}

export class InputSetting extends Setting {
  static defaultProps = {
    onChange: function(v) {},
    type: 'text',
    size: 5,
  }
  componentDidMount() {
    super.componentDidMount();
    var self = this;
    //$(this.refs.ui).click(event => event.currentTarget.children[0].children[0].focus());
    $(this.refs.input).change(event => self.onChange(event.target.value));
    this.daemonSettingsChanged(daemon.settings);
  }
  onChange(value) {
    if (this.updatingSettings) return;
    daemon.post.applySettings({ [this.props.name]: parseInt(value, 10) }); // FIXME: format
    this.props.onChange(value);
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.hasOwnProperty(this.props.name)) { $(this.refs.input).val(settings[this.props.name] || ""); } // FIXME: specific format
    delete this.updatingSettings;
  }
  render() {
    return (
      <SettingLine className={this.props.className} hidden={this.props.hidden} indented={this.props.indented} disabled={this.props.disabled} onClick={() => this.refs.input.focus()}>
        <div className="ui input" ref="ui">
          <input type={this.props.type} size={this.props.size} name={this.props.name} id={this.props.name} disabled={this.props.disabled ? 'disabled' : null} ref="input"/>
        </div>
        <label>{this.props.label}{this.props.caption ? <small>{this.props.caption}</small> : null}</label>
      </SettingLine>
    );
  }
}

export class LinkSetting extends Setting {
  static defaultProps = {
    onClick: function() {},
    formatValue: function(v) { return v; },
  }
  componentDidMount() {
    super.componentDidMount();
    this.daemonSettingsChanged(daemon.settings);
  }
  daemonSettingsChanged(settings) {
    this.updatingSettings = true;
    if (settings.hasOwnProperty(this.props.name)) { $(this.refs.link).attr('data-value', this.props.formatValue(settings[this.props.name])); }
    delete this.updatingSettings;
  }
  render() {
    return (
      <SettingLine className={this.props.className} hidden={this.props.hidden} indented={this.props.indented} disabled={this.props.disabled}>
        <Link to={this.props.to} tabIndex={this.props.disabled ? "-1" : "0"} onClick={!this.props.disabled && this.props.onClick} data-value={this.props.formatValue(daemon.settings[this.props.name])} ref="link">{this.props.label}{this.props.caption ? <small>{this.props.caption}</small> : null}</Link>
      </SettingLine>
    );
  }
}
