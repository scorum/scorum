#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start scorumd traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/scorumd
  cp /usr/local/bin/scorum-sv-run.sh /etc/service/scorumd/run
  chmod +x /etc/service/scorumd/run
  runsv /etc/service/scorumd
else
  /usr/local/bin/startpaasscorumd.sh
fi
