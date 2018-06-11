#!/bin/bash

ulimit -c unlimited

# Additional env variable for binary
if [[ `echo $NODE | tr [:upper:] [:lower:]` =  "full" ]]; then
    SCORUMD="/usr/local/scorumd-full/bin/scorumd"
else
    SCORUMD="/usr/local/scorumd-default/bin/scorumd"
fi

chown -R scorumd:scorumd $HOME

if [ ! -f "${HOME}/config.ini" ]; then
    echo "Config ${HOME}/config.ini file not found. Using default"
    cp /etc/scorumd/fullnode.config.ini "${HOME}/config.ini"
    chown scorumd:scorumd "${HOME}/config.ini"
fi

# without --data-dir it uses cwd as datadir(!)
# who knows what else it dumps into current dir
cd $HOME

echo "Starting " $SCORUMD

# slow down restart loop if flapping
sleep 1

exec chpst -u scorumd \
    $SCORUMD \
        --config-file="${HOME}/config.ini" \
        --data-dir="${HOME}" \
        2>&1
