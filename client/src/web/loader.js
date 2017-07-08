import 'semantic/components/dimmer';
import 'semantic/components/loader';

var loader = document.getElementById('load-screen');
var loaderLabel = $(loader).find('div');
var loading = true;

loader.addEventListener('cancel', event => event.preventDefault());

$(loader).dimmer({ closable: false });
$(loader).dimmer('set dimmed');
loader.showModal();

export default class Loader {
  static show(text) {
    var label = text || "Loading";
    loaderLabel.text(label);
    if (!loading) {
      loading = true;
      $(loader).dimmer('show');
      if (!loader.open) {
        loader.showModal();
      }
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
