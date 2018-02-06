#!/bin/bash

if [ "$AZURE_UPLOAD" = true ]; then
  ARCHIVE_NAME="scorum_$BRANCH_NAME_${GIT_COMMIT:0:7}.tar.gz"
  echo "Creating arhive with base files"
  /bin/tar czf /var/lib/scorumd/$ARCHIVE_NAME --directory=/usr/local/scorumd-full/bin scorumd cli_wallet
  echo "Upload arhive to azure"
  /usr/local/bin/az storage blob upload --container-name $AZURE_CONTAINER_NAME --file /var/lib/scorumd/$ARCHIVE_NAME --name $ARCHIVE_NAME
else
  echo "Azure upload disabled"
fi
