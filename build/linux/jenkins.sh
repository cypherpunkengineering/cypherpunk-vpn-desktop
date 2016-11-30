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
[ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh"
nvm install v6.8.0
nvm alias default v6.8.0
nvm use v6.8.0

# Invoke build script
../build_linux.sh

# Archive build artifacts
#scp -P92 ../../out/osx/cypherpunk-*.pkg upload@builds-upload.cypherpunk.engineering:/data/builds/ || echo "* Warning: failed to upload build"

# Done
exit 0
