#!/bin/bash -e

# set umask to prevent weird permission bugs
umask 022

# change pwd to folder containing this file
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../

# app vars
APP_NAME="CypherpunkPrivacy"
APP_NS="com.cypherpunk.privacy"
export APP_VER="$(cat client/package.json | grep version | cut -d '"' -f 4)"

# installer vars
INSTALL_PKG="CypherpunkPrivacy.pkg"

# clean workspace
rm -rf out/linux
rm -f "out/${INSTALL_PKG}"

# build client app
mkdir -p out/linux
cd client
npm install
npm --production run build
./node_modules/.bin/electron-rebuild

# package app
./node_modules/.bin/electron-packager ./app/ "${APP_NAME}" \
--app-bundle-id="${APP_NS}.app" \
--platform=linux \
--arch=x64 \
--icon=../res/osx/logo2.icns \
--out=../out/linux

# build daemon
cd ../daemon/posix/
make

# get app bundle
cd "../../out/linux/"
mkdir -p "usr/local/"
mv "${APP_NAME}-linux-x64" "usr/local/cypherpunk"

# Copy OpenVPN scripts to app bundle
mkdir -p "usr/local/cypherpunk/scripts/"
cp -pR ../../res/linux/openvpn-scripts/ "usr/local/cypherpunk/scripts"

# install daemon
mkdir -p "usr/local/cypherpunk/bin/"
install -c -m 755 ../../daemon/posix/cypherpunk-privacy-service usr/local/cypherpunk/bin/cypherpunk-privacy-service
# install OpenVPN binary
install -c -m 755 ../../daemon/third_party/openvpn_linux/64/openvpn usr/local/cypherpunk/bin/cypherpunk-privacy-openvpn

# install daemon init script
# TODO: daemon init startup script

# Ensure install scripts are executable
install -c -m 755 ../../res/linux/install-scripts/preinstall .
install -c -m 755 ../../res/linux/install-scripts/postinstall .

# Package
# run dpkg-deb

# cleanup
#rm -rf usr

# show artifact
#ls -la "${INSTALL_PKG}"

# done
exit 0
