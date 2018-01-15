import { app } from 'electron';
import tray from './tray.js';
import path from 'path';

const DEFAULT_NOTIFICATION_ICON = `assets/img/icon_128.png`;

export default class Notification
{
  constructor(title = "Private Internet Access", options = {}) {
    this.success = true;

    if (!options.force && (!daemon || !daemon.settings.showNotifications)) {
      this.success = false;
    } else {
      delete options.force;
      if (typeof(title) !== 'string') {
        options = Object.assign(title, options);
        title = "Private Internet Access";
      }
      do {
        // FIXME: MacOS displays the icon as a second image on High Sierra, figure out if it needs to be specified as another property or whether the app icon is fixed and unchangable
        if (!options.icon && process.platform !== 'darwin') options.icon = DEFAULT_NOTIFICATION_ICON;
        // Make paths absolute
        ['badge','icon','image'].forEach(attr => {
          if (options[attr] && options[attr].indexOf(':') < 0 && !options[attr].startsWith('/')) {
            options[attr] = path.resolve(__dirname, options[attr]);
          }
        });
        // Use tray.displayBalloon if applicable
        if (options.tray || !window)
        {
          if (process.platform === 'win32' && tray) {
            tray.getElectronTray().displayBalloon({
              icon: options.icon,
              title: title,
              content: options.body
            });
            break;
          } else {
            delete options.tray;
          }
        }
        if (window) {
          if (!options.hasOwnProperty('silent')) options.silent = true;
          window.webContents.executeJavaScript(`new Notification(${JSON.stringify(title)}, ${JSON.stringify(options)});`);
        } else {
          this.success = false;
        }
      } while (false);
    }
  }
}
