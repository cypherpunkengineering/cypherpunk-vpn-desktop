import tray from './tray.js';

const DEFAULT_NOTIFICATION_ICON = `${__dirname}/assets/img/`

export default class Notification
{
  constructor(title = "Cypherpunk Privacy", options = {}) {
    this.success = true;

    if (!options.force && (!daemon || !daemon.settings.showNotifications)) {
      this.success = false;
    } else {
      delete options.force;
      if (typeof(title) !== 'string') {
        options = Object.assign(title, options);
        title = "Cypherpunk Privacy";
      }
      do {
        //if (!options.icon) options.icon = DEFAULT_NOTIFICATION_ICON;
        if (options.tray || !window)
        {
          if (process.platform === 'win32' && tray) {
            tray.displayBalloon({
              icon: options.icon && `${__dirname}/${options.icon}`,
              title: title,
              content: options.body
            });
            break;
          } else {
            delete options.tray;
          }
        }
        if (window) {
          // Adjust image URLs
          ['badge','icon','image'].forEach(attr => {
            if (options[attr] && options[attr].indexOf(':') < 0)
              options[attr] = '../' + options[attr];
          });
          options.silent = true;
          window.webContents.executeJavaScript(`new Notification(${JSON.stringify(title)}, ${JSON.stringify(options)});`);
        }
      } while (false);
    }
  }
}
