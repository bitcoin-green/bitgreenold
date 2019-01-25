Bitcoin Green Core
==================

Setup
---------------------
[Bitcoin Green Core](http://savebitcoin.io) is the original Bitcoin Green client and it builds the backbone of the network. However, it downloads and stores the entire history of Bitcoin Green transactions; depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. Thankfully you only have to do this once.

Running
---------------------
The following are some helpful notes on how to run Bitcoin Green Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/bitcoingreen-qt` (GUI) or
- `bin/bitcoingreend` (headless)

### Windows

Unpack the files into a directory, and then run bitcoingreen-qt.exe.

### macOS

Drag BitcoinGreen-Qt to your applications folder, and then run BitcoinGreen-Qt.

### Need Help?

* Join our Discord server [Discord Server](https://discord.savebitcoin.io)

Building
---------------------
The following are developer notes on how to build Bitcoin Green Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Bitcoin Green repo's [root README](https://github.com/bitcoingreen/bitcoingreen/blob/master/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/bitcoin/doxygen/) ***TODO***
- [Translation Process](translation_process.md)
- [Unit Tests](unit-tests.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Dnsseed Policy](dnsseed-policy.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
