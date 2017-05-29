#!/bin/bash -x
set -e

# set umask to prevent weird permission bugs
umask 022

# change pwd to folder containing this file
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../../

# app vars
APP_NAME="Cypherpunk Privacy"
APP_NAME_NOSPACE="CypherpunkPrivacy"
APP_DESC="Cypherpunk Privacy is your guardian of online privacy and freedom."
APP_HOMEPAGE="https://cypherpunk.com/"
APP_NS="com.cypherpunk.privacy"
APP_PATH_ABSOLUTE="/usr/local/cypherpunk"
APP_PATH_RELATIVE="./usr/local/cypherpunk"
APP_SIZE_KB=150000

# build vars
NODE_VER=v6.9.2
PLATFORM=linux
OS=DEBIAN # must be caps
ARCH=x64
LINUX_ARCH=amd64
BITS=64

# load nvm, set node version
set +x
export NVM_DIR="$HOME/.nvm"
source "$NVM_DIR/nvm.sh" "${NODE_VER}"
nvm install "${NODE_VER}"
nvm alias default "${NODE_VER}"
nvm use "${NODE_VER}"
set -x

# Generate version number
cd client
npm install
npm run build-version
npm run apply-version
cd ..

# Extract the short build number for packaging purposes
APP_VERSION="$(cat version.txt)"
APP_VERSION_SHORT="${APP_VERSION%%+*}"

# pkg vars
PKG_NAME="cypherpunk-privacy-${PLATFORM}-${ARCH}"
PKG_MAINTAINER="Cypherpunk Privacy <debian-maintainer@cypherpunk.com>"
PKG_STR="${PKG_NAME}-${APP_VERSION}"
PKG_STR_MINUS="${PKG_STR//+/-}"
PKG_FILE="${PKG_STR_MINUS}.deb"
PKG_PATH="out/${PKG_FILE}"

# script vars
OUT_PATH="./out/${PKG_NAME}"
ELECTRON_NAME="${APP_NAME}-${PLATFORM}-${ARCH}"

# clean workspace
rm -rf "${OUT_PATH}"
rm -f "${PKG_PATH}"

# init workspace
mkdir -p "${OUT_PATH}"

# build client app
cd client
npm --production run build

# rebuild electron stuff in case outdated cached stuff from before
# NB: install these required deps for electron-rebuild
# sudo apt-get install libgtk2.0-0 libasound2-dev libgconf2-dev libxrandr-dev libxcomposite-dev libxdamage-dev libxcursor-dev libpango1.0-dev libcairo2-dev libgdk-pixbuf2.0-dev libpangocairo-1.0-0 libxss-dev
./node_modules/.bin/electron-rebuild

# package app
./node_modules/.bin/electron-packager ./app/ "${APP_NAME}" \
--app-bundle-id="${APP_NS}.app" \
--platform="${PLATFORM}" \
--arch="${ARCH}" \
--icon="../res/osx/logo2.icns" \
--out="../${OUT_PATH}"

# build daemon
cd ../daemon/posix/
make RELEASE=1 all

# prepare dirs
cd "../../${OUT_PATH}/"
mkdir -p "./${APP_PATH_RELATIVE}/"
mkdir -p "./${APP_PATH_RELATIVE}/etc/"
mkdir -p "./${APP_PATH_RELATIVE}/etc/settings/"
mkdir -p "./${APP_PATH_RELATIVE}/etc/scripts/"
mkdir -p "./${APP_PATH_RELATIVE}/bin/"
mkdir -p "./usr/share/applications/"

# get app bundle
mv "${ELECTRON_NAME}"/* "./${APP_PATH_RELATIVE}/"
rm -r "./${ELECTRON_NAME}"

# rename app binary to remove space
mv "${APP_PATH_RELATIVE}/${APP_NAME}" "${APP_PATH_RELATIVE}/${APP_NAME_NOSPACE}"
# install daemon
install -c -m 755 "../../daemon/posix/out/cypherpunk-privacy-service" "${APP_PATH_RELATIVE}/bin/cypherpunk-privacy-service"
# install OpenVPN binary
install -c -m 755 "../../daemon/third_party/openvpn_${PLATFORM}/${BITS}/openvpn" "${APP_PATH_RELATIVE}/bin/cypherpunk-privacy-openvpn"

# make dirs
mkdir -p "./${OS}/"
mkdir -p "./etc/init.d/"

# add daemon init script
install -c -m 755 "../../res/${PLATFORM}/install-scripts/cypherpunk" "./etc/init.d/"

# add launcher icon
install -c -m 755 "../../res/${PLATFORM}/launcher_linux_512.png" "./${APP_PATH_RELATIVE}/"

# add package scripts
install -c -m 755 "../../res/${PLATFORM}/install-scripts/preinst" "./${OS}/"
install -c -m 755 "../../res/${PLATFORM}/install-scripts/postinst" "./${OS}/"
install -c -m 755 "../../res/${PLATFORM}/install-scripts/prerm" "./${OS}/"
install -c -m 755 "../../res/${PLATFORM}/install-scripts/postrm" "./${OS}/"

# Copy OpenVPN scripts to app bundle
install -c -m 755 "../../res/${PLATFORM}/openvpn-scripts/updown.sh" "${APP_PATH_RELATIVE}/etc/scripts/"

# write debian package control file
cat > "./${OS}/control" << _EOF_
Package: ${PKG_NAME}
Description: ${APP_DESC}
Version: ${APP_VERSION_SHORT}
Architecture: ${LINUX_ARCH}
Maintainer: ${PKG_MAINTAINER}
Installed-Size: ${APP_SIZE_KB}
Homepage: ${APP_HOMEPAGE}
Depends: python-appindicator
_EOF_

cat > "./usr/share/applications/${APP_NS}.desktop" << _EOF_
[Desktop Entry]
Version=${APP_VERSION_SHORT}
Name=${APP_NAME}
Comment=${APP_DESC}
Exec=env XDG_CURRENT_DESKTOP=Unity ${APP_PATH_ABSOLUTE}/${APP_NAME_NOSPACE}
Icon=${APP_PATH_ABSOLUTE}/launcher_linux_512.png
Terminal=false
Type=Application
Categories=Utility;Application;
_EOF_

# can be used to make new package in the future
#export DEBFULLNAME="Cypherpunk Privacy"
#export DEBEMAIL="support@cypherpunk.com"
#dh_make -y -p "${PKG_NAME}" -n -s
#dpkg-buildpackage -us -uc -b
#debuild

# remove any LICENSE* files, and create debian package
cd ..
find "${PKG_NAME}" -name LICENSE\* -exec rm {} \;
fakeroot dpkg -b "${PKG_NAME}" "${PKG_FILE}"

# cleanup
rm -rf "${PKG_NAME}"

# show artifact
ls -la "${PKG_FILE}"

# done
exit 0
