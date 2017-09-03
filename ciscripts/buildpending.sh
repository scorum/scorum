#!/bin/bash
echo "TOKEN: $GITLAB_TOKEN"
curl --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://gitlab.scorum.me/api/v4/projects/6/statuses/$(git rev-parse HEAD)?state=pending
