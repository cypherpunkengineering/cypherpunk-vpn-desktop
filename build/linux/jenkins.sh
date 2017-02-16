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
cd ../../out/
ARTIFACT=$(echo cypherpunk-*.deb)
echo "Uploading build to builds repo..."
scp -P92 "${ARTIFACT}" upload@builds-upload.cypherpunk.engineering:/data/builds/
echo "Uploading build to GCS bucket..."
gsutil cp "${ARTIFACT}" gs://builds.cypherpunk.com/builds/debian/
echo "Sending notification to slack..."
curl -X POST --data "payload={\"text\": \"cypherpunk-privacy-debian build ${BUILD_NUMBER} is now available from https://download.cypherpunk.com/builds/debian/${ARTIFACT}\"}" https://hooks.slack.com/services/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

# Done
exit 0
