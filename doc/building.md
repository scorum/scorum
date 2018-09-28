# Building Scorum

## Compile-Time Options (cmake)

### CMAKE_BUILD_TYPE=[Release/Debug]

Specifies whether to build with or without optimization and without or with
the symbol table for debugging. Unless you are specifically debugging or
running tests, it is recommended to build as release.

### SCORUM_LOW_MEMORY_NODE=[OFF/ON]

Builds scorumd to be a consensus-only low memory node. Data and fields not
needed for consensus are not stored in the object database.  This option is
recommended for witnesses and seed-nodes.

### SCORUM_CLEAR_VOTES=[ON/OFF]

Clears old votes from memory that are no longer required for consensus.

### SCORUM_SKIP_BY_TX_ID=[OFF/ON]

By default this is off. Enabling will prevent the account history plugin querying transactions
by id, but saving around 65% of CPU time when reindexing. Enabling this option is a
huge gain if you do not need this functionality.

## Building under Docker

We ship a Dockerfile. This builds both common node type binaries.

    git clone https://github.com/scorum/scorum.git
    cd scorum
    git submodule update --init --recursive
    docker build -t scorum/scorum .

## Building on Ubuntu 16.04

For Ubuntu 16.04 users, after installing the right packages with `apt` Scorum
will build out of the box without further effort:

    # Required packages
    sudo apt-get install -y \
        autoconf \
        automake \
        cmake \
        g++ \
        git \
        libssl-dev \
        libicu-dev \
        libtool \
        make \
        pkg-config \
        python3 \
        python3-jinja2 \
        libboost-all-dev \
        libicu-dev

    # Optional packages (not required, but will make a nicer experience)
    sudo apt-get install -y \
        doxygen \
        libncurses5-dev \
        libreadline-dev \
        perl

    git clone https://github.com/scorum/scorum.git
    cd scorum
    git submodule update --init --recursive
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc) scorumd
    make -j$(nproc) cli_wallet
    # optional
    make install  # defaults to /usr/local

## Building on Other Platforms

- Windows and MacOS build instructions do not exist yet.

- The developers normally compile with gcc 5.4.0.

- Community members occasionally attempt to compile the code with clang, mingw,
  Intel and Microsoft compilers. These compilers may work, but the
  developers do not use them. Pull requests fixing warnings / errors from
  these compilers are accepted.
