#!/bin/bash

if [ "$AZURE_UPLOAD" = true ]; then

  if [[ -z "${BUILD_VERSION}"  ]]; then
    ARCHIVE_NAME="scorumd_${BRANCH_NAME}_${GIT_COMMIT:0:7}.tar.gz"
  else
    ARCHIVE_NAME="scorumd_${BUILD_VERSION}_${GIT_COMMIT:0:7}.tar.gz"
  fi

  echo "Creating arhive with base files"
  /bin/tar czf /var/lib/scorumd/$ARCHIVE_NAME --directory=/usr/local/scorumd-full/bin scorumd cli_wallet

  echo "Upload arhive to azure"
  /usr/local/bin/az storage blob upload --container-name $UPLOAD_PATH --file /var/lib/scorumd/$ARCHIVE_NAME --name $ARCHIVE_NAME

else

  echo "Azure upload disabled"

fi
