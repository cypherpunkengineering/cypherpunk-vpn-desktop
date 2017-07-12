import 'semantic/components/dimmer';
import 'semantic/components/loader';

let loader = null;
let loaderLabel = null;
let loading = true;

export default class Loader {
  static show(text) {
    if (loaderLabel) loaderLabel.text(text || "Loading");
    if (!loading) {
      loading = true;
      Loader.update();
    }
  }
  static hide() {
    if (loading) {
      loading = false;
      Loader.update();
    }
  }
  static update() {
    if (!loader) return;
    if (loading) {
      $(loader).dimmer('show');
      if (!loader.open) {
        loader.showModal();
      }
    } else {
      if (loader.open) {
        loader.close();
      }
      $(loader).dimmer('hide');
    }
  }
}

function onDocumentReady() {
  loader = document.getElementById('load-screen');
  loader.addEventListener('cancel', event => event.preventDefault());
  loaderLabel = $(loader).find('div');
  Loader.update();
}

if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', onDocumentReady, false); else onDocumentReady();
