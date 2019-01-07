LDFLAGS=-L/usr/local/lib -lmpsse
CFLAGS=-O2 -g $(shell pkg-config --cflags libftdi1)

all: spiflash mageec-spifetch spitest-ftdi spi-continuous spitest-rxonly spiflashfast i2ceeprom ds1305 gpio bitbang

spiflash: spiflash.c
	$(CC) $(CFLAGS) spiflash.c -o spiflash $(LDFLAGS)

mageec-spifetch: mageec-spifetch.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

spitest-ftdi: spitest-ftdi.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

spi-continuous: spi-continuous.c
	$(CC) $(CFLAGS) spi-continuous.c -o spi-continuous $(LDFLAGS)

spitest-rxonly: spitest-rxonly.c
	$(CC) $(CFLAGS) spitest-rxonly.c -o spitest-rxonly $(LDFLAGS)

spiflashfast: spiflashfast.c
	$(CC) $(CFLAGS) spiflashfast.c -o spiflashfast $(LDFLAGS)

i2ceeprom: i2ceeprom.c
	$(CC) $(CFLAGS) i2ceeprom.c -o i2ceeprom $(LDFLAGS)

ds1305: ds1305.c
	$(CC) $(CFLAGS) ds1305.c -o ds1305 $(LDFLAGS)

gpio: gpio.c
	$(CC) $(CFLAGS) gpio.c -o gpio $(LDFLAGS)

bitbang: bitbang.c
	$(CC) $(CFLAGS) bitbang.c -o bitbang $(LDFLAGS)

clean:
	rm -f *.dSYM
	rm -f bitbang gpio ds1305 i2ceeprom spiflash spiflashfast mageec-spifetch spitest-ftdi spitest-rxonly spi-continuous

distclean: clean
