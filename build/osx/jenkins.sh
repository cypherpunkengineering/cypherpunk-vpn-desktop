#!/bin/bash -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

# Initialize and update submodules
git submodule update --init --recursive

# Fix path
export PATH=$PATH:/usr/local/bin
export PATH=$PATH:$HOME/Library/google-cloud-sdk/bin

# Use nvm to set nodejs version
export NVM_DIR="$HOME/.nvm"
source "$(brew --prefix nvm)/nvm.sh" v6.9.2
nvm install v6.9.2
nvm alias default v6.9.2
nvm use v6.9.2

# select and unlock keychain for signing
KEYCHAIN="cypherpunk.keychain-db"
# sudo security list-keychains -d common -s "${KEYCHAIN}"
# security find-identity # should list signing identities
security default-keychain -s "${KEYCHAIN}"
security unlock-keychain -p cypherpunkkeychainpassword "${KEYCHAIN}"
security set-keychain-settings -u -t 3600 "${KEYCHAIN}"

# Override and use the Cypherpunk Partners certificates for Jenkins builds
export APP_SIGN_IDENTITY='"Developer ID Application: Cypherpunk Partners, slf. (3498MVRSX2)"'
export INSTALLER_SIGN_IDENTITY='"Developer ID Installer: Cypherpunk Partners, slf. (3498MVRSX2)"'

# Invoke build script
make vpn great again

# Archive build artifacts
cd ../../out/osx/
ARTIFACT=$(echo cypherpunk-*.zip)
echo "Uploading build to builds repo..."
scp -P92 "${ARTIFACT}" upload@builds-upload.cypherpunk.engineering:/data/builds/
echo "Uploading build to GCS bucket..."
gsutil cp "${ARTIFACT}" gs://builds.cypherpunk.com/builds/macos/
echo "Sending notification to slack..."
curl -X POST --data "payload={\"text\": \"cypherpunk-privacy-macos build $(cat ../../version.txt) is now available from https://download.cypherpunk.com/builds/macos/${ARTIFACT}\"}" https://hooks.slack.com/services/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

# Done
exit 0
