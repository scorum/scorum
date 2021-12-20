FROM phusion/baseimage:0.9.19 as builder

ARG BRANCH_NAME
ARG GIT_COMMIT
ARG BUILD_VERSION
ARG LIVE_TESTNET

ENV LANG=en_US.UTF-8

ENV LIVE_TESTNET=${LIVE_TESTNET:-OFF}

RUN \
        apt-get update && \
        apt-get install -y \
            autoconf \
            automake \
            autotools-dev \
            bsdmainutils \
            build-essential \
            cmake \
            doxygen \
            git \
            libboost-all-dev \
            libicu-dev \
            libreadline-dev \
            libssl-dev \
            libtool \
            ncurses-dev \
            pbzip2 \
            pkg-config \
            python3 \
            python3-dev \
            python3-jinja2 \
            python3-pip \
            python-pip \
            nginx \
            fcgiwrap \
            s3cmd \
            awscli \
            jq \
            wget \
            gdb \
        && \
        apt-get clean && \
        rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
        pip3 install gcovr

ADD . /usr/local/src/scorum

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DSCORUM_LIVE_TESTNET=${LIVE_TESTNET} \
        -DSCORUM_LOW_MEMORY_NODE=OFF \
        -DSCORUM_CLEAR_VOTES=ON \
        -DSCORUM_SKIP_BY_TX_ID=ON \
        -DENABLE_COVERAGE_TESTING=ON \
        .. && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    cd /usr/local/src/scorum && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    cd /usr/local/src/scorum/build && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/scorum && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local/scorumd-default \
        -DSCORUM_LIVE_TESTNET=${LIVE_TESTNET} \
        -DSCORUM_LOW_MEMORY_NODE=ON \
        -DSCORUM_CLEAR_VOTES=ON \
        -DSCORUM_SKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    make install && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local/scorumd-full \
        -DSCORUM_LIVE_TESTNET=${LIVE_TESTNET} \
        -DSCORUM_LOW_MEMORY_NODE=OFF \
        -DSCORUM_CLEAR_VOTES=OFF \
        -DSCORUM_SKIP_BY_TX_ID=OFF \
        .. && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    make install && \
    cd / && \
    rm -rf /usr/local/src/scorum

FROM phusion/baseimage:0.9.19 as runtime

RUN \
    apt-get update && \
    apt-get install -y libicu55 libreadline6 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY --from=builder /usr/local/scorumd-full/bin/scorumd /usr/local/bin/scorumd-full
COPY --from=builder /usr/local/scorumd-full/bin/cli_wallet /usr/local/bin/cli_wallet
COPY --from=builder /usr/local/scorumd-default/bin/scorumd /usr/local/bin/scorumd

ADD contrib/config.witness.ini /etc/scorumd/config.witness.ini
ADD contrib/config.rpc.ini /etc/scorumd/config.rpc.ini
ADD contrib/seeds.mainnet.ini /etc/scorumd/seeds.mainnet.ini
ADD contrib/seeds.testnet.ini /etc/scorumd/seeds.testnet.ini
ADD contrib/scorumdentrypoint.sh /usr/local/bin/scorumdentrypoint.sh

RUN chmod +x /usr/local/bin/scorumdentrypoint.sh

# not sure that /var/lib/scorumd and /var/cache/scorumd is needed at all
RUN \
    useradd -s /bin/bash -m -d /var/lib/scorumd scorumd && \
    mkdir /var/cache/scorumd && \
    chown scorumd:scorumd -R /var/cache/scorumd && \
    chown scorumd:scorumd -R /var/lib/scorumd

ENV HOME /var/lib/scorumd
VOLUME ["/var/lib/scorumd"]

# rpc service:
EXPOSE 8001
# p2p service:
EXPOSE 2001

CMD ["/bin/bash", "/usr/local/bin/scorumdentrypoint.sh"]
