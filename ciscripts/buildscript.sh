#!/bin/bash
set -e

UPLOAD_PATH=${UPLOAD_PATH:-develop}

IMAGE_NAME="scorum/$UPLOAD_PATH:$BRANCH_NAME.${GIT_COMMIT:0:7}"

sudo docker build --build-arg BRANCH_NAME=$BRANCH_NAME --build-arg GIT_COMMIT=$GIT_COMMIT --build-arg AZURE_UPLOAD=$AZURE_UPLOAD \
--build-arg AZURE_STORAGE_ACCOUNT=$AZURE_STORAGE_ACCOUNT --build-arg AZURE_STORAGE_ACCESS_KEY=$AZURE_STORAGE_ACCESS_KEY \
--build-arg AZURE_STORAGE_CONNECTION_STRING=$AZURE_STORAGE_CONNECTION_STRING --build-arg UPLOAD_PATH=$UPLOAD_PATH \
--build-arg BUILD_VERSION=$BUILD_VERSION --tag=$IMAGE_NAME .
#sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
#sudo docker push $IMAGE_NAME
#sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
#cp -r /var/jenkins_home/cobertura .
