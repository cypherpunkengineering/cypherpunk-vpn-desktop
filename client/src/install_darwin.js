const fs = require('fs');
const path = require('path');
const child = require('child_process');
const { nodePromise } = require('./util.js');

const sudoPrompt = require('sudo-prompt');

const THIS_APP_PATH = path.resolve(__dirname, '../../..');
const ICON_PATH = path.resolve(THIS_APP_PATH, 'Contents/Resources/electron.icns');
const DAEMON_ARCHIVE = path.resolve(__dirname, '../daemon.tar.bz2');

const DAEMON_PATH_WHITELIST = [
  'Library/LaunchDaemons/',
  'usr/local/cypherpunk/',
];


export function run(...args) {
  if (process.getuid() === 0) {
    return runAsRoot(...args);
  } else {
    return new Promise((resolve, reject) => {
      let cmd = `ELECTRON_RUN_AS_NODE=1 "${process.execPath}" "${__filename}" --sudo '${new Buffer(JSON.stringify(args)).toString('base64')}'`;
      console.log("Executing " + cmd);
      sudoPrompt.exec(cmd, { name: "Cypherpunk Privacy", icns: ICON_PATH }, function(err, stdout, stderr) {
        if (err) return reject(err);
        stdout = String(stdout);
        stderr = String(stderr);
        try {
          if (stderr) reject(JSON.parse(stderr));
          else if (stdout) resolve(JSON.parse(stdout));
        } catch (e) { reject('unable to parse result'); }
      });
    });
  }
}

