#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpsse.h>
#include <time.h>
#include <string.h>
#include "../support.h"
/* #include "simplelink.h" */

/* Sync word, host to device, WRITE mode.  Least-significant hex digit
   of byte #3 can be any value in the range 4..7.  */
const unsigned char SYNCWORD_HOST2DEV_WRITE[4] = { 0x43, 0x21, 0x12, 0x34 };

/* Sync word, host to device, READ mode.  Least-significant hex digit
   of byte #3 can be any value in the range 0xc..0xf according to spec
   at http://processors.wiki.ti.com/index.php/CC31xx_Host_Interface,
   but the Windows+FTDI driver uses a 0x8.  */
const unsigned char SYNCWORD_HOST2DEV_READ[4] = { 0x87, 0x65, 0x56, 0x78 };

/* Sync word, device to host.  Least-significant hex digit of byte #3
   can be ny value in the range 0xc..0xf.  */
const unsigned char SYNCWORD_DEV2HOST[4] = { 0xab, 0xcd, 0xdc, 0xbc };

/* Set up I/Os: Configure GPIOL0 to be an output.  */
void setup_IOs (struct mpsse_context *ti_iface)
{
  if (ti_iface)
    ti_iface->tris |= GPIO0;
  else
    {
      fprintf (stderr, "Could not enable output on pin GPIOL0\n");
      exit (2);
    }
}

/* Wait for TI event.  */
const unsigned char WAIT_FOR_HOSTINT_RAISE[] = { 0x88 };

void wait_for_TI_event (struct mpsse_context *ti_iface)
{
  unsigned char state;

  WaitForGpioL1High (ti_iface, &state);
  
  if ((state & GPIO1) != GPIO1)
    fprintf (stderr, "*** Did not get the correct GPIO state after wait (got 0x%02x)\n",
	     state);
}

/* Run the test:
   - open the MPSSE block
   - configure the I/Os: set nHIB as output
   - pull nHIB down
   - sleep 100ms
   - pull nHIB up
   - wait for host interrupt to ba raised
     - send [ Wait for GPIOl1 high (0x88) ] to FTDI
     - read GPIOs (will block until interrupt is raised)
   - start the SPI
   - send sync word (H2D READ) [ Start ; Write H2DR sync word ; Stop ]
   - stop the SPI
   - wait for host interrupt to be cleared
     - send [ Wait for GPIOL1 low (0x89) ] to FTDI
     - read GPIOs (will block until interrupt is cleared)
   - read the data from the device
     - Start SPI
     - read data 32 bytes (or read header; decide on size; read payload );
       data should start with D2H sync word
     - Stop SPI
   - process reply; maybe
     - Start
     - send H2DW command with payload
     - Stop
     - EXAMPLE: send connect-to-predefined-profile command
   - wait for interrupt
   - send sync word H2DR
   - wait for interrup to be cleared
   - process reply
   - ...
   - close the MPSSE block.  */
int main (int argc, char **argv)
{
  struct mpsse_context *cc3100_iface = NULL;
  uint8_t *data = NULL;

  /* Attempt to open the MPSSE engine in GPIO mode.  */
  cc3100_iface = MPSSE (BITBANG, 15000000, MSB);

  /* Complain and exit if the attempt failed.  */
  if (!cc3100_iface || !cc3100_iface->open)
    {
      fprintf (stderr, "Failed to open the FTDI interface to CC3100, exiting...\n");
      exit (1);
    }

#if 0 /* Needs rework.  */
  /* Configure the nHIB line (GPIOL0) as output.  */
  SetGpioPinDirection (cc3100_iface, GPIOL0, OUTPUT);
  SetGpioPinDirection (cc3100_iface, GPIOL1, INPUT);
#endif
  
  /* Pull nHIB line down.  */
  if (gpio_write (cc3100_iface, 4, 0) != MPSSE_OK)
	{
		fprintf (stderr, "Could not CLEAR nHIB pin!\n");
	}

  /* Sleep 250 ms.  */
  usleep (250 * 1000);

  /* Purge the FTDI Rx buffer to get rid of any stale data.  */
  PurgeRxBuffer (cc3100_iface);
  
  /* Pull nHIB line up to pull the chip out of hibernation.  */
  if (gpio_write (cc3100_iface, 4, 1) != MPSSE_OK)
	{
		fprintf (stderr, "Could not SET nHIB pin!\n");
	}

  /* Wait for hostINT (GPIOL1) to be pulled up.

     This instructs the FTDI chip to block the execution of any subsequent
     commands until the GPIOL1 line becomes high.  */
  wait_for_TI_event (cc3100_iface);

  /* Wait 10ms before changing MPSSE state.  */
  usleep (2 * 1000);
	
  /* ACHTUNG: non-compliant use of the MPSSE interface...  */
  cc3100_iface->mode = SPI0;
  SetMode (cc3100_iface, MSB);

  /* Let the MPSSE set up properly: assume 10 ms is enough.  */
  usleep (10 * 1000);

  /* Start the SPI communication.  */
  Start (cc3100_iface);

  /* Receive 32 bytes of data.  */
  data = Read (cc3100_iface, 32);

  /* Sopt the SPI communication.  */
  Stop (cc3100_iface);

  /* Close the interface.  */
  Close (cc3100_iface);

  return 0;
}
