#!/bin/bash
set -e

# set umask to prevent weird permission bugs
umask 022

# change pwd to folder containing this file
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd ../

# get app version, and git hash or build number
APP_VERSION_SHORT="$(grep version client/package.json|head -1|cut -d \" -f4)"
if [ -z "${BUILD_NUMBER}" ];then
	GIT_HASH=$(git --git-dir="./.git" describe --always --match=nosuchtagpattern --dirty=-p)
	BUILD_NUMBER="${BUILD_NAME}-${GIT_HASH}"
	APP_VERSION=$(echo "${APP_VERSION_SHORT}" | sed -E "s/^([^-+]*)(-[^+]*)?(\+.*)?/\1\2${BUILD_NUMBER}/")
else
	APP_VERSION=$(echo "${APP_VERSION_SHORT}" | sed -E "s/^([^-+]*)(-[^+]*)?(\+.*)?/\1\2+${BUILD_NUMBER}/")
fi

# export app version so electron can build it into app UI
export APP_VERSION

# build vars
NODE_VER=v6.9.1
PLATFORM=linux
OS=DEBIAN # must be caps
ARCH=x64
LINUX_ARCH=amd64
BITS=64

# app vars
PKG_NAME="cypherpunk-privacy-${PLATFORM}-${ARCH}"
APP_NAME="Cypherpunk Privacy"
APP_NS="com.cypherpunk.privacy"
APP_PATH="./usr/local/cypherpunk"
OUT_PATH="./out/${PKG_NAME}"
PKG_STR="${PKG_NAME}_${APP_VERSION}"
PKG_FILE="${PKG_STR}.deb"
PKG_PATH="out/${PKG_FILE}"
ELECTRON_NAME="${APP_NAME}-${PLATFORM}-${ARCH}"

# clean workspace
rm -rf "${OUT_PATH}"
rm -f "${PKG_PATH}"

# init workspace
mkdir -p "${OUT_PATH}"

# load nvm
export NVM_DIR="$HOME/.nvm"
source "$NVM_DIR/nvm.sh" "${NODE_VER}"

# set node version
nvm install "${NODE_VER}"
nvm alias default "${NODE_VER}"
nvm use "${NODE_VER}"

# enable debug prints
set -x

# build client app
cd client
npm install
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
make

# prepare dirs
cd "../../${OUT_PATH}/"
mkdir -p "./${APP_PATH}/"
mkdir -p "./${APP_PATH}/bin/"
mkdir -p "./${APP_PATH}/scripts/"

# get app bundle
mv "${ELECTRON_NAME}"/* "./${APP_PATH}/"
rm -r "./${ELECTRON_NAME}"

# Copy OpenVPN scripts to app bundle
#cp -pR "../../res/${PLATFORM}/openvpn-scripts/*" "${APP_PATH}/scripts/"

# move bin
mv "${APP_PATH}/${APP_NAME}" "${APP_PATH}/bin/"
# install daemon
install -c -m 755 "../../daemon/posix/cypherpunk-privacy-service" "${APP_PATH}/bin/cypherpunk-privacy-service"
# install OpenVPN binary
install -c -m 755 "../../daemon/third_party/openvpn_${PLATFORM}/${BITS}/openvpn" "${APP_PATH}/bin/cypherpunk-privacy-openvpn"

# make dirs
mkdir -p "./${OS}/"
mkdir -p "./etc/init.d/"

# install daemon init script
install -c -m 755 "../../res/${PLATFORM}/install-scripts/cypherpunk" "./etc/init.d/"

# install package scripts
install -c -m 755 "../../res/${PLATFORM}/install-scripts/preinst" "./${OS}/"
install -c -m 755 "../../res/${PLATFORM}/install-scripts/postinst" "./${OS}/"

# write debian package control file
cat > "./${OS}/control" << _EOF_
Package: ${PKG_NAME}
Description: Cypherpunk Privacy linux vpn client app
Version: ${APP_VERSION}
Architecture: ${LINUX_ARCH}
Maintainer: Cypherpunk Privacy <debian-maintainer@cypherpunk.com>
Installed-Size: 150000
Homepage: https://cypherpunk.com
_EOF_

# can be used to make new package in the future
#export DEBFULLNAME="Cypherpunk Privacy"
#export DEBEMAIL="support@cypherpunk.com"
#dh_make -y -p "${PKG_NAME}" -n -s
#dpkg-buildpackage -us -uc -b
#debuild

# create debian package
cd ..
dpkg -b "${PKG_NAME}" "${PKG_FILE}"

# cleanup
rm -rf "${PKG_NAME}"

# show artifact
ls -la "${PKG_FILE}"

# done
exit 0
