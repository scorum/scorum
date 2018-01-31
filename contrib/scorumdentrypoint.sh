#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

mkdir -p /etc/service/scorumd
cp /usr/local/bin/scorumd.run /etc/service/scorumd/run
chmod +x /etc/service/scorumd/run

runsv /etc/service/scorumd
