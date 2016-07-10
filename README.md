# Cypherpunk VPN Desktop Client

The Cypherpunk VPN desktop client is written in [Electron](http://electron.atom.io), with a native background service/daemon (to perform actual VPN tasks in privileged mode) written in C++11.

## Background Daemon

The background helper service project is in the `daemon` subdirectory.

## How to clone/develop/run

The desktop client app project is in the `client` subdirectory.
To get started, you'll need [Git](https://git-scm.com) and [Node.js](https://nodejs.org/en/download/) (which comes with [npm](http://npmjs.com)) installed on your computer.
You also need to have an instance of the background daemon running.
Then, run the following from your command line:

```bash
# Go into the repository
cd client
# Install dependencies and run the app
npm install && npm start
```
