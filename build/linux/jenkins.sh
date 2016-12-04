#!/bin/bash -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

# Initialize and update submodules
git submodule update --init --recursive

# Pad the build number to five digits
export BUILD_NUMBER="$(printf '%05d' "${BUILD_NUMBER}")"

# Fix path
export PATH=$PATH:/usr/local/bin

# Invoke build script
../build_linux.sh

# Archive build artifacts
scp -P92 ../../out/cypherpunk-*.deb upload@builds-upload.cypherpunk.engineering:/data/builds/ || echo "* Warning: failed to upload build"

# Done
exit 0
