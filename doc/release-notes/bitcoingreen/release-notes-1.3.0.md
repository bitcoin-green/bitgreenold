Bitcoin Green Core version 1.3.0 is now available from:

  <https://github.com/bitcoingreen/bitcoingreen/releases>

This is a new minor version release, including various bug fixes and improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoingreen/bitcoingreen/issues>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/BitcoinGreen-Qt (on Mac) or bitcoingreend/bitcoingreen-qt (on Linux).

Compatibility
==============

Bitcoin Green Core is extensively tested on multiple operating systems using
Linux, macOS 10.10+, and Windows 7 and later.

Change log
==========

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates.

- Switched from OpenSSL to SECP256K1 for consensus
- Reduced logging to specific categories
- Updated masternode activation and broadcast
- Updated RPC masternode
- Fixed masternode/budget system for testnet
- Optimized image assets
- Updated copyrights
- Added QoS for IPV6
- Refactored BIP38 layout
- Improved wallet unlocking
- Added label saving to multisend
- Fixed dynamic screen elements issues
- Improved send coins fee and SwiftTX system
- Removed macOS growl support
- Removed potentional memory leak and updated multisend code
- Removed fallback for Qt versions lower than 5
- Overview page overhaul
- Implemented Locked funds on overview
- Removed unused sporks
- Refactor ConnectBlock to prevent chain corruptions

Credits
=======

Thanks to everyone who directly contributed to this release:
- bluepp2
- donverde
- esejuli94
- konez2k