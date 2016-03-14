#define SWIGPYTHON

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpsse.h>
#include <time.h>
#include <string.h>

#define FOUT	"trace.bin"		// Output file

#ifdef SWIGPYTHON
  #warning SWIGPYTHON enabled!
#endif

#define NUM_FRAMES     160
#define TRANSFER_SIZE (1536 * 8)

/* Append current timeval to buffer at PTR.  */
#define WRITE_TIMESTAMP_TO_BUFFER(PTR) \
do { \
  gettimeofday ((struct timeval *) (PTR), NULL); \
  (PTR) = ((unsigned char *) (PTR)) + sizeof (struct timeval); \
} while (0)

unsigned char tx_buffer[TRANSFER_SIZE] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe };
unsigned char *rx_buffer = NULL, *data_ptr;

int main(int argc, char **argv)
{
	FILE *fp = NULL;
	swig_string_data data = { 0, 0};
	int retval = EXIT_FAILURE, dumped_bytes = 0;
	int num_frames = NUM_FRAMES;
	struct mpsse_context *flash = NULL;

	if (argc > 1)
	{
		int success = sscanf (argv[1], "%d", &num_frames);
		if (!success || num_frames <= 0)
		{
			fprintf (stderr, "Usage: %s number_of_frames\n", argv[0]);
			exit (1);
		}
	}

	/* Add 16 bytes for the log start TS, and 32 bytes per frame to hold
	   host-side frame start and end TSes.  */
	if ((rx_buffer = malloc (16 + num_frames * (TRANSFER_SIZE + 32))) == NULL)
	{
		perror (argv[0]);
		exit (1);
	}
	else
		data_ptr = rx_buffer;

	fp = fopen(FOUT, "wb");
	if (!fp)
	{
		perror (argv[0]);
		exit (1);
	}

	/* Write timestamp of FTx232H init.  */
	WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
	dumped_bytes += sizeof (struct timeval);

	if((flash = MPSSE(SPI0, 15000000, MSB)) != NULL && flash->open)
	{
		FlushAfterRead (flash, 0);

		printf("%s initialized at %d Hz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));

		Start(flash);

		/* Write timestamp of pulling down #CS.  */
		WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
		dumped_bytes += sizeof (struct timeval);

#if 1
		data = Transfer(flash, tx_buffer, TRANSFER_SIZE);
#else
		data = Read (flash, TRANSFER_SIZE);
#endif

		/* Write timestamp of end-of-tranfer.  */
		WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
		dumped_bytes += sizeof (struct timeval);
#if 1		
		if(data.data)
		{
			if(fp)
			{
				int i;

				fprintf (stderr, "data.size = %d, data.data = 0x%08lx\n", data.size, (unsigned long) data.data);
#if 0
				fwrite(data.data, 1, data.size, fp);
#else
				memcpy (data_ptr, data.data, data.size);
				data_ptr += data.size;
#endif
				free(data.data);
				dumped_bytes += data.size;

				Stop(flash);

				/* Exchange next 'num_frames' TRANSFER_SIZE-byte bursts.  */
				for (i = 0; i < num_frames - 1; i++)
				{
					Start(flash);

#if 1
					/* Write timestamp of pulling down #CS.  */
					WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
					dumped_bytes += sizeof (struct timeval);
#endif

#if 1
					data = Transfer(flash, tx_buffer, TRANSFER_SIZE);
#else
					data = Read (flash, TRANSFER_SIZE);
#endif

#if 1
					/* Write timestamp of end-of-transfer.  */
					WRITE_TIMESTAMP_TO_BUFFER (data_ptr);
					dumped_bytes += sizeof (struct timeval);
#endif

					if (data.data)
					{
#if 0
						fwrite(data.data, 1, data.size, fp);
#else
						memcpy (data_ptr, data.data, data.size);
						data_ptr += data.size;
#endif
						dumped_bytes += data.size;
						free(data.data);
						Stop(flash);
					}
					else
						break;
				}
				fwrite (rx_buffer, dumped_bytes, 1, fp);
				fclose(fp);
				
				printf("Dumped %d bytes to %s\n", dumped_bytes, FOUT);
				retval = EXIT_SUCCESS;
			}
		}
#endif
	}
	else
	{
		printf("Failed to initialize MPSSE: %s\n", ErrorString(flash));
	}

	if (flash)
		Close(flash);

	return retval;
}