// Perform any necessary installation steps while holding root privileges
function runAsRoot(options) {

  let log = [];
  let result = { log };

  function exec(cmd) { return new Promise((resolve, reject) => { child.exec(cmd, (error, stdout, stderr) => { log.push({ cmd, result: error, stdout: stdout.toString(), stderr: stderr.toString() }); if (error) reject(error); else resolve(stdout); }); }); }
  function execFile(file, args) { return new Promise((resolve, reject) => { child.execFile(file, args, (error, stdout, stderr) => { log.push({ file, args, result: error, stdout: stdout.toString(), stderr: stderr.toString() }); if (error) reject(error); else resolve(stdout); }); }); }

  let { installDaemon = false, startDaemon = false, moveToApplications = false, fixPermissions = false, uninstall = false } = options;

  let group = checkGroup();

  let createGroup = group.gid === null || !group.hasEveryone;
  let stopDaemon = installDaemon || createGroup;
  startDaemon = startDaemon || installDaemon || createGroup;
  let reloadPF = installDaemon || startDaemon || createGroup;
  fixPermissions = fixPermissions || createGroup || moveToApplications;

  let p = Promise.resolve();

  p = p.then(() => nodePromise(cb => fs.writeFile('/tmp/testrootfile.txt', __filename + "\n" + __dirname + "\n" + JSON.stringify(options) + "\n", cb)));

  if (uninstall) {

    result.exit = 0;

  } else {

    if (group.gid === null) {
      p = p.then(() =>
        // List all groups
        exec('dscl . -list /Groups PrimaryGroupID', cb)
        // Find an available group ID between 333 and 500
        .then(stdout => {
          let takenGroupIDs = [];
          stdout.split('\n').map(l => l.split(' ', 2)).filter(l => l.length == 2).map(l => l[1].trim()).forEach(e => takenGroupIDs[Number(e)] = true);
          let firstAvailableGroupID;
          for (let i = 333; i < 500; i++) {
            if (!takenGroupIDs[i]) {
              firstAvailableGroupID = i;
              break;
            }
          }
          if (!firstAvailableGroupID) {
            throw new Error("Unable to find an available group ID");
          }
          return group.gid = firstAvailableGroupID;
        })
        // Create group
        .then(gid => exec('dscl . -create /Groups/cypherpunk gid ' + gid))
      );
    }
    if (!group.hasEveryone) {
      // Add 'everyone' to the group as a nested group, otherwise users will not be able to run binaries
      // with +setgid owned by the cypherpunk group (this does not affect everyone's traffic)
      p = p.then(() => exec('dseditgroup -o edit -a everyone -t group cypherpunk').then(() => { group.hasEveryone = true; }));
    }
    if (createGroup) {
      // Flush any changes so they appear to other posix commands
      p = p.then(() => child.exec('dscacheutil -flushcache'));
    }
    if (stopDaemon) {
      p = p.then(() => exec('launchctl unload /Library/LaunchDaemons/com.cypherpunk.privacy.service.plist').catch(() => {}));
    }
    if (installDaemon) {
      p = p.then(() => execFile('/usr/bin/tar', [ 'xyvf', DAEMON_ARCHIVE, '-C', '/', ...DAEMON_PATH_WHITELIST ]));
    }
    if (reloadPF) {
      p = p.then(() => exec('pfctl -q -f /etc/pf.conf'));
    }
    if (startDaemon) {
      p = p.then(() => exec('launchctl load /Library/LaunchDaemons/com.cypherpunk.privacy.service.plist'));
    }
    if (moveToApplications) {
      if (THIS_APP_PATH !== '/Applications/Cypherpunk Privacy.app') {
        p = p.then(() => exec('rm -rf "/Applications/Cypherpunk Privacy.app"'));
        p = p.then(() => exec(`mv -f "${THIS_APP_PATH}" /Applications/`));
        result.relaunch = { execPath: '/Applications/Cypherpunk Privacy.app/Contents/MacOS/Cypherpunk Privacy', args: [ '--clean', THIS_APP_PATH ] };
      } else {
        result.relaunch = {};
      }
      p = p.then(() => execFile('/usr/sbin/chown', [ '-R', 'root:wheel', '/Applications/Cypherpunk Privacy.app' ]));
      p = p.then(() => exec('xattr -dr com.apple.quarantine "/Applications/Cypherpunk Privacy.app"'));
    }
    if (fixPermissions) {
      let clientExecutable = moveToApplications ? '/Applications/Cypherpunk Privacy.app/Contents/MacOS/Cypherpunk Privacy' : THIS_APP_PATH;
      p = p.then(() => execFile('/usr/bin/chgrp', [ 'cypherpunk', clientExecutable ]));
      p = p.then(() => execFile('/bin/chmod', [ 'g+s', clientExecutable ]));
      if (!result.relaunch) result.relaunch = {};
    }
  }

  return p.then(() => Object.assign(result, { status: 'installed' }), e => { throw Object.assign(result, { error: String(e) }); });
}


// Check if a 'cypherpunk' group exists on the machine
export function checkGroup() {
  let result = { gid: null, hasEveryone: false };
  try {
    let stdout = child.execSync('dscl . -read /Groups/cypherpunk').toString().split('\n');
    for (let s of stdout) {
      if (s.startsWith('PrimaryGroupID: ')) {
        result.gid = Number.parseInt(s.slice(16));
      } else if (s.startsWith('NestedGroups: ')) {
        result.hasEveryone = s.slice(14).split().includes('ABCDEFAB-CDEF-ABCD-EFAB-CDEF0000000C');
      }
    }
  } catch (e) {}
  return result;
}


// This bit of code triggers when this file is run directly with Electron-as-Node
if (process.argv.length >= 4 && process.argv[1] === __filename && process.argv[2] === '--sudo') {
  if (process.getuid() !== 0) {
    console.error('"not running as root"');
    process.exit(1);
  } else {
    let args = JSON.parse(new Buffer(process.argv[3], 'base64').toString('utf8'));
    runAsRoot(...args)
    .then(v => {
      try { console.log(JSON.stringify(v)); } catch (e) { console.log(JSON.stringify(String(v))); }
    }, e => {
      try { console.error(JSON.stringify(e instanceof Error ? e.message : e)); } catch (e2) { console.error('"error"'); }
    })
    .then(() => process.exit(0));
  }
}
