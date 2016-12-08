const { app, dialog } = require('electron');
const fs = require('fs');
const sudo = require('sudo-fn');
import child_process from 'child_process';
import { nodePromise } from './util.js';

sudo.setName("Cypherpunk Privacy");

exports.checkInstalled = function() {
  return new Promise(function(resolve, reject) {
    if (process.platform !== 'darwin') {
      return resolve('installed');
    }
    var binaryPath = app.getPath('exe');
    // Perform quick checks to determine if we're installed:
    // 1. Check if the daemon is registered
    // 2. Check if the client has the setgid bit set
    // 3. (Optional) Check if we're in the Applications folder
    const daemonInstalled = fs.existsSync('/Library/LaunchDaemons/com.cypherpunk.privacy.service.plist');
    const hasSetgid = (fs.statSync(binaryPath).mode & 1024) != 0;
    const [ inApplications, applicationRelativeFolder, recursiveAppFolderCount ] = (m => [ !!m, m && m[1], (m && m[1].match(/\.app\//g) || []).length ])(binaryPath.match(/^\/(Users\/[^\/]+\/)?Applications\/(.*)/));
    const debugBuild = fs.existsSync(__dirname + '/../../.git');
    const clientInstalled = inApplications && recursiveAppFolderCount <= 1;

    if (daemonInstalled && hasSetgid) {
      return resolve('installed');
    }
    if (debugBuild) {
      if (!daemonInstalled) {
        try {
          var pid = child_process.execSync('pgrep cypherpunk-privacy-service');
          console.log("Daemon not installed but found running process " + pid.toString().trim() + ", continuing.");
        } catch (e) {
          dialog.showErrorBox("Cypherpunk Privacy Service not installed", "Please install a working copy of the Cypherpunk Privacy helper service either from an official build or via make install-daemon.");
          resolve('quit');
          app.exit(1);
          return;
        }
      }
      // Just fudgenut and set the electron binary's group+setgid
      if (!hasSetgid) {
        dialog.showMessageBox({
          type: 'warning',
          message: "Fix Electron permissions?",
          detail: "In order for the killswitch to work, the client binary must have its permissions adjusted. For debug builds, this means tweaking the Electron binary in your local node_modules directory.",
          buttons: [ "Fix now", "Ignore", "Quit" ],
          defaultId: 0,
          cancelId: 2,
        }, function(id) {
          if (id === 0) {
            console.log("Fixing permissions on Electron binary...");
            fixPermissions(binaryPath).then(() => {
              console.log("Permissions fixed, restart required.");
              resolve('restart');
              app.relaunch();
              app.exit();
            }, err => {
              console.log("Failed to set permissions: " + err);
              reject(err);
            });
          } else if (id === 1) {
            console.log("Ignoring missing permissions on Electron binary.");
            resolve('restart');
            app.relaunch();
            app.exit();
          } else {
            console.log("Aborting.");
            resolve('quit');
            app.exit();
          }
        });
        app.show();
        app.focus();
        return;
      }
      return resolve('installed');
    }
    var detail = "Cypherpunk Privacy needs to make some changes to your computer in order to work. This will require an administrator password.";
    var buttons = [ "Install", "Quit" ];
    var cancelId = 1;
    if (!inApplications) {
      detail += "\n\nAdditionally, we recommend moving the app to the Applications folder.";
      buttons = [ "Install to Applications", "Install without moving", "Quit" ];
      cancelId = 2;
    }
    dialog.showMessageBox({
      type: 'none',
      message: "Install Cypherpunk Privacy?",
      detail,
      buttons,
      defaultId: 0,
      cancelId,
    }, function(id) {
      console.log(id);
      if (id === cancelId) {
        resolve('quit');
        app.exit();
      } else {
        const moveToApplications = !inApplications && id === 0;
        install(moveToApplications ? 'install-move' : 'install-only').then(() => {
          if (inApplications || id === 1) {
            if (hasSetgid) {
              console.log("Installation successful, safe to continue.");
              resolve('installed');
            } else {
              console.log("Installation successful, restart required.");
              resolve('restart');
              app.relaunch();
              app.exit();
            }
          } else {
            console.log("Installation successful, restart required.");
            resolve('restart');
            app.relaunch({ execPath: newPath + '/Contents/MacOS/Cypherpunk Privacy', args: process.argv.slice(1).concat([ '--clean', binaryPath ])});
            app.exit();
          }
          resolve(inApplications || id === 1 ? 'installed' : 'restart');
        }, err => {
          reject(err);
        });
      }
    });
    app.show();
    app.focus();
  });
}

function runAsRoot(functionName, ...args) {
  return nodePromise(cb => sudo.call({
    module: __filename,
    function: functionName,
    params: args,
    type: 'promise',
  }, cb));
}

export function install(operation) {
  return runAsRoot('rootInstall', operation);
}
export function rootInstall(operation) {
  return nodePromise(cb => fs.writeFile('/tmp/testrootfile.txt', __filename + "\n" + __dirname + "\n", cb));
  var result = new Promise((resolve, reject) => {

  });
  if (operation == "install-move") {
    // Copy current *.app/* to /Applications/*.app/*
    result = result.then(res => {

    });
  }
  return result;
}

export function uninstall() {
  return runAsRoot('rootUninstall');
}
export function rootUninstall() {
  return new Promise((resolve, reject) => {
    resolve();
    // rm -rf /usr/local/cypherpunk
    // rm -rf "/Users/$USER/Library/Application Support/Cypherpunk Privacy"
  });
}

export function fixPermissions(path) {
  if (!path) path = app.getPath('exe');
  return runAsRoot('rootFixPermissions', path);
}
export function rootFixPermissions(path) {
  var uid, gid, mode;
  return nodePromise(cb => fs.stat(path, cb)).then(stat => {
    ({ uid, gid, mode } = stat);
  })
  //.then(() => nodePromise(cb => fs.chown(path, uid, /*cypherpunk*/, cb)))
  .then(() => nodePromise(cb => child_process.execFile('/usr/bin/chgrp', [ 'cypherpunk', path ], cb)))
  .then(() => nodePromise(cb => fs.chmod(path, mode | 1024, cb)))
  //.then(() => nodePromise(cb => child_process.execFile('/bin/chmod', [ 'g+s', path ])))
  .then(() => true, err => {
    console.log(err);
    return false;
  });
}

// Check if a 'cypherpunk' group exists on the machine
export function checkCypherpunkGroup() {
  return nodePromise(cb => child_process.exec('dscl . -read /Groups/cypherpunk', cb)).then(() => true, () => false);
}

// Set up a 'cypherpunk' group on the machine, which will be used to exempt Cypherpunk's own traffic from the killswitch
export function rootCreateCypherpunkGroup() {
  return checkCypherpunkGroup().then(exists => {
    if (exists) {
      return true;
    }
    // Find an available group id
    return nodePromise(cb => child_process.exec('dscl . -list /Groups PrimaryGroupID', cb)).then(stdout => {
      var takenGroupIDs = [];
      stdout.split('\n').map(l => l.split(' ', 2)).filter(l => l.length == 2).map(l => l[1].trim()).forEach(e => takenGroupIDs[Number(e)] = true);
      var firstAvailableGroupID;
      for (var i = 333; i < 500; i++) {
        if (!takenGroupIDs[i]) {
          firstAvailableGroupID = i;
          break;
        }
      }
      if (!firstAvailableGroupID) {
        throw new Error("Unable to find an available group ID");
      }
      return firstAvailableGroupID;
    })
    // Create group
    .then(gid => nodePromise(cb => child_process.exec('dscl . -create /Groups/cypherpunk gid ' + gid, cb)))
	  // Add 'everyone' to the group as a nested group, otherwise users will not be able to run binaries
	  // with +setgid owned by the cypherpunk group (this does not affect everyone's traffic)
    .then(stdout => nodePromise(cb => child_process.exec('dseditgroup -o edit -a everyone -t group cypherpunk', cb)))
    // Done, clean up and catch any error so far
    .then(() => true, err => {
      console.log(err);
      return false;
    });
  });
}
