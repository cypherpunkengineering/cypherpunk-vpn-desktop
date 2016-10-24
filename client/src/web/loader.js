import 'semantic';

var loader = document.getElementById('load-screen');
var loaderLabel = $(loader).find('div');
var loading = true;

loader.addEventListener('cancel', event => event.preventDefault());

$(loader).dimmer({ closable: false });
$(loader).dimmer('show');
loader.showModal();

export default class Loader {
  static show(text) {
    var label = text || "Loading";
    loading = true;
    loaderLabel.text(label);
    $(loader).dimmer('show');
    if (!loader.open) {
      loader.showModal();
    }
  }
  static hide() {
    $(loader).dimmer('hide');
    if (loader.open) {
      loader.close();
    }
    loading = false;
  }
}
