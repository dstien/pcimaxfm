/*
 * pcimaxfm - PCI MAX FM transmitter driver and tools
 * Copyright (C) 2007 Daniel Stien <daniel@stien.org>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <pcimaxfm.h>

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define ERROR_MSG(format, ...) { if (verbosity >= 0) fprintf(stderr, "Error: " format "\n", ## __VA_ARGS__); exit(-1); }
#define NOTICE_MSG(format, ...) if (verbosity >= 0) printf(format "\n", ## __VA_ARGS__)
#define DEBUG_MSG(format, ...) if (verbosity == 1) printf(format "\n", ## __VA_ARGS__)

#define FREQ(steps) (steps / 20.0f)
#define IS_ON(val) (val == 0 ? "Off" : "On")

int verbosity = 0;
int fd = 0;
char *dev = "/dev/pcimaxfm0";

enum
{
	PS00, PS01, PS02, PS03, PS04, PS05, PS06, PS07, PS08, PS09,
	PS10, PS11, PS12, PS13, PS14, PS15, PS16, PS17, PS18, PS19,
	PS20, PS21, PS22, PS23, PS24, PS25, PS26, PS27, PS28, PS29,
	PS30, PS31, PS32, PS33, PS34, PS35, PS36, PS37, PS38, PS39,

	PD00, PD01, PD02, PD03, PD04, PD05, PD06, PD07, PD08, PD09,
	PD10, PD11, PD12, PD13, PD14, PD15, PD16, PD17, PD18, PD19,
	PD20, PD21, PD22, PD23, PD24, PD25, PD26, PD27, PD28, PD29,
	PD30, PD31, PD32, PD33, PD34, PD35, PD36, PD37, PD38, PD39,

	RT, RDS_PARAM_END
};

enum
{
	TEXT8, TEXT64, INT10
};

const char *rds_type_name[] = {
	[TEXT8]  = "8 chars",
	[TEXT64] = "64 chars",
	[INT10]  = "0 - 10"
};

const char *rds_params_name[] = {
	[PS00] = "PS00", [PS01] = "PS01", [PS02] = "PS02", [PS03] = "PS03",
	[PS04] = "PS04", [PS05] = "PS05", [PS06] = "PS06", [PS07] = "PS07",
	[PS08] = "PS08", [PS09] = "PS09", [PS10] = "PS10", [PS11] = "PS11",
	[PS12] = "PS12", [PS13] = "PS13", [PS14] = "PS14", [PS15] = "PS15",
	[PS16] = "PS16", [PS17] = "PS17", [PS18] = "PS18", [PS19] = "PS19",
	[PS20] = "PS20", [PS21] = "PS21", [PS22] = "PS22", [PS23] = "PS23",
	[PS24] = "PS24", [PS25] = "PS25", [PS26] = "PS26", [PS27] = "PS27",
	[PS28] = "PS28", [PS29] = "PS29", [PS30] = "PS30", [PS31] = "PS31",
	[PS32] = "PS32", [PS33] = "PS33", [PS34] = "PS34", [PS35] = "PS35",
	[PS36] = "PS36", [PS37] = "PS37", [PS38] = "PS38", [PS39] = "PS39",

	[PD00] = "PD00", [PD01] = "PD01", [PD02] = "PD02", [PD03] = "PD03",
	[PD04] = "PD04", [PD05] = "PD05", [PD06] = "PD06", [PD07] = "PD07",
	[PD08] = "PD08", [PD09] = "PD09", [PD10] = "PD10", [PD11] = "PD11",
	[PD12] = "PD12", [PD13] = "PD13", [PD14] = "PD14", [PD15] = "PD15",
	[PD16] = "PD16", [PD17] = "PD17", [PD18] = "PD18", [PD19] = "PD19",
	[PD20] = "PD20", [PD21] = "PD21", [PD22] = "PD22", [PD23] = "PD23",
	[PD24] = "PD24", [PD25] = "PD25", [PD26] = "PD26", [PD27] = "PD27",
	[PD28] = "PD28", [PD29] = "PD29", [PD30] = "PD30", [PD31] = "PD31",
	[PD32] = "PD32", [PD33] = "PD33", [PD34] = "PD34", [PD35] = "PD35",
	[PD36] = "PD36", [PD37] = "PD37", [PD38] = "PD38", [PD39] = "PD39",

	[RT] = "RT"
};

const int rds_params_type[] = {
	[PS00] = TEXT8, [PS01] = TEXT8, [PS02] = TEXT8, [PS03] = TEXT8,
	[PS04] = TEXT8, [PS05] = TEXT8, [PS06] = TEXT8, [PS07] = TEXT8,
	[PS08] = TEXT8, [PS09] = TEXT8, [PS10] = TEXT8, [PS11] = TEXT8,
	[PS12] = TEXT8, [PS13] = TEXT8, [PS14] = TEXT8, [PS15] = TEXT8,
	[PS16] = TEXT8, [PS17] = TEXT8, [PS18] = TEXT8, [PS19] = TEXT8,
	[PS20] = TEXT8, [PS21] = TEXT8, [PS22] = TEXT8, [PS23] = TEXT8,
	[PS24] = TEXT8, [PS25] = TEXT8, [PS26] = TEXT8, [PS27] = TEXT8,
	[PS28] = TEXT8, [PS29] = TEXT8, [PS30] = TEXT8, [PS31] = TEXT8,
	[PS32] = TEXT8, [PS33] = TEXT8, [PS34] = TEXT8, [PS35] = TEXT8,
	[PS36] = TEXT8, [PS37] = TEXT8, [PS38] = TEXT8, [PS39] = TEXT8,

	[PD00] = INT10, [PD01] = INT10, [PD02] = INT10, [PD03] = INT10,
	[PD04] = INT10, [PD05] = INT10, [PD06] = INT10, [PD07] = INT10,
	[PD08] = INT10, [PD09] = INT10, [PD10] = INT10, [PD11] = INT10,
	[PD12] = INT10, [PD13] = INT10, [PD14] = INT10, [PD15] = INT10,
	[PD16] = INT10, [PD17] = INT10, [PD18] = INT10, [PD19] = INT10,
	[PD20] = INT10, [PD21] = INT10, [PD22] = INT10, [PD23] = INT10,
	[PD24] = INT10, [PD25] = INT10, [PD26] = INT10, [PD27] = INT10,
	[PD28] = INT10, [PD29] = INT10, [PD30] = INT10, [PD31] = INT10,
	[PD32] = INT10, [PD33] = INT10, [PD34] = INT10, [PD35] = INT10,
	[PD36] = INT10, [PD37] = INT10, [PD38] = INT10, [PD39] = INT10,

	[RT] = TEXT64
};

const char *rds_params_description[] = {
	[PS00] = "Program service banks",
	[PD00] = "Program service banks duration",
	[RT] = "Radio text"
};

static struct option long_options[] = {
	{ "freq",     optional_argument, 0, 'f' },
	{ "power",    optional_argument, 0, 'p' },
	{ "stereo",   optional_argument, 0, 's' },
	{ "rds",      required_argument, 0, 'r' },
	{ "device",   optional_argument, 0, 'd' },
	{ "verbose",  no_argument,       0, 'v' },
	{ "quiet",    no_argument,       0, 'q' },
	{ "version",  no_argument,       0, 'e' },
	{ "help",     no_argument,       0, 'h' },
	{ "help-rds", no_argument,       0, 'H' },
	{ 0, 0, 0, 0 }
};

void print_version(char *prog)
{
	printf("%s (%s) %s\n", prog, PACKAGE, PACKAGE_VERSION);
	printf(PACKAGE_COPYRIGHT"\n");
	printf("This is free software. You may redistribute copies of it under the terms of\n");
	printf("the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");

	exit(0);
}

void print_help(char *prog, int status)
{
	printf("Usage: %s [OPTION]...\n", prog);
	printf("Control PCI MAX FM transmitter devices.\n\n");

	printf("Omitting optional arguments will print current value.\n");
	printf("-f, --freq[=MHz|50KHz]    get/set frequency in MHz (%.2f-%.2f)\n", FREQ(PCIMAXFM_FREQ_MIN), FREQ(PCIMAXFM_FREQ_MAX));
	printf("                          or 50 KHz steps (%d-%d)\n", PCIMAXFM_FREQ_MIN, PCIMAXFM_FREQ_MAX);
	printf("-p, --power[=LVL]         get/set power level (%d-%d)\n", PCIMAXFM_POWER_MIN, PCIMAXFM_POWER_MAX);
	printf("-s, --stereo[=1|0]        get/toggle stereo encoder (1 = on, 0 = off)\n");
	printf("-r, --rds=PARM=VAL[,...]  set RDS parameters (see --help-rds)\n\n");

	printf("-d, --device[=FILE]       pcimaxfm device (default: /dev/pcimaxfm0)\n");
	printf("-v, --verbose             verbose output\n");
	printf("-q, --quiet               no output\n");
	printf("-e, --version             print version and exit\n");
	printf("-h, --help                print this text and exit\n");
	printf("-H, --help-rds            print list of valid RDS parameters\n\n");

	printf("Report bugs to <"PACKAGE_BUGREPORT">.\n");

	exit(status);
}

void print_help_rds(int status)
{
	int i;

	printf("Valid parameters for the --rds option.\n\n");
	printf("Parameter    Type       Description\n");
	printf("~~~~~~~~~~~  ~~~~       ~~~~~~~~~~~\n");
	printf("%s - %s  %-10s %s\n",
			rds_params_name[PS00],
			rds_params_name[PS39],
			rds_type_name[rds_params_type[PS00]],
			rds_params_description[PS00]);
	printf("%s - %s  %-10s %s\n",
			rds_params_name[PD00],
			rds_params_name[PD39],
			rds_type_name[rds_params_type[PD00]],
			rds_params_description[PD00]);


	for (i = RT; i < RDS_PARAM_END; i++) {
		printf("%-12s %-10s %s\n", rds_params_name[i], rds_type_name[rds_params_type[i]], rds_params_description[i]);
	}

	printf("\n");

	exit(status);
}

void dev_open()
{
	if (!fd) {
		if((fd = open(dev, O_RDWR)) == -1) {
			perror("open");
			exit(-1);
		}
	}
}

void dev_close()
{
	if (fd)
		close(fd);
}

void freq(const char *arg)
{
	int freq;
	double ffreq;

	dev_open();
	if (arg) {
		if (sscanf(arg, "%lf", &ffreq) < 1) {
			ERROR_MSG("Invalid frequency. Got \"%s\", expected floating point number in the range of %.2f-%.2f or integer in the range of %d-%d.",
					arg, FREQ(PCIMAXFM_FREQ_MIN),
					FREQ(PCIMAXFM_FREQ_MAX),
					PCIMAXFM_FREQ_MIN, PCIMAXFM_FREQ_MAX);
		}

		freq = (int)ffreq;

		if (freq < PCIMAXFM_FREQ_MIN) {
			freq = (int)(ffreq * 20.0f);
		}

		if (freq < PCIMAXFM_FREQ_MIN || freq > PCIMAXFM_FREQ_MAX) {
			ERROR_MSG("Frequency out of range. Got %.2f, expected %.2f-%.2f or %d-%d.",
					ffreq, FREQ(PCIMAXFM_FREQ_MIN),
					FREQ(PCIMAXFM_FREQ_MAX),
					PCIMAXFM_FREQ_MIN, PCIMAXFM_FREQ_MAX);
		}
		
		if (ioctl(fd, PCIMAXFM_FREQ_SET, &freq) == -1) {
			ERROR_MSG("Setting frequency failed.");
		}
	} else {
		if (ioctl(fd, PCIMAXFM_FREQ_GET, &freq) == -1) {
			ERROR_MSG("Reading frequency failed.");
		}

		if (freq == PCIMAXFM_FREQ_NA) {
			NOTICE_MSG("Frequency not set yet.");
			return;
		}
	}

	NOTICE_MSG("Frequency: %.2f MHz (%d 50 KHz steps)", FREQ(freq), freq);
}

void power(const char *arg)
{
	int power;

	dev_open();
	if (arg) {
		if (sscanf(arg, "%u", &power) < 1) {
			ERROR_MSG("Invalid power level. Got \"%s\", expected integer in the range of %d-%d.", arg, PCIMAXFM_POWER_MIN, PCIMAXFM_POWER_MAX);
		}

		if (power < PCIMAXFM_POWER_MIN || power > PCIMAXFM_POWER_MAX) {
			ERROR_MSG("Power level out of range. Got %d, expected %d-%d.", power, PCIMAXFM_POWER_MIN, PCIMAXFM_POWER_MAX);
		}

		if (ioctl(fd, PCIMAXFM_POWER_SET, &power) == -1) {
			ERROR_MSG("Setting power level failed.");
		}
	} else {
		if (ioctl(fd, PCIMAXFM_POWER_GET, &power) == -1) {
			ERROR_MSG("Reading power level failed.");
		}

		if (power == PCIMAXFM_POWER_NA) {
			NOTICE_MSG("Power level not set yet.");
			return;
		}
	}

	NOTICE_MSG("Power level: %d/%d", power, PCIMAXFM_POWER_MAX);
}

void stereo(char *arg)
{
	int stereo;

	dev_open();
	if (arg) {
		if (sscanf(arg, "%u", &stereo) < 1) {
			ERROR_MSG("Invalid stereo encoder state. Got \"%s\", expected integer 1 or 0.", arg);
		}

		if (stereo < 0 || stereo > 1) {
			ERROR_MSG("Invalid stereo encoder state. Got \"%s\", expected integer 1 or 0.", arg);
		}

		if (ioctl(fd, PCIMAXFM_STEREO_SET, &stereo) == -1) {
			ERROR_MSG("Setting stereo encoder state failed.");
		}
	} else {
		if (ioctl(fd, PCIMAXFM_STEREO_GET, &stereo) == -1) {
			ERROR_MSG("Reading stereo encoder state failed.");
		}
	}

	NOTICE_MSG("Stereo encoder: %s", IS_ON(stereo));
}

void rds_write(int param, char *val)
{
	int len, integer;
	struct pcimaxfm_rds_set rds_set;

	if (param < 0 || param >= RDS_PARAM_END) {
		ERROR_MSG("Invalid RDS parameter (id %d)", param);
	}

	switch (rds_params_type[param]) {
		case TEXT8:
			len = strlen(val);

			if (len < 1 || len > 8) {
				ERROR_MSG("Invalid value for RDS parameter %s. Got \"%s\", expected 1-8 characters text string.", rds_params_name[param], val);
			}
			break;

		case TEXT64:
			len = strlen(val);

			if (len < 1 || len > 64) {
				ERROR_MSG("Invalid value for RDS parameter %s. Got \"%s\", expected 1-64 characters text string.", rds_params_name[param], val);
			}
			break;

		case INT10:
			if (sscanf(val, "%u", &integer) < 1) {
				ERROR_MSG("Invalid value for RDS parameter %s. Got \"%s\", expected integer in the range of 0-10.",  rds_params_name[param], val);
			}

			if (integer < 0 || integer > 10) {
				ERROR_MSG("Integer value for RDS parameter %s out of range. Got %d, expected 0-10.", rds_params_name[param], integer);
			}

			sprintf(val, "%u", integer); // Recreate clean string without potential garbage.
			break;

		default:
			ERROR_MSG("Invalid RDS parameter type (parameter id %d).", param);
	}

	dev_open();

	rds_set.param = (char *)rds_params_name[param];
	rds_set.value = val;

	if(ioctl(fd, PCIMAXFM_RDS_SET, &rds_set) == -1)
		ERROR_MSG("Writing RDS parameter %s = \"%s\" failed.", rds_set.param, rds_set.value);

	NOTICE_MSG("RDS: %-4s = \"%s\"", rds_set.param, rds_set.value);

}

void rds(char *arg)
{
	int c;
	char *val;

	while (*arg != '\0') {
		if ((c = (getsubopt(&arg, rds_params_name, &val))) == -1)
			ERROR_MSG("Invalid RDS parameter \"%s\".", val);

		rds_write(c, val);
	}
}

void device(char *arg)
{
	if (arg) {
		if (fd) {
			ERROR_MSG("File descriptor in use. Set device before any query options.");
		}

		dev = arg;
		DEBUG_MSG("Using device \"%s\".", dev);
	} else {
		NOTICE_MSG("Using device \"%s\".", dev);
	}
}

int main(int argc, char **argv)
{
	int c, option_index;

	if (argc < 2)
		print_help(argv[0], -1);

	while (1) {
		option_index = 0;

		c = getopt_long(argc, argv, "f::p::s::r:d::vqehH", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
			case 'f':
				freq(optarg);
				break;
			case 'p':
				power(optarg);
				break;
			case 's':
				stereo(optarg);
				break;
			case 'r':
				rds(optarg);
				break;
			case 'd':
				device(optarg);
				break;
			case 'v':
				verbosity = 1;
				DEBUG_MSG("Verbose output.");
				break;
			case 'q':
				verbosity = -1;
				break;
			case 'e':
				print_version(argv[0]);
			case 'h':
				print_help(argv[0], 0);
			case 'H':
				print_help_rds(0);
			default:
				exit(1);
		}
	}

	dev_close();

	return 0;
}
