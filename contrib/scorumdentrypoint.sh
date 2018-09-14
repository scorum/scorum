#!/bin/bash

ulimit -c unlimited

# Additional env variable for binary
CONF_TYPE=$(echo $NODE | tr [:upper:] [:lower:])
if [[ $CONF_TYPE = "rpc" ]]; then
    SCORUMD="/usr/local/scorumd-full/bin/scorumd"
else
    SCORUMD="/usr/local/scorumd-default/bin/scorumd"
fi

chown -R scorumd:scorumd $HOME

if [ ! -f "${HOME}/config.ini" ]; then
    echo "Config ${HOME}/config.ini file not found. Using default"
    if [[ $CONF_TYPE = "rpc" ]]; then
        cp /etc/scorumd/config.ini.rpc "${HOME}/config.ini"
    elif [[ $CONF_TYPE = "seed" ]]; then
        cp /etc/scorumd/config.ini.witness "${HOME}/config.ini"
    elif [[ $CONF_TYPE = "witness" ]]; then
        cp /etc/scorumd/config.ini.witness "${HOME}/config.ini"
    else
        # witness is default configuration
        cp /etc/scorumd/config.ini.witness "${HOME}/config.ini"
    fi
    chown scorumd:scorumd "${HOME}/config.ini"

    if [ $LIVE_TESTNET = ON ]; then
        cat /etc/scorumd/seeds.ini.testnet >> "${HOME}/config.ini" || exit 1
    else
        cat /etc/scorumd/seeds.ini.mainnet >> "${HOME}/config.ini" || exit 1
    fi
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
