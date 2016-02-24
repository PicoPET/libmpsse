Libmpsse is a library for interfacing with SPI/I2C devices via FTDI's FT-2232 family of USB chips. Based around the libftdi library, it is written in C and includes a Python wrapper courtesy of swig.

Key features of libmpsse are:

  * SPI and I2C master mode operations
  * Full support for SPI modes 0 and 2 (partial support for modes 1 and 3)
  * Simple API for both C and Python
  * Control of up to 12 GPIO pins (4 input/output, 8 others output only)
  * Supports full bitbang mode
  * Clock speeds of up to 30MHZ
  * Data transfer speeds of better than 1MB/s are possible

Libmpsse supports FTDI chips that contain an MPSSE engine, such as the FT-2232H and the FT-232H.

Currently libmpsse is supported on Linux and Mac OSX.