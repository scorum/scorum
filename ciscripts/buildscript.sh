#!/bin/bash
set -e

IMAGE_NAME="scorum/scorum:$BRANCH_NAME"

sudo docker build --build-arg GIT_BRANCH=$GIT_BRANCH --build-arg GIT_COMMIT=$GIT_COMMIT --build-arg AZURE_UPLOAD=$AZURE_UPLOAD \
--build-arg AZURE_STORAGE_ACCOUNT=$AZURE_STORAGE_ACCOUNT --build-arg AZURE_STORAGE_ACCESS_KEY=$AZURE_STORAGE_ACCESS_KEY \
--build-arg AZURE_STORAGE_CONNECTION_STRING=$AZURE_STORAGE_CONNECTION_STRING --build-arg AZURE_CONTAINER_NAME=$AZURE_CONTAINER_NAME \
--tag=$IMAGE_NAME .
#sudo docker login --username=$DOCKER_USER --password=$DOCKER_PASS
#sudo docker push $IMAGE_NAME
#sudo docker run -v /var/jenkins_home:/var/jenkins $IMAGE_NAME cp -r /var/cobertura /var/jenkins
#cp -r /var/jenkins_home/cobertura .
