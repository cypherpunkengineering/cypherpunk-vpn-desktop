#!/bin/bash -e

# set umask to prevent weird permission bugs
umask 022

cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../

APPNAME="Cypherpunk Privacy"
APPSIGNIDENTITY="Developer ID Application: Cypherpunk Engineering K.K. (S353YJSBDX)"
INSTALLERSIGNIDENTITY="Developer ID Installer: Cypherpunk Engineering K.K. (S353YJSBDX)"

export APP_VER="$(cat client/package.json | grep version | cut -d '"' -f 4)"

rm -rf out/osx
rm -f out/"${APPNAME}".pkg
rm -f out/"${APPNAME}".pkg.zip

# Client
mkdir -p out/osx/Applications
cd client
npm install
npm --production run build
./node_modules/.bin/electron-rebuild
./node_modules/.bin/electron-packager ./app/ "${APPNAME}" --platform=darwin --arch=x64 --icon=../res/osx/logo2.icns --out=../out/osx/Applications
cd ../
mv "out/osx/Applications/${APPNAME}-darwin-x64/${APPNAME}.app" out/osx/Applications/
rm -rf "out/osx/Applications/${APPNAME}-darwin-x64"

# Copy OpenVPN scripts
mkdir -p "out/osx/Applications/${APPNAME}.app/Contents/Resources/scripts"
cp -pR res/osx/openvpn-scripts/ "out/osx/Applications/${APPNAME}.app/Contents/Resources/scripts"

sleep 3
codesign --force --deep --sign "${APPSIGNIDENTITY}" "out/osx/Applications/${APPNAME}.app"

# Service
cd daemon/posix
make
cd ../..
mkdir -p out/osx/usr/local/bin
install -c -m 755 daemon/posix/cypherpunk-privacy-service out/osx/usr/local/bin
codesign --sign "${APPSIGNIDENTITY}" out/osx/usr/local/bin/cypherpunk-privacy-service

# LaunchDaemon for the service
mkdir -p out/osx/Library/LaunchDaemons
cp ./res/osx/plist/com.cypherpunk.privacy.service.plist out/osx/Library/LaunchDaemons

# LaunchAgent for the client
mkdir -p out/osx/Library/LaunchAgents
cp ./res/osx/plist/com.cypherpunk.privacy.client.plist out/osx/Library/LaunchAgents

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
codesign -s "${APPSIGNIDENTITY}" out/osx/usr/local/bin/cypherpunk-privacy-openvpn || true # XXX: ignore 'is already signed' error from causing build script to fail

# Ensure install scripts are executable
chmod +x res/osx/install-scripts/postinstall
chmod +x res/osx/install-scripts/preinstall

# Package
cd out
pkgbuild --root osx --scripts ../res/osx/install-scripts --sign "${INSTALLERSIGNIDENTITY}" --identifier "com.cypherpunk.privacy.pkg" --version "${APP_VER}" --ownership recommended --install-location / Build.pkg
productbuild --resources ../res/osx/resources --distribution ../res/osx/resources/distribution.xml --sign "${INSTALLERSIGNIDENTITY}" --version "${APP_VER}" "${APPNAME}.pkg"
#zip "${APPNAME}.pkg.zip" "${APPNAME}.pkg"
rm -f Build.pkg
rm -rf osx
ls -la "${APPNAME}.pkg"
exit 0
