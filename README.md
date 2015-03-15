pcimaxfm
========

PCI MAX 200x FM transmitter driver and tools.

Description
-----------

This project overly ambitiously aimed to provide a free, cross-platform suite of unofficial drivers and tools for the PCI MAX 2007+ and compatible FM radio transmitter cards manufactured by [PCS Electronics](https://www.pcs-electronics.com/). Currently only a Linux 2.6+ driver module and a command line user space tool are ready. A GUI tool and drivers for other operating systems may be added if there's demand.

Getting started
---------------

1. Building sources from Git (requires GNU autotools and Linux kernel headers):
```
$ git clone https://github.com/dstien/pcimaxfm.git
$ cd pcimaxfm
$ ./autogen.sh
$ ./configure
$ make
```
2. Installing and loading Linux module (as superuser):
```
# make install
# modprobe pcimaxfm
```
3. Usage:
```
$ pcimaxctl --help
```

Releases
--------

No releases yet, get sources from the Git repository [https://github.com/dstien/pcimaxfm.git](https://github.com/dstien/pcimaxfm).

Links
-----

* [PCS Electronics](https://www.pcs-electronics.com/) ― Manufacturer.
* [PCS Electronics forum](https://www.pcs-electronics.com/phpBB2/) ― Official support and discussion forum.
* [pcimax-ctl](https://github.com/koradlow/pcimax-ctl) ― Linux driver for the current PCI MAX 3000+ card.
* [PCMAX](http://fscked.org/projects/minihax/abast-mateys-pcmax-i2c-bow) ― Linux 2.4 driver for legacy PC MAX cards.
