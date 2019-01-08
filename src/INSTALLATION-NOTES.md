Installing libmpsse on Ubuntu Bionic Beaver
========

* `sudo apt install libftdi1-dev python-ftdi1 python3-ftdi1` for libftdi with Python bindings
* `sudo apt install swig` for SWIG
* `sudo apt install python-dev python3-dev` for missing `Python.h`

`make clean all`
`sudo make install`
`cd examples`
`make clean all`

TODO list
=========

* Implement Python3 support (fornow only 2.7 is present)
