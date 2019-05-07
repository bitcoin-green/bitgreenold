
Debian
====================
This directory contains files used to package bitgreend/bitgreen-qt
for Debian-based Linux systems. If you compile bitgreend/bitgreen-qt yourself, there are some useful files here.

## bitgreen: URI support ##


bitgreen-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitgreen-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitgreenqt binary to `/usr/bin`
and the `../../share/pixmaps/bitgreen128.png` to `/usr/share/pixmaps`

bitgreen-qt.protocol (KDE)

