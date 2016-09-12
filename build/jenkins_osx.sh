#!/bin/bash -e
cd "$( dirname "${BASH_SOURCE[0]}" )"
export PATH=$PATH:$HOME/.nvm/versions/node/v6.4.0/bin/
./build_osx.sh
PKG="../out/CypherpunkVPN.pkg"
ARTIFACT="../`printf 'cypherpunk-vpn-macos-%05d' ${BUILD_NUMBER}`.pkg"
mv "${PKG}" "${ARTIFACT}"
exit 0
