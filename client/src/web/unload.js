import { ipcRenderer as ipc, remote } from 'electron';

var exiting = false;

// Hook the window.onbeforeunload event (in case the window itself is closed)
window.addEventListener('beforeunload', e => {
  if (exiting || (exiting = remote.getGlobal('exiting'))) {
    // Do nothing, allow window to close
  } else {
    ipc.send('hide-window');
    e.returnValue = false;
    return false;
  }
});

