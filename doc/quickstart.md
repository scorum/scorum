Quickstart
----------

### Get current scorumd
Use docker to build from Dockerfile or download from scorum dockerhub (for instance scorum/release).
Default docker image has scorumd-default and scorumd-full executables for low memory and full memory
nodes respectively.

#### Low memory node?
Above runs low memory node, which is suitable for:
- seed nodes
- witness nodes
- exchanges, etc.

### Configure for your use case
#### Full API node
You can Use `contrib/fullnode.config.ini` as a base for your `config.ini` file. 
Do not forget to set seed-node options. For instance:

```ini
seed-node = seed1-testnet.scorum.com:2001
seed-node = seed1-testnet.scorum.com:2002
seed-node = seed1-testnet.scorum.com:2003
seed-node = seed1-testnet.scorum.com:2004
```

#### Exchanges
Use low memory node.

Also make sure that your `config.ini` contains:
```
enable-plugin = blockchain_history
public-api = database_api login_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```
Do not add other APIs or plugins unless you know what you are doing.

### Resources usage

Please make sure that you have enough resources available.
Check `shared-file-size =` in your `config.ini` to reflect your needs.
Set it to at least 25% more than current size.

Provided values are expected to grow significantly over time.

Blockchain data takes over **16GB** of storage space.

#### Full node
Shared memory file for full node uses over **65GB**

#### Exchange node
Shared memory file for exchange node users over **16GB**
(tracked history for single account)

#### Seed node
Shared memory file for seed node uses over **5.5GB**

#### Other use cases
Shared memory file size varies, depends on your specific configuration but it is expected to be somewhere between "seed node" and "full node" usage.
