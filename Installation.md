# Prerequisites #

## Basic Prerequisites ##
Libmpsse depends on the [libftdi library](http://www.intra2net.com/en/developer/libftdi/). This may be installed from source, or via your package manager of choice:

```
$ sudo apt-get install libfdti-dev
```

## Python Prerequisites ##

If you wish to install the Python wrappers as well, you will need [swig](http://www.swig.org/) and the Python development package:

```
$ sudo apt-get install swig python-dev
```

# Installation #

## Installing Everything ##

To build and install both the libmpsse library and the Python wrapper module:

```
$ ./configure && make && sudo make install
```

## Installing Without Python ##

If you do no wish to install the Python wrapper module, use the configure script's --disable-python option:

```
$ ./configure --disable-python && make && sudo make install
```