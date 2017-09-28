#!/bin/bash
curl --silent --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://gitlab.scorum.me/api/v4/projects/3/statuses/$(git rev-parse HEAD)?state=pending
