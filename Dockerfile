FROM phusion/baseimage:0.9.19

#ARG SCORUMD_BLOCKCHAIN=https://example.com/scorumd-blockchain.tbz2

ARG BRANCH_NAME
ARG GIT_COMMIT
ARG AZURE_UPLOAD
ARG AZURE_STORAGE_ACCOUNT
ARG AZURE_STORAGE_ACCESS_KEY
ARG AZURE_STORAGE_CONNECTION_STRING
ARG UPLOAD_PATH
ARG BUILD_VERSION

ENV LANG=en_US.UTF-8

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
        pip3 install gcovr && \
        pip install azure-cli

ADD . /usr/local/src/scorum

RUN git rev-parse --short HEAD

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SCORUM_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/scorum && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE_TESTING=ON \
        -DBUILD_SCORUM_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/scorum && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/scorumd-default \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_SCORUM_TESTNET=OFF \
        .. \
    && \
    make -j$(nproc) && \
    ./libraries/chainbase/test/chainbase_test && \
    ./tests/utests/utests && \
    ./tests/chain_tests/chain_tests && \
    ./tests/wallet_tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    make install && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/scorumd-full \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_SCORUM_TESTNET=OFF \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/scorumd-full/bin/scorumd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/scorumdversion && \
    cat /etc/scorumdversion && \
    rm -rf /usr/local/src/scorum

RUN \
        apt-get remove -y \
            automake \
            autotools-dev \
            bsdmainutils \
            build-essential \
            cmake \
            doxygen \
            dpkg-dev \
            git \
            libboost-all-dev \
            libc6-dev \
            libexpat1-dev \
            libgcc-5-dev \
            libhwloc-dev \
            libibverbs-dev \
            libicu-dev \
            libltdl-dev \
            libncurses5-dev \
            libnuma-dev \
            libopenmpi-dev \
            libpython-dev \
            libpython2.7-dev \
            libreadline-dev \
            libreadline6-dev \
            libssl-dev \
            libstdc++-5-dev \
            libtinfo-dev \
            libtool \
            linux-libc-dev \
            m4 \
            make \
            manpages \
            manpages-dev \
            mpi-default-dev \
            python-dev \
            python2.7-dev \
            python3-dev \
        && \
        apt-get autoremove -y && \
        rm -rf \
            /var/lib/apt/lists/* \
            /tmp/* \
            /var/tmp/* \
            /var/cache/* \
            /usr/include \
            /usr/local/include

RUN useradd -s /bin/bash -m -d /var/lib/scorumd scorumd

RUN mkdir /var/cache/scorumd && \
          chown scorumd:scorumd -R /var/cache/scorumd

# add blockchain cache to image
#ADD $SCORUMD_BLOCKCHAIN /var/cache/scorumd/blocks.tbz2

ENV HOME /var/lib/scorumd
RUN chown scorumd:scorumd -R /var/lib/scorumd

VOLUME ["/var/lib/scorumd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
ADD doc/seednodes.txt /etc/scorumd/seednodes.txt

# the following adds lots of logging info to stdout
ADD contrib/fullnode.config.ini /etc/scorumd/fullnode.config.ini

# add normal startup script that start via runsv
RUN mkdir /etc/sv/scorumd
RUN mkdir /etc/sv/scorumd/log
ADD contrib/runsv/scorumd.run /etc/sv/scorumd/run
ADD contrib/runsv/scorumd-log.run /etc/sv/scorumd/log/run
ADD contrib/runsv/scorumd-log.config /etc/sv/scorumd/log/config
RUN chmod +x /etc/sv/scorumd/run /etc/sv/scorumd/log/run

# add nginx templates
ADD contrib/scorumd.nginx.conf /etc/nginx/scorumd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add healthcheck script
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# upload archive to azure
ADD contrib/azure_upload.sh /usr/local/bin/azure_upload.sh
RUN chmod +x /usr/local/bin/azure_upload.sh
RUN /usr/local/bin/azure_upload.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/scorumdentrypoint.sh /usr/local/bin/scorumdentrypoint.sh
RUN chmod +x /usr/local/bin/scorumdentrypoint.sh
CMD /usr/local/bin/scorumdentrypoint.sh
