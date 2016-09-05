import 'semantic';

var loader = $('#load-screen');
var loaderLabel = loader.find('div');
var loading = true;

loader.dimmer({ closable: false });
loader.dimmer('show');

function filterEvent(e) {
  if (loading) {
    e.preventDefault();
    e.stopPropagation();
    return false;
  }
}

[ 'keydown', 'keypress', 'keyup', 'scroll' ].forEach(e => {
  document.body.addEventListener(e, filterEvent, true);
});

export default class Loader {
  static show(text) {
    var label = text || "Loading";
    loading = true;
    loaderLabel.text(label);
    loader.dimmer('show');
  }
  static hide() {
    loader.dimmer('hide');
    loading = false;
  }
}
