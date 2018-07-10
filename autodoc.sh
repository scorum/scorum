#!/bin/bash

set -e

UPLOAD_PATH=${UPLOAD_PATH:-develop}
DOCS_PATH=${DOCS_PATH:-docs}


echo "Generate documentation by doxygen"
sudo doxygen Doxyfile

echo "Upload generated documentation to azure"
az storage blob upload-batch \
    --destination ${DOCS_PATH} \
    --source ${WORKSPACE}/doxygen \
    --destination-path ${UPLOAD_PATH}
