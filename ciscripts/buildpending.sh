#!/bin/bash
# curl --silent --request POST -H "PRIVATE-TOKEN: $GITLAB_TOKEN" https://$GITLAB_DOMAIN/api/v4/projects/$GITLAB_PROJECT_ID/statuses/$(git rev-parse HEAD)?state=pending

curl --silent -XPOST -H "Authorization: token $GITHUB_SECRET" https://api.github.com/repos/scorum/scorum/statuses/$(git rev-parse HEAD) -d "{
  \"state\": \"pending\",
  \"target_url\": \"${BUILD_URL}\",
  \"description\": \"The build is now pending in Jenkins CI!\",
  \"context\": \"jenkins-ci-scorum\"
}"
