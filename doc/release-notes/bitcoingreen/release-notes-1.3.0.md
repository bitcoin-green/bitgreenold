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

- Switched from OpenSSL to SECP256K1 for consensus.
  This will make the staking behaviour of the software a lot more stable. This also solves the issue of the wallet that stops staking after a while, which a couple of users reported.

- Reduced logging to specific categories.
  Stake modifier is now saved to log only when the printstakemodifier parameter is used.

- Updated masternode activation and broadcast.
  Masternode activation and broadcast to other nodes has been improved, new checks have been added also to masternode alive checks.

- Updated RPC masternode.
  Wallet locking is handled better on masternode aliases and listmasternodes now displays the correct example code.

- Fixed masternode/budget system for testnet.
  The budget system is now also working on the testnet. This has helped us test a couple of scenarios, and will help us add new features in the future.

- Optimized image assets.
  All images assets have been passed to a script to reduce their size and weird notifications logged on debug.log due to a Qt problem with PNG data.

- Updated copyrights.
  Copyrights headers have been reviewed, added missing ones and updated to current year.

- Added QoS for IPv6.
  QoS tc.sh shell script now supports IPv6 networking mangling.

- Refactored BIP38 layout.
  Updated labels to improve the usability of the BIP38 feature and added a paste button from clipboard.

- Improved wallet unlocking.
  Wallet unlocking now works based on context where it is used, the option to "stake only" is available and automatically checked only when unlocked from the menu.

- Added label saving to multisend.
  Multisend now supports address labels when saving to the addressbook database.

- Fixed dynamic screen elements issues.
  Layout updates to issue bad UX behaviors.

- Improved send coins fee and SwiftTX system.
  SwiftTX now have more highlight and the fee system has been optimized to reflect the current network transactions load.

- Removed macOS growl support.
  Since Growl has been deprecated on older macOS versions and we currently support 10.10+, growl support has been removed in favor of the newest Apple notification system.

- Removed potentional memory leak and updated multisend code.
  Multisend stop iterating if we have sent out all the MultiSend(s).

- Removed fallback for Qt versions lower than 5.
  Removing Qt4 fallbacks to simplify maintenance and adding new GUI functionality.

- Overview page overhaul.
  GUI elements have been optimized and we've added a new option (available under settings) to hide zero balances.

- Implemented Locked funds on overview.
  Masternode collaterals or locked inputs are now shown as "Locked" on the overview page, this improves the balances overview UX.

- Removed unused sporks.
  Inactive or pointless sporks have been removed to simplify code maintenance.

- Refactor ConnectBlock to prevent chain corruptions.
  Removed a method that could cause the wallet to corrupt on force close or under other special conditions, removed the auto-repair db on init and refactored the staking checks to improve wallet stability.

- Show warnings for "dangerous" Debug Console commands.
  In order to prevent scam attempts, a message warning is returned the first time a dumpprivkey or dumpwallet RPC command is issued on console.

- Added watchonly parameter to listunspent RPC command.
  A new optional parameter on listunspent RPC command has been added to filter the unspent outputs per regular/watchonly or all transactions.

- Fixed wtxOrdered on new transactions.
  This addresses an issue where new incoming transactions aren't recorded, and subsequently, not returned with `listtransactions` in the same session.

Credits
=======

Thanks to everyone who directly contributed to this release:
- bluepp2
- donverde
- esejuli94
- konez2k