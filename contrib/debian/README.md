
Debian
====================
This directory contains files used to package bitcoingreend/bitcoingreen-qt
for Debian-based Linux systems. If you compile bitcoingreend/bitcoingreen-qt yourself, there are some useful files here.

## bitcoingreen: URI support ##


bitcoingreen-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitcoingreen-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitcoingreenqt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoingreen128.png` to `/usr/share/pixmaps`

bitcoingreen-qt.protocol (KDE)

