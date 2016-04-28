#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpsse.h>
#include <time.h>
#include <string.h>

#define FOUT	"trace.bin"		// Output file

#define NUM_FRAMES     160
/* Frame size: 12888 bytes, or 8 times 1.5 KiB.  */
#define TRANSFER_SIZE (1536 * 8)

#ifdef SWIGPYTHON
  #warning SWIGPYTHON enabled!
  #define DATA_POINTER(ARG) (ARG).data
  #define DATA_SIZE(ARG) (ARG).size
#else
  #define DATA_POINTER(ARG) (ARG)
  #define DATA_SIZE(ARG) (TRANSFER_SIZE)
#endif

/* Append current timeval to character buffer at PTR.  */
#define WRITE_TIMESTAMP_TO_BUFFER(PTR) \
do { \
  gettimeofday ((struct timeval *) (PTR), NULL); \
  (PTR) += sizeof (struct timeval); \
} while (0)

/* Sequence used to check correct MOSI reception at the STM32F4 side.  */
unsigned char tx_buffer[TRANSFER_SIZE] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe };

/* Reception buffer and current data pointer.  */
unsigned char *rx_buffer = NULL, *data_ptr;

int main(int argc, char **argv)
{
	FILE *fp = NULL;
#ifdef SWIGPYTHON
	swig_string_data data = { 0, 0};
#else
	unsigned char *data = NULL;
#endif
	int retval = EXIT_FAILURE, dumped_bytes = 0;
	unsigned int num_frames = NUM_FRAMES;
	struct mpsse_context *flash = NULL;
	char *filename = FOUT;

	/* If the first argument is given, it is the number of frames.
	   Any negative value will be converted to unsigned number,
	   resulting in a very large number (-1 ==> UINT_MAX).  */
	if (argc > 1)
	{
		int raw_frame_count;
		int success = sscanf (argv[1], "%d", &raw_frame_count);
		if (!success)
		{
			fprintf (stderr, "Usage: %s number_of_frames [filename]\n", argv[0]);
			exit (1);
		}

		num_frames = (unsigned int) raw_frame_count;
	}

	/* If there's a second argument, it's the output file name.  */
	if (argc > 2)
		filename = argv[2];

	/* Rx buffer is sized to hold 16/32 bytes of host-side frame start
	   and end TSes (size depends on whether the OS is 32 or 64 bits.)
	   At the very beginning, it holds the log start timestamp.  */
	if ((rx_buffer = malloc (TRANSFER_SIZE + (2 * sizeof (struct timeval)))) == NULL)
	{
		perror (argv[0]);
		exit (1);
	}
	else
		data_ptr = rx_buffer;

	/* Try to open the file.  If the opening, bail out.  */
	fp = fopen(filename, "wb");
	if (!fp)
	{
		perror (argv[0]);
		exit (1);
	}

	/* Write timestamp of FTx232H init to rx_buffer, then write it
	   to output file.  */
	WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
	fwrite (rx_buffer, sizeof (struct timeval), 1, fp);

	if((flash = MPSSE(SPI0, 15000000, MSB)) != NULL && flash->open)
	{
		PurgeRxBuffer (flash);

		printf("%s initialized at %d Hz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));

		/* Pull down #CS.  */
		Start(flash);

		/* Write timestamp of pulling down #CS.  */
		data_ptr = rx_buffer;
		WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
		dumped_bytes += sizeof (struct timeval);

		/* Perform the FTDI transfer.  */
		data = Transfer(flash, tx_buffer, TRANSFER_SIZE);

		/* Write timestamp of end-of-transfer immediately after
		   the timestamp of start-of-transfer and write both TSes
		   to the output file.  */
		WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
		fwrite (rx_buffer, sizeof (struct timeval), 2, fp);

		dumped_bytes += sizeof (struct timeval);

		if (DATA_POINTER(data))
		{
			int i;

			fwrite(DATA_POINTER (data), 1, DATA_SIZE (data), fp);
			data_ptr += DATA_SIZE (data);

			free(DATA_POINTER (data));
			dumped_bytes += DATA_SIZE (data);

			Stop(flash);

			/* Exchange next 'num_frames' TRANSFER_SIZE-byte bursts.  */
			for (i = 0; i < num_frames - 1; i++)
			{
				/* Pull down #CS.  */
				Start (flash);

				/* Write timestamp of pulling down #CS.  */
				data_ptr = rx_buffer;
				WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
				dumped_bytes += sizeof (struct timeval);

				/* Perform the FTDI transfer.  */
				data = Transfer (flash, tx_buffer, TRANSFER_SIZE);

				/* Write timestamp of end-of-transfer to
				   buffer and flush both TSes to file.  */
				WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
				dumped_bytes += sizeof (struct timeval);
				fwrite (rx_buffer, sizeof (struct timeval), 2, fp);

				if (DATA_POINTER (data))
				{
					fwrite (DATA_POINTER (data), 1, DATA_SIZE (data), fp);
					dumped_bytes += DATA_SIZE (data);
					free (DATA_POINTER (data));
					Stop (flash);
				}
				else
					break;
			}
			fclose (fp);
			
			printf ("Dumped %d bytes to %s\n", dumped_bytes, filename);
			retval = EXIT_SUCCESS;
		}
		else
		{
			fprintf (stderr, "*** Cannot get data from FTDI chip!\n");

			/* Clean up: close file...  */
			fclose (fp);

			/* Pull up #CS.  */
			Stop (flash);

			/* Turn off the FTDI.  */
			Close (flash);
			exit (1);
		}
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(flash));
	}

	if (flash)
		Close(flash);

	return retval;
}
