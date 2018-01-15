import React from 'react';
import { Link } from 'react-router';
import daemon, { DaemonAware } from '../../daemon.js';
import { SecondaryTitlebar } from '../Titlebar';
import { Subpanel, PanelContent } from '../Panel';
import { classList } from '../../util';
import analytics from '../../analytics';


export const Item = ({ setting, value, align = "left top", children, ...props }) => (
  <div className="setting" {...props}>
    <div className={classList("ui radio checkbox", align)}>
      <input type="radio" name={setting} value={value} id={`${setting}-${value}`}/>
      <label>{children}</label>
    </div>
  </div>
);

export const Pane = ({ title, setting, align, children, ...props }) => (
  <div className="pane" data-title={title} {...props}>
    { React.Children.map(children, c => React.cloneElement(c, { setting, align })) }
  </div>
);

export default class SelectScreen extends DaemonAware(React.Component)  {
  static defaultProps = {
    title: "Setting",
    setting: 'setting',
    align: 'left top',
    type: 'string',
  }
  constructor(props) {
    super(props);
    this.state = { back: this.props.location.pathname.replace(/\/[^/]+\/?$/, '') };
  }
  componentDidMount() {
    super.componentDidMount();
    $(this.refs.root).find('.ui.checkbox').click(this.onClick.bind(this)).checkbox({ onChange: this.onChanged.bind(this) });
    this.onDaemonSettingsChanged();
  }
  onChanged() {
    if (this.updatingSettings) return;
    var value = $(this.refs.root).find(`input[name=${this.props.setting}]:checked`).val();
    switch (this.props.type) {
      case 'int': value = Number.parseInt(value); break;
      case 'float':
      case 'number': value = Number.parseFloat(value); break;
    }
    daemon.post.applySettings({ [this.props.setting]: value });
    analytics.event('Setting', this.props.setting, { label: value });
  }
  onClick() {
    setImmediate(() => { History.push(this.state.back); });
  }
  onDaemonSettingsChanged() {
    this.updatingSettings = true;
    $(this.refs.root).find(`#${this.props.setting}-${daemon.settings[this.props.setting]}`).parent().checkbox('set checked');
    delete this.updatingSettings;
  }
  render() {
    const hasPanes = React.Children.toArray(this.props.children).some(c => c.type === Pane);
    let children = React.Children.map(this.props.children, c => React.cloneElement(c, { setting: this.props.setting, align: this.props.align }));
    return(
      <Subpanel>
        <PanelContent>
          <SecondaryTitlebar title={this.props.title} back={this.state.back}/>
          <div className="scrollable content" ref="root">
            { hasPanes ? children : <div className="pane">{children}</div> }
          </div>
        </PanelContent>
      </Subpanel>
    );
  }
}
