#!/bin/bash

if [ "$AZURE_UPLOAD" = true ]; then
  ARCHIVE_NAME=scorum_$(echo $GIT_BRANCH | cut -d '/' -f 2)_${GIT_COMMIT:0:7}.tar.gz
  echo "Creating arhive with base files"
  /bin/tar -cvzf /var/lib/scorumd/$ARCHIVE_NAME /usr/local/scorumd-full/bin/scorumd /usr/local/scorumd-full/bin/cli_wallet
  echo "Upload arhive to azure"
  /usr/local/bin/az storage blob upload --container-name $AZURE_CONTAINER_NAME --file /var/lib/scorumd/$ARCHIVE_NAME --name $ARCHIVE_NAME
else
  echo "Azure upload disabled"
fi
