#!/bin/bash
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../

CODESIGNATURE="FIXME"

export APP_VER="$(cat client/package.json | grep version | cut -d '"' -f 4)"

rm -rf out/osx
rm -f out/CypherVPN.pkg
rm -f out/CypherVPN.pkg.zip

# Client
mkdir -p out/osx/Applications
cd client
npm install
./node_modules/.bin/electron-rebuild
./node_modules/.bin/electron-packager ./ CypherVPN --platform=darwin --arch=x64 --icon=../res/logo.icns --out=../out/osx/Applications
cd ../
mv out/osx/Applications/CypherVPN-darwin-x64/CypherVPN.app out/osx/Applications/
rm -rf out/osx/Applications/CypherVPN-darwin-x64
sleep 3
codesign --force --deep --sign "${CODESIGNATURE}" out/osx/Applications/CypherVPN.app

# Service
cd daemon/posix
make
cd ../..
mkdir -p out/osx/usr/local/bin
cp daemon/posix/cyphervpn-service out/osx/usr/local/bin/cyphervpn-service
codesign --sign "${CODESIGNATURE}" out/osx/usr/local/bin/cyphervpn-service

# LaunchDaemon for the service
mkdir -p out/osx/Library/LaunchDaemons
cp service_osx/com.cypherpunk.vpn.service.plist out/osx/Library/LaunchDaemons

# LaunchAgent for the client
mkdir -p out/osx/Library/LaunchAgents
cp service_osx/com.cypherpunk.vpn.client.plist out/osx/Library/LaunchAgents

# TUN/TAP adapter extensions
mkdir -p out/osx/Library/Extensions
cp -pR daemon/third_party/tuntap_osx/tap.kext out/osx/Library/Extensions/
cp -pR daemon/third_party/tuntap_osx/tun.kext out/osx/Library/Extensions/
mkdir -p out/osx/Library/LaunchDaemons
cp daemon/third_party/tuntap_osx/net.sf.tuntaposx.tap.plist out/osx/Library/LaunchDaemons/
cp daemon/third_party/tuntap_osx/net.sf.tuntaposx.tun.plist out/osx/Library/LaunchDaemons/

# OpenVPN binary
mkdir -p out/osx/usr/local/bin
cp openvpn_osx/openvpn out/osx/usr/local/bin/cyphervpn-openvpn
codesign -s "${CODESIGNATURE}" out/osx/usr/local/bin/cyphervpn-openvpn

# Ensure install scripts are executable
chmod +x res/osx_resources/scripts/postinstall
chmod +x res/osx_resources/scripts/preinstall

# Package
cd out
pkgbuild --root osx --scripts ../res/osx_scripts --sign "${CODESIGNATURE}" --identifier com.cypherpunk.pkg.CypherVPN --version $APP_VER --ownership recommended --install-location / Build.pkg
productbuild --resources ../res/osx_resources --distribution ../osx_resources/distribution.xml --sign "${CODESIGNATURE}" --version $APP_VER CypherVPN.pkg
zip Pritunl.pkg.zip Pritunl.pkg
rm -f Build.pkg
