Installing libmpsse on Ubuntu Bionic Beaver
========

Prerequisites
-------------

* `sudo apt install libftdi1-dev python-ftdi1 python3-ftdi1` for libftdi with Python bindings
* `sudo apt install swig` for SWIG
* `sudo apt install python-dev python3-dev` for missing `Python.h`

Installation
------------

`./configure`
`make clean`
`make all`
`sudo make install`
`cd examples`
`make clean all`

TODO list
=========

* Implement Python3 support (fornow only 2.7 is present)

Installing libmpsse on Fedora Core 29
====================

Prerequisites
-------------

* `sudo dnf install libftdi-devel python2-libftdi python3-libftdi`
* `sudo dnf install swig`

Installation
------------

`./configure`
`make clean`
`make all`
`sudo make install`
