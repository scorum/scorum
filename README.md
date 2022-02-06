# Introducing Scorum

Scorum platform has three core functions:
 - Blogging platform where authors and readers will be rewarded for creating and engaging with content
 - Statistical centers where fans can browse and authors can use Microsoft’s Power BI tool to integrate data-rich visuals into their content
 - Commission-free betting exchange where fans can place bets against each other using Scorum Coins (SCR)
Scorum’s blockchain protocol is built on the Graphene Framework and utilizes a delegated proof of stake consensus.

# Public Announcement & Discussion

The [Scorum team](https://scorumcoins.com/en-us/team) has been hard at work developing the blogging platform and the [statistics center](http://stats.scorum.com/).

Find out more as we take the project public through the following channels:
 - Get the latest updates and chat with us on [Telegram](https://telegram.me/SCORUM), [Facebook](https://www.facebook.com/SCORUM.COMMUNITY/), and [Twitter](https://twitter.com/SCORUM_en)
 - Read more about our vision on [Steemit](https://steemit.com/@scorum.community)
 - Join our [affiliate program](https://scorumcoins.com/en-us/affiliate) to get Scorum Coins for free or [apply for whitelist](https://scorumcoins.com/en-us/whitelist) to get coins with a discount

# No Support & No Warranty

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

# Blockchain consensus rules

Rather than attempt to describe the rules of the blockchain, it is up to
each individual to inspect the code to understand the consensus rules.

# Quickstart

Just want to get up and running quickly?  Try deploying a prebuilt
dockerized container.

## Dockerized Node

See [this guide](https://github.com/scorum/scorum/wiki/How-to:-run-node-using-docker-image) for detailed instructions including commands for Ubuntu  16.04 (LTS).

# Seed Nodes

A list of some seed nodes to get you started can be found in
[seed-nodes](https://github.com/scorum/scorum/wiki/Seeds). This list is embedded into default config.ini.

# Building

See [doc/building.md](doc/building.md) for detailed build instructions, including
compile-time options, and specific commands for Ubuntu 16.04 (LTS).

# System Requirements

For a full node, you need 10GB of space available. Scorumd uses a memory mapped file which currently holds 2GB of data and by default is set to use up to 10GB. It's highly recommended to run scorumd on a fast disk such as an SSD or by placing the shared memory files in a ramdisk and using the `shared-file-dir` config (or command line) option to specify where. Any CPU with decent single core performance should be sufficient.

# Main net chain_id

genesis.json hash sum: `db4007d45f04c1403a7e66a5c66b5b1cdfc2dde8b5335d1d2f116d592ca3dbb1`

# Test net chain_id

genesis.testnet.json hash sum: `d3c1f19a4947c296446583f988c43fd1a83818fabaf3454a0020198cb361ebd2`
