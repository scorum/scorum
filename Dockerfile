FROM phusion/baseimage:0.9.19

#ARG SCORUMD_BLOCKCHAIN=https://example.com/scorumd-blockchain.tbz2

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
    git submodule update --init --recursive && \
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
    ./tests/chain_test && \
    ./tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    cd /usr/local/src/scorum && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    git submodule update --init --recursive && \
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
    ./tests/chain_test && \
    ./tests/wallet_tests && \
    mkdir -p /var/cobertura && \
    gcovr --object-directory="../" --root=../ --xml-pretty --gcov-exclude=".*tests.*" --gcov-exclude=".*fc.*" --gcov-exclude=".*app*" --gcov-exclude=".*net*" --gcov-exclude=".*plugins*" --gcov-exclude=".*schema*" --gcov-exclude=".*time*" --gcov-exclude=".*utilities*" --gcov-exclude=".*wallet*" --gcov-exclude=".*programs*" --output="/var/cobertura/coverage.xml" && \
    cd /usr/local/src/scorum && \
    rm -rf /usr/local/src/scorum/build

RUN \
    cd /usr/local/src/scorum && \
    git submodule update --init --recursive && \
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
    ./tests/chain_test && \
    ./tests/wallet_tests && \
    ./programs/util/test_fixed_string && \
    make install && \
    cd .. && \
    ( /usr/local/scorumd-default/bin/scorumd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/scorumdversion && \
    cat /etc/scorumdversion && \
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
ADD contrib/config-for-docker.ini /etc/scorumd/config.ini
ADD contrib/fullnode.config.ini /etc/scorumd/fullnode.config.ini

# add normal startup script that starts via sv
ADD contrib/scorumd.run /usr/local/bin/scorum-sv-run.sh
RUN chmod +x /usr/local/bin/scorum-sv-run.sh

# add nginx templates
ADD contrib/scorumd.nginx.conf /etc/nginx/scorumd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add PaaS startup script and service script
ADD contrib/startpaasscorumd.sh /usr/local/bin/startpaasscorumd.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/startpaasscorumd.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/scorumdentrypoint.sh /usr/local/bin/scorumdentrypoint.sh
RUN chmod +x /usr/local/bin/scorumdentrypoint.sh
CMD /usr/local/bin/scorumdentrypoint.sh
