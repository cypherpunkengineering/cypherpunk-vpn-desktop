#!/bin/bash -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

# Initialize and update submodules
git submodule update --init --recursive

# Pad the build number to five digits
export BUILD_NUMBER="$(printf '%05d' "${BUILD_NUMBER}")"

# Fix path
export PATH=$PATH:/usr/local/bin

# Use nvm to set nodejs version
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

# Invoke build script
make vpn great again

# Archive build artifacts
scp -P92 ../../out/osx/cypherpunk-*.pkg upload@builds-upload.cypherpunk.engineering:/data/builds/ || echo "* Warning: failed to upload build"

# Done
exit 0
