#define SWIGPYTHON

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpsse.h>
#include <time.h>

#define SIZE	0x10			// Size of SPI flash device: 1MB
#define RCMD	"\xde\xad\xbe\xef\xca\xfe\xba\xbe"	// Standard SPI flash read command (0x03) followed by starting address (0x000000)
#define FOUT	"trace.bin"		// Output file

#ifdef SWIGPYTHON
  #warning SWIGPYTHON enabled!
#endif

#define NUM_FRAMES     160
#define TRANSFER_SIZE 1536

/* Append current timeval to file DEST.  */
#define WRITE_TIMESTAMP_TO_FILE(DEST) \
do { \
  struct timeval buf; \
  gettimeofday (&buf, NULL); \
  fwrite ((void *) &buf, sizeof (struct timeval), 1, DEST); \
} while (0)

unsigned char tx_buffer[TRANSFER_SIZE] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe };

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

	fp = fopen(FOUT, "wb");
	if (!fp)
	{
		perror ("Cannot open FOUT");
		exit (1);
	}

	/* Write timestamp of FTx232H init.  */
	WRITE_TIMESTAMP_TO_FILE(fp);

	if((flash = MPSSE(SPI0, 15000000, MSB)) != NULL && flash->open)
	{
		printf("%s initialized at %d Hz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));

		Start(flash);

		/* Write timestamp of pulling down #CS.  */
		WRITE_TIMESTAMP_TO_FILE(fp);

		data = Transfer(flash, tx_buffer, TRANSFER_SIZE);

		/* Write timestamp of end-of-tranfer.  */
		WRITE_TIMESTAMP_TO_FILE(fp);

		Stop(flash);

#if 1		
		if(data.data)
		{
			if(fp)
			{
				int i;

				fprintf (stderr, "data.size = %d, data.data = 0x%08lx\n", data.size, (unsigned long) data.data);
				fwrite(data.data, 1, data.size, fp);
				free(data.data);
				dumped_bytes += data.size;

				/* Exchange next 'num_frames' TRANSFER_SIZE-byte bursts.  */
				for (i = 0; i < num_frames - 1; i++)
				{
					Start(flash);

					/* Write timestamp of pulling down #CS.  */
					WRITE_TIMESTAMP_TO_FILE(fp);

					data = Transfer(flash, tx_buffer, TRANSFER_SIZE);

					/* Write timestamp of end-of-transfer.  */
					WRITE_TIMESTAMP_TO_FILE(fp);

					Stop(flash);

					if (data.data)
					{
						fwrite(data.data, 1, data.size, fp);
						dumped_bytes += data.size;
						free(data.data);
					}
					else
						break;
				}
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
