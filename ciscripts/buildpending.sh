#!/bin/bash
env
curl --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://gitlab.scorum.me/api/v4/projects/6/statuses/$(git rev-parse HEAD)?state=pending
