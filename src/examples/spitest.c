#define SWIGPYTHON

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpsse.h>

#define SIZE	0x10			// Size of SPI flash device: 1MB
#define RCMD	"\xde\xad\xbe\xef\xca\xfe\xba\xbe"	// Standard SPI flash read command (0x03) followed by starting address (0x000000)
#define FOUT	"trace.bin"		// Output file

#ifdef SWIGPYTHON
  #warning SWIGPYTHON enabled!
#endif

#define TRANSFER_SIZE 1536
unsigned char tx_buffer[TRANSFER_SIZE] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe };


int main(void)
{
	FILE *fp = NULL;
	swig_string_data data = { 0, 0};
	int retval = EXIT_FAILURE, dumped_bytes = 0;
	struct mpsse_context *flash = NULL;
	
	if((flash = MPSSE(SPI0, 15000000, MSB)) != NULL && flash->open)
	{
		printf("%s initialized at %d Hz (SPI mode 0)\n", GetDescription(flash), GetClock(flash));
		
		Start(flash);
		data = Transfer(flash, tx_buffer, TRANSFER_SIZE);
		//data = Read(flash, SIZE);
		//data1 = Read(flash, SIZE);
		//usleep (1);
		Stop(flash);

#if 1		
		if(data.data)
		{
			fp = fopen(FOUT, "wb");
			if(fp)
			{
				int i;

				fprintf (stderr, "data.size = %d, data.data = 0x%08lx\n", data.size, (unsigned long) data.data);
				fwrite(data.data, 1, data.size, fp);
				free(data.data);
				dumped_bytes += data.size;

				/* Exchange next 15 TRANSFER_SIZE-byte bursts, wait 100 microseconds in between.  */
				for (i = 0; i < 15; i++)
				{
					Start(flash);
					data = Transfer(flash, tx_buffer, TRANSFER_SIZE);
					//usleep (1);
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
