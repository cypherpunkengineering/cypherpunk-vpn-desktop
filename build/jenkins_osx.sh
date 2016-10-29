#!/bin/bash

# fix cwd
cd "$( dirname "${BASH_SOURCE[0]}" )"

# fix path
export PATH=$PATH:/usr/local/bin

# use nvm to set nodejs version
export NVM_DIR="$HOME/.nvm"
source "$(brew --prefix nvm)/nvm.sh"
nvm use v6.8.0

# select and unlock keychain for signing
KEYCHAIN="cypherpunk.keychain-db"
# sudo security list-keychains -d common -s "${KEYCHAIN}"
# security find-identity # should list signing identities
security default-keychain -s "${KEYCHAIN}"
security unlock-keychain -p cypherpunkkeychainpassword "${KEYCHAIN}"

# call macos build script
./build_osx.sh

# archive package artifact
PKG="../out/CypherpunkVPN.pkg"
ARTIFACT="../`printf 'cypherpunk-vpn-macos-%05d' ${BUILD_NUMBER}`.pkg"
mv "${PKG}" "${ARTIFACT}"

# done
exit 0
