#!/bin/bash
cd "$( dirname "${BASH_SOURCE[0]}" )"
export NVM_DIR="$HOME/.nvm"
source "$(brew --prefix nvm)/nvm.sh"
nvm use v6.8.0
./build_osx.sh
PKG="../out/CypherpunkVPN.pkg"
ARTIFACT="../`printf 'cypherpunk-vpn-macos-%05d' ${BUILD_NUMBER}`.pkg"
mv "${PKG}" "${ARTIFACT}"
exit 0
