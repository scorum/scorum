#/bin/bash

#curl --silent --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://$GITLAB_DOMAIN/api/v4/projects/$GITLAB_PROJECT_ID/statuses/$(git rev-parse HEAD)?state=success

curl --silent -XPOST -H "Authorization: token $GITHUB_SECRET" https://api.github.com/repos/scorum/scorum/statuses/$(git rev-parse HEAD) -d "{
  \"state\": \"success\",
  \"target_url\": \"${BUILD_URL}\",
  \"description\": \"Jenkins CI reports build succeeded!!\",
  \"context\": \"jenkins-ci-scorum\"
}"

if [ "$DOCKER_HUB" = true ]; then
  docker login --username=$DOCKER_USR --password=$DOCKER_PWD
  if [[ -z "${BUILD_VERSION}"  ]]; then
    FULL_IMAGE_NAME="scorum/$UPLOAD_PATH:${BRANCH_NAME}.${GIT_COMMIT:0:7}"
    docker push "$FULL_IMAGE_NAME"
  else
    FULL_IMAGE_NAME="scorum/$UPLOAD_PATH:$BUILD_VERSION.${GIT_COMMIT:0:7}"
    docker tag "scorum/$UPLOAD_PATH:$BRANCH_NAME.${GIT_COMMIT:0:7}" "$FULL_IMAGE_NAME"
    docker push "$FULL_IMAGE_NAME"
  fi
else
  echo "Docker hub upload disabled"
fi

rm -rf $WORKSPACE/*
# make docker cleanup after itself and delete all exited containers
# sudo docker rm -v $(docker ps -a -q -f status=exited) || true
sudo docker rm -v $(docker ps --filter status=exited -q 2>/dev/null) 2>/dev/null
sudo docker rmi $(docker images --filter dangling=true -q 2>/dev/null) 2>/dev/null
