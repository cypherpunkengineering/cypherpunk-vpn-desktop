# Cypherpunk Privacy Desktop Client

The Cypherpunk Privacy desktop client is written in [Electron](http://electron.atom.io), with a native background service/daemon (to perform actual VPN tasks in privileged mode) written in C++11.

## Background Daemon

The background helper service project is in the `daemon` subdirectory.

## How to clone/develop/run

The desktop client app project is in the `client` subdirectory.
To get started, you'll need [Git](https://git-scm.com) and [Node.js](https://nodejs.org/en/download/) (which comes with [npm](http://npmjs.com)) installed on your computer.
You also need to have an instance of the background daemon running.
Then, run the following from your command line:

```bash
# Go into the client directory
cd client
# Install dependencies
npm install
# Run the app
npm start
```

The `npm start` command takes the following parameters:

* `--debug`: Open a Web Inspector window for the main window.
* `--background`: Create but initially don't show the main window (used at system startup).
* `--semantic`: Launch the alternative Semantic UI version.

## Building on Windows

In addition to the prerequisites above, you'll need the following installed on your machine:

* [Visual Studio 2015](https://beta.visualstudio.com/vs/community/) (with C++ support) or [Visual C++ Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools)
* [Inno Setup 5](http://www.jrsoftware.org/isdl.php)
* [npm](http://npmjs.com) version >= 3.10.7, see [this workaround](https://www.npmjs.com/package/npm-windows-upgrade).

Then, run the following from your command line:

```bash
cd build
build.bat
```
