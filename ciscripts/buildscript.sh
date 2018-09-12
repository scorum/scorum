#!/bin/bash
set -e

UPLOAD_PATH=${UPLOAD_PATH:-develop}
BRANCH_NAME=${BRANCH_NAME:-scorum}

if [[ -z "${GIT_COMMIT}" ]]; then
  GIT_COMMIT=$(git rev-parse --short HEAD)
fi

IMAGE_NAME="scorum/$UPLOAD_PATH:$BRANCH_NAME.${GIT_COMMIT:0:7}"

# Preparing configuration files
if [[ "$UPLOAD_PATH" == "testnet" ]]; then
  CONF_TYPE="testnet"
else
  CONF_TYPE="mainnet"
fi
cat $WORKSPACE/contrib/seeds.ini.$CONF_TYPE >> $WORKSPACE/contrib/config.ini.witness || exit 1
cat $WORKSPACE/contrib/seeds.ini.$CONF_TYPE >> $WORKSPACE/contrib/config.ini.rpc || exit 1

# Build container
sudo docker build \
--build-arg BRANCH_NAME=$BRANCH_NAME \
--build-arg GIT_COMMIT=$GIT_COMMIT \
--build-arg AZURE_UPLOAD=$AZURE_UPLOAD \
--build-arg AZURE_STORAGE_ACCOUNT=$AZURE_STORAGE_ACCOUNT \
--build-arg AZURE_STORAGE_ACCESS_KEY=$AZURE_STORAGE_ACCESS_KEY \
--build-arg AZURE_STORAGE_CONNECTION_STRING=$AZURE_STORAGE_CONNECTION_STRING \
--build-arg UPLOAD_PATH=$UPLOAD_PATH \
--build-arg BUILD_VERSION=$BUILD_VERSION \
--tag=$IMAGE_NAME .
