#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>

#include "fileop.h"


#define M_FORMAT_PCM      (0)
#define M_FORMAT_G711A    (1)
#define M_FORMAT_G711U    (2)
#define M_FORMAT_G726_16  (3)
#define M_FORMAT_G726_32  (4)
#define M_FORMAT_AAC_ADTS (5)

static char g_infname[256];
static char g_outfname[256];
static char g_midfname[256] = ".swap.mid.dat";
static int  g_informat  = -1;
static int  g_outformat = -1;
static int  g_infd  = -1;
static int  g_outfd = -1;
static int  g_midfd = -1;

extern int file_trans_g711A_2_pcm(int ifd, int ofd);
extern int file_trans_g711U_2_pcm(int ifd, int ofd);
extern int file_trans_g726_16K_2_pcm(int ifd, int ofd);
extern int file_trans_g726_32K_2_pcm(int ifd, int ofd);
extern int file_trans_aac_2_pcm(int ifd, int ofd);

extern int file_trans_pcm_2_g711A(int ifd, int ofd);
extern int file_trans_pcm_2_g711U(int ifd, int ofd);
extern int file_trans_pcm_2_g726_16K(int ifd, int ofd);
extern int file_trans_pcm_2_g726_32K(int ifd, int ofd);
extern int file_trans_pcm_2_aac(int ifd, int ofd, int channels, int sample_rate, int bit_rate);


void usage(const char *program_name)
{
	printf("\n");
	printf("Usage: \n");
	printf("       %s -i [input file name] -o [output file name] -f [input file format] -k [output file format] -r sample_rate\n\n", program_name);
	printf("the supported file format:\n");
	printf("       0: 16bit pcm\n");
	printf("       1: G711-Alaw\n");
	printf("       2: G711-Ulaw\n");
	printf("       3: G726-16K\n");
	printf("       4: G726-32K\n");
	printf("       5: AAC-ADTS\n\n");
	printf("Example:\n");
	printf("       %s -i in.pcm -o out.aac -f 0 -k 5 -r 8000 --> transform in.pcm to out.aac\n\n", program_name);
	printf("\n");

	exit(0);
}

int main(int argc, char* argv[])
{
	int c;
	int sample_rate = 0;

	while ((c = getopt(argc, argv, "i:o:f:k:r:")) != -1) {
		switch (c) {
		case 'i':
			strcpy(g_infname, optarg);
			break;
		case 'o':
			strcpy(g_outfname, optarg);
			break;
		case 'f':
			g_informat = strtol(optarg, NULL, 0);
			break;
		case 'k':
			g_outformat = strtol(optarg, NULL, 0);
			break;
		case 'r':
			sample_rate = strtol(optarg, NULL, 0);
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if ( !g_infname[0] || !g_outfname[0] || g_informat < 0 || g_informat > 5 || g_outformat < 0 || g_outformat > 5)
	{
		usage(argv[0]);
	}

	if (g_informat == g_outformat)
	{
		printf("same file format, no need to transform!!!\n");
		return -1;
	}

	g_infd  = open(g_infname, O_RDONLY);
	g_outfd = open(g_outfname, O_RDWR | O_CREAT | O_TRUNC);
	g_midfd = open(g_midfname, O_RDWR | O_CREAT | O_TRUNC);

	if (g_infd < 0 || g_outfd < 0 || g_midfd < 0)
	{
		printf("open file failed @ start!!!\n");
		goto Exit;
	}

	//in-file ===> mid-file
	switch (g_informat)
	{
	case M_FORMAT_G711A:
		file_trans_g711A_2_pcm(g_infd, g_midfd);
		break;

	case M_FORMAT_G711U:
		file_trans_g711U_2_pcm(g_infd, g_midfd);
		break;

	case M_FORMAT_G726_16:
		file_trans_g726_16K_2_pcm(g_infd, g_midfd);
		break;

	case M_FORMAT_G726_32:
		file_trans_g726_32K_2_pcm(g_infd, g_midfd);
		break;

	case M_FORMAT_AAC_ADTS:
		file_trans_aac_2_pcm(g_infd, g_midfd);
		break;

	default: //M_FORMAT_PCM
		copy_file(g_infd, g_midfd);
		break;
	}

	close(g_midfd);
	g_midfd  = open(g_midfname, O_RDONLY);
	if (g_midfd < 0)
	{
		printf("open file failed @ mid!!!\n");
		goto Exit;
	}

	//mid-file ====> out-file
	switch (g_outformat)
	{
	case M_FORMAT_G711A:
		file_trans_pcm_2_g711A(g_midfd, g_outfd);
		break;

	case M_FORMAT_G711U:
		file_trans_pcm_2_g711U(g_midfd, g_outfd);
		break;

	case M_FORMAT_G726_16:
		file_trans_pcm_2_g726_16K(g_midfd, g_outfd);
		break;

	case M_FORMAT_G726_32:
		file_trans_pcm_2_g726_32K(g_midfd, g_outfd);
		break;

	case M_FORMAT_AAC_ADTS:
		if (sample_rate <= 0 || sample_rate > 48000)
		{
			printf("error: for aac encode, you should specify the sample rate.\n");
			goto Exit;
		}
		file_trans_pcm_2_aac(g_midfd, g_outfd, 1/*now use one channel to test*/, sample_rate, 0/*use the default bitrate*/);
		break;

	default: //M_FORMAT_PCM
		copy_file(g_midfd, g_outfd);
		break;
	}

Exit:
	if (g_infd >= 0)
		close(g_infd);
	if (g_outfd >= 0)
		close(g_outfd);
	if (g_midfd)
		close(g_midfd);

	return 0;
}
