import { app, dialog } from 'electron';
import fs from 'fs';
import child from 'child_process';
import { nodePromise } from './util.js';

export function check() {
  switch (process.platform) {
    case 'darwin': return mac_check();
    case 'win32':
    case 'linux':
    default: return dummy_check();
  }
}

export function run(options) {
  switch (process.platform) {
    case 'darwin': return mac_run(options);
    case 'win32':
    case 'linux':
    default: return dummy_run(options);
  }
}


function mac_check() {
  var binaryPath = app.getPath('exe');
  const debugBuild = !binaryPath.endsWith('.app/Contents/MacOS/Cypherpunk Privacy'); // developer builds will use an Electron binary

  // 1. Check if the 'cypherpunk' group exists
  let { gid: groupID, hasEveryone: groupHasEveryone } = require('./install_darwin.js').checkGroup(); // doesn't actually need root
  let needsGroup = groupID === null || !groupHasEveryone;

  // 2. Check if the daemon is installed
  let daemonInstalled = fs.existsSync('/Library/LaunchDaemons/com.cypherpunk.privacy.service.plist') && fs.existsSync('/usr/local/cypherpunk/bin/cypherpunk-privacy-service');

  // 3. Check if the daemon is running
  let daemonRunning = false;
  try {
    child.execSync('pgrep cypherpunk-privacy-service');
    daemonRunning = true;
  } catch (e) {}

  // 4. Check if the client binary has the correct group and the setgid bit set
  let hasCorrectPermissions = false;
  try {
    let stats = fs.statSync(binaryPath);
    hasCorrectPermissions = stats.gid === groupID && (stats.mode & 1024) != 0;
  } catch (e) {}

  // 5. Check if we're in the Applications folder
  let [ inApplications, applicationRelativeFolder, recursiveAppFolderCount ] = (m => [ !!m, m && m[2], (m && m[2].match(/\.app\//g) || []).length ])(binaryPath.match(/^\/(Users\/[^\/]+\/)?Applications\/(.*)/));
  let clientInstalled = inApplications && recursiveAppFolderCount <= 1;

  let result = { debugBuild, groupInstalled: !needsGroup, daemonInstalled, daemonRunning, clientPermissions: hasCorrectPermissions, clientInstalled };

  if (daemonRunning && hasCorrectPermissions && (debugBuild || (daemonInstalled && clientInstalled)) && hasCorrectPermissions && !needsGroup) {
    result.status = 'installed';
  } else {
    result.status = 'needs install';
  }

  return result;
}

function mac_run(status) {
  let options = {};
  options.installDaemon = (!status.daemonRunning || !(status.daemonInstalled && !status.debugBuild));
  options.moveToApplications = !status.clientInstalled && !status.debugBuild;
  options.fixPermissions = !status.groupInstalled || !status.clientPermissions;

  return new Promise((resolve, reject) => {
    let msg = {
      type: 'none',
      message: "Install Cypherpunk Privacy?",
      detail: "Cypherpunk Privacy needs to make some changes to your computer in order to work. This will require an administrator password.",
      buttons: [ "Install", "Quit" ],
      defaultId: 0,
      cancelId: 1,
    };
    //if (options.moveToApplications) {
    //  msg.detail += "\n\nAdditionally, we recommend moving the app to the Applications folder.";
    //  msg.checkboxLabel = "Move to Applications";
    //  msg.checkboxChecked = true;
    //}
    dialog.showMessageBox(msg, function (response, checked) {
      if (response === msg.cancelId) {
        console.log('Aborting installation'); // for some reason, this is necessary to avoid a UI delay - message loop pump?
        return resolve({ exit: 0 });
      }
      //if (options.moveToApplications && !checked) {
      //  options.moveToApplications = false;
      //}
      console.log('Proceeding with installation');
      require('./install_darwin.js').run(options).then(resolve, reject);
    });
    app.show();
    app.focus();
  });
}


function dummy_check() {
  return { status: 'installed' };
}

function dummy_run(options) {
  return { status: 'installed' };
}
