#!/bin/bash

#curl --silent --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://$GITLAB_DOMAIN/api/v4/projects/$GITLAB_PROJECT_ID/statuses/$(git rev-parse HEAD)?state=failed

curl --silent -XPOST -H "Authorization: token $GITHUB_SECRET" https://api.github.com/repos/scorum/scorum/statuses/$(git rev-parse HEAD) -d "{
  \"state\": \"failure\",
  \"target_url\": \"${BUILD_URL}\",
  \"description\": \"Jenkins CI reports the build has failed!\",
  \"context\": \"jenkins-ci-scorum\"
}"

rm -rf $WORKSPACE/*
# make docker cleanup after itself and delete all exited containers
# sudo docker rm -v $(docker ps -a -q -f status=exited) || true
sudo docker rm -v $(docker ps --filter status=exited -q 2>/dev/null) 2>/dev/null
sudo docker rmi $(docker images --filter dangling=true -q 2>/dev/null) 2>/dev/null
sudo docker system prune -a -f
