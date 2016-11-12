#!/bin/bash -e

# fetch submodules
git submodule update --init --recursive

# fix cwd
cd "$( dirname "${BASH_SOURCE[0]}" )"

# fix path
export PATH=$PATH:/usr/local/bin

# use nvm to set nodejs version
export NVM_DIR="$HOME/.nvm"
source "$(brew --prefix nvm)/nvm.sh" v6.8.0
nvm install v6.8.0
nvm alias default v6.8.0
nvm use v6.8.0

# select and unlock keychain for signing
KEYCHAIN="cypherpunk.keychain-db"
# sudo security list-keychains -d common -s "${KEYCHAIN}"
# security find-identity # should list signing identities
security default-keychain -s "${KEYCHAIN}"
security unlock-keychain -p cypherpunkkeychainpassword "${KEYCHAIN}"
security set-keychain-settings -u -t 3600 "${KEYCHAIN}"

# call macos build script
./build_osx.sh

# archive package artifact
PKG="../out/Cypherpunk Privacy.pkg"
ARTIFACT="../`printf 'cypherpunk-vpn-macos-%05d' ${BUILD_NUMBER}`.pkg"
mv "${PKG}" "${ARTIFACT}"
scp -P92 "${ARTIFACT}" upload@builds-upload.cypherpunk.engineering:/data/builds/

# done
exit 0
