// Reimplement a downsized version of the auto-launch module to work around a few bugs

const { app } = require('electron');

var appName = "CypherpunkPrivacy";
var execPath = app.getPath('exe');

var AutoStart = {};

if (process.platform == 'win32') {

  const reg = require('winreg');

  const key = new reg({ hive: reg.HKCU, key: '\\Software\\Microsoft\\Windows\\CurrentVersion\\Run' });
  //appName = execPath.split('\\').slice(-1)[0].replace(/\.exe$/, '');

  AutoStart.enable = () => new Promise((resolve, reject) => {
    key.set(appName, reg.REG_SZ, `"${execPath}" --background`, err => { if (err) reject(err); else resolve(); });
  });
  AutoStart.disable = () => new Promise((resolve, reject) => {
    key.remove(appName, err => { if (err && err.message.indexOf('unable to find the specified registry key') < 0) reject(err); else resolve(); });
  });
  AutoStart.isEnabled = () => new Promise((resolve, reject) => {
    key.get(appName, (err, item) => resolve(!(err || !item)));
  });

} else if (process.platform == 'darwin') {

  const applescript = require('applescript');

  execPath = execPath.replace(/\.app\/Content.*$/, '.app');
  appName = execPath.split('/').slice(-1)[0].replace(/\.app$/, '');

  const runAppleScript = what => new Promise((resolve, reject) => {
    applescript.execString(`tell application "System Events" to ${what}`, (err, result) => { if (err) reject(err); else resolve(result); });
  });
  AutoStart.enable = () => runAppleScript(`make login item at end with properties { path: ${JSON.stringify(execPath)}, name: ${JSON.stringify(appName)}, kind: "Application", hidden: true }`);
  AutoStart.disable = () => runAppleScript(`delete login item ${JSON.stringify(appName)}`);
  AutoStart.isEnabled = () => runAppleScript('get the name of every login item').then(items => (items && items.indexOf(appName) >= 0)).catch(err => false);

} else if (process.platform == 'linux') {

  const fs = require('fs');

  const path = `${app.getPath('appData')}/autostart/${appName}.desktop`;

  function ensureDirExists(path) {
    if (!fs.existsSync(path)) fs.mkdirSync(path);
  }
  AutoStart.enable = () => new Promise((resolve, reject) => {
    var data = `[Desktop Entry]
                Type=Application
                Version=${app.getVersion()}
                Name=${appName}
                Comment=${appName} launch script
                Exec=${execPath} --background
                StartupNotify=false
                Terminal=false`.replace(/^ +/m, '');
    ensureDirExists(path.split('/').slice(0,-2).join('/'));
    ensureDirExists(path.split('/').slice(0,-1).join('/'));
    fs.writeFile(path, data, err => { if (err) reject(err); else resolve(); });
  });
  AutoStart.disable = () => new Promise((resolve, reject) => {
    fs.unlink(path, err => { if (err) reject(err); else resolve(); });
  });
  AutoStart.isEnabled = () => new Promise((resolve, reject) => {
    fs.access(path, err => resolve(!err));
  });

} else {
  throw new Error("Unsupported platform");
}

module.exports = AutoStart;
