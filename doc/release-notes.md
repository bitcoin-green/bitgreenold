(note: this is a temporary file, to be added-to by anybody, and moved to release-notes at release time)

Bitcoin Green Core version *version* is now available from:

  <https://github.com/bitcoingreen/bitcoingreen/releases>

This is a new major version release, including various bug fixes and
performance improvements, as well as updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoingreen/bitcoingreen/issues>

Mandatory Update
==============


How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over /Applications/BitcoinGreen-Qt (on Mac) or bitcoingreend/bitcoingreen-qt (on Linux).

Compatibility
==============

Bitcoin Green Core is extensively tested on multiple operating systems using
Linux, macOS 10.10+, and Windows 7 and later.

Bitcoin Green Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable Changes
===============

Random-cookie RPC authentication
---------------------------------

When no `-rpcpassword` is specified, the daemon now uses a special 'cookie'
file for authentication. This file is generated with random content when the
daemon starts, and deleted when it exits. Its contents are used as
authentication token. Read access to this file controls who can access through
RPC. By default it is stored in the data directory but its location can be
overridden with the option `-rpccookiefile`.

This is similar to Tor's CookieAuthentication: see
https://www.torproject.org/docs/tor-manual.html.en

This allows running bitcoingreend without having to do any manual configuration.


*version* Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### Broad Features
### P2P Protocol and Network Code
### GUI
### Miscellaneous

Credits
=======

Thanks to everyone who directly contributed to this release:

