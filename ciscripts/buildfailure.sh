#!/bin/bash
curl --silent --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://gitlab.scorum.me/api/v4/projects/6/statuses/$(git rev-parse HEAD)?state=failed
rm -rf $WORKSPACE/*
# make docker cleanup after itself and delete all exited containers
sudo docker rm -v $(docker ps -a -q -f status=exited) || true
