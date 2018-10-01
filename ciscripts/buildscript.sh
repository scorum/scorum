#!/bin/bash
set -e

UPLOAD_PATH=${UPLOAD_PATH:-develop}
BRANCH_NAME=${BRANCH_NAME:-scorum}
LIVE_TESTNET=${LIVE_TESTNET:-OFF}

if [[ -z "${GIT_COMMIT}" ]]; then
  GIT_COMMIT=$(git rev-parse --short HEAD)
fi

if [[ ${UPLOAD_PATH} = "testnet" ]]; then
    LIVE_TESTNET=ON
fi

IMAGE_NAME="scorum/${UPLOAD_PATH}:${BRANCH_NAME}.${GIT_COMMIT:0:7}"

# Build container
sudo docker build \
--build-arg BRANCH_NAME=${BRANCH_NAME} \
--build-arg GIT_COMMIT=${GIT_COMMIT} \
--build-arg AZURE_UPLOAD=${AZURE_UPLOAD} \
--build-arg AZURE_STORAGE_ACCOUNT=${AZURE_STORAGE_ACCOUNT} \
--build-arg AZURE_STORAGE_ACCESS_KEY=${AZURE_STORAGE_ACCESS_KEY} \
--build-arg AZURE_STORAGE_CONNECTION_STRING=${AZURE_STORAGE_CONNECTION_STRING} \
--build-arg UPLOAD_PATH=${UPLOAD_PATH} \
--build-arg LIVE_TESTNET=${LIVE_TESTNET} \
--build-arg BUILD_VERSION=${BUILD_VERSION} \
--tag=${IMAGE_NAME} .

