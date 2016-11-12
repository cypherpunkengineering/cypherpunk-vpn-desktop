#!/bin/bash -e

# set umask to prevent weird permission bugs
umask 022

# change pwd to folder containing this file
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../

# app vars
APP_BUNDLE="CypherpunkPrivacy.app"
APP_NAME="CypherpunkPrivacy"
APP_NS="com.cypherpunk.privacy"
APP_SIGN_IDENTITY="Developer ID Application: Cypherpunk Engineering K.K. (S353YJSBDX)"
export APP_VER="$(cat client/package.json | grep version | cut -d '"' -f 4)"

# installer vars
INSTALL_PKG="CypherpunkPrivacy.pkg"
INSTALLER_SIGN_IDENTITY="Developer ID Installer: Cypherpunk Engineering K.K. (S353YJSBDX)"

# clean workspace
rm -rf out/osx
rm -f "out/${INSTALL_PKG}"

# build client app
mkdir -p out/osx/Applications
cd client
npm install
npm --production run build
./node_modules/.bin/electron-rebuild

# package app
./node_modules/.bin/electron-packager ./app/ "${APP_NAME}" \
--app-bundle-id="${APP_NS}.app" \
--platform=darwin \
--arch=x64 \
--icon=../res/osx/logo2.icns \
--out=../out/osx/Applications

# get app bundle
cd ../
mv "out/osx/Applications/${APP_NAME}-darwin-x64/${APP_BUNDLE}" out/osx/Applications/
rm -rf "out/osx/Applications/${APP_NAME}-darwin-x64"

# fudge plist to insert space in app display name
sed -i -e 's!\t<key>CFBundleDisplayName</key>\r\t<string>CypherpunkPrivacy</string>!\t<key>CFBundleDisplayName</key>\r\t<string>Cypherpunk Privacy</string>!' out/osx/Applications/${APP_BUNDLE}/Contents/Info.plist

# Copy OpenVPN scripts to app bundle
mkdir -p "out/osx/Applications/${APP_BUNDLE}/Contents/Resources/scripts"
cp -pR res/osx/openvpn-scripts/ "out/osx/Applications/${APP_BUNDLE}/Contents/Resources/scripts"

# sign app bundle contents recursively
sleep 3
codesign --force --deep --sign "${APP_SIGN_IDENTITY}" "out/osx/Applications/${APP_BUNDLE}"

# build daemon
cd daemon/posix
make
cd ../..
mkdir -p out/osx/usr/local/bin
install -c -m 755 daemon/posix/cypherpunk-privacy-service out/osx/usr/local/bin
codesign --sign "${APP_SIGN_IDENTITY}" out/osx/usr/local/bin/cypherpunk-privacy-service

# prepare LaunchDaemon plist for the daemon
mkdir -p out/osx/Library/LaunchDaemons
cp ./res/osx/plist/${APP_NS}.service.plist out/osx/Library/LaunchDaemons

# prepare LaunchService plist for the client
mkdir -p out/osx/Library/LaunchAgents
cp ./res/osx/plist/${APP_NS}.client.plist out/osx/Library/LaunchAgents

# TUN/TAP adapter extensions
mkdir -p out/osx/Library/Extensions
cp -pR daemon/third_party/tuntap_osx/tap.kext out/osx/Library/Extensions/
cp -pR daemon/third_party/tuntap_osx/tun.kext out/osx/Library/Extensions/
mkdir -p out/osx/Library/LaunchDaemons
cp daemon/third_party/tuntap_osx/net.sf.tuntaposx.tap.plist out/osx/Library/LaunchDaemons/
cp daemon/third_party/tuntap_osx/net.sf.tuntaposx.tun.plist out/osx/Library/LaunchDaemons/

# OpenVPN binary
mkdir -p out/osx/usr/local/bin
install -c -m 755 ./daemon/third_party/openvpn_osx/openvpn out/osx/usr/local/bin/cypherpunk-privacy-openvpn
codesign -s "${APP_SIGN_IDENTITY}" out/osx/usr/local/bin/cypherpunk-privacy-openvpn || true # XXX: ignore 'is already signed' error from causing build script to fail

# Ensure install scripts are executable
chmod +x res/osx/install-scripts/postinstall
chmod +x res/osx/install-scripts/preinstall

# Package
cd out

# run pkgbuild
pkgbuild \
--root osx \
--scripts ../res/osx/install-scripts \
--sign "${INSTALLER_SIGN_IDENTITY}" \
--identifier "${APP_NS}.pkg" \
--version "${APP_VER}" \
--ownership recommended \
--install-location / \
Build.pkg

# run productbuild for installer
productbuild \
--resources ../res/osx/resources \
--distribution ../res/osx/resources/distribution.xml \
--sign "${INSTALLER_SIGN_IDENTITY}" \
--version "${APP_VER}" \
"${INSTALL_PKG}"

# cleanup
rm -f Build.pkg
rm -rf osx

# show artifact
ls -la "${INSTALL_PKG}"

# done
exit 0
