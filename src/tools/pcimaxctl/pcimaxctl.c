/*
 * pcimaxfm - PCI MAX FM transmitter driver and tools
 * Copyright (C) 2007-2009 Daniel Stien <daniel@stien.org>
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

#define _XOPEN_SOURCE 500

#include <pcimaxfm.h>

#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if PCIMAXFM_ENABLE_RDS
#include "../../common/rds.h"
#endif /* PCIMAXFM_ENABLE_RDS */

#define ERROR_MSG(format, ...) { if (verbosity >= 0) fprintf(stderr, "Error: " format "\n", ## __VA_ARGS__); exit(-1); }
#define NOTICE_MSG(format, ...) if (verbosity >= 0) printf(format "\n", ## __VA_ARGS__)
#define DEBUG_MSG(format, ...) if (verbosity == 1) printf(format "\n", ## __VA_ARGS__)

#define FREQ(steps) (steps / 20.0f)

int verbosity = 0;
int fd = 0;
char *dev = "/dev/pcimaxfm0";

static struct option long_options[] = {
#if PCIMAXFM_ENABLE_TX_TOGGLE
	{ "tx",         optional_argument, 0, 't' },
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
	{ "freq",       optional_argument, 0, 'f' },
	{ "power",      optional_argument, 0, 'p' },
	{ "stereo",     optional_argument, 0, 's' },
#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
	{ "rds-signal", optional_argument, 0, 'g' },
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
	{ "rds",        required_argument, 0, 'r' },
#endif /* PCIMAXFM_ENABLE_RDS */
	{ "device",     optional_argument, 0, 'd' },
	{ "verbose",    no_argument,       0, 'v' },
	{ "quiet",      no_argument,       0, 'q' },
	{ "version",    no_argument,       0, 'e' },
	{ "help",       no_argument,       0, 'h' },
#if PCIMAXFM_ENABLE_RDS
	{ "help-rds",   no_argument,       0, 'H' },
#endif /* PCIMAXFM_ENABLE_RDS */
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
#if PCIMAXFM_ENABLE_TX_TOGGLE
	printf("-t, --tx[=1|0]            get/toggle transmitter power (1 = on, 0 = off)\n");
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
	printf("-f, --freq[=MHz|50KHz]    get/set frequency in MHz (%.2f-%.2f)\n", FREQ(PCIMAXFM_FREQ_MIN), FREQ(PCIMAXFM_FREQ_MAX));
	printf("                          or 50 KHz steps (%d-%d)\n", PCIMAXFM_FREQ_MIN, PCIMAXFM_FREQ_MAX);
	printf("-p, --power[=LVL]         get/set power level (%d-%d)\n", PCIMAXFM_POWER_MIN, PCIMAXFM_POWER_MAX);
	printf("-s, --stereo[=1|0]        get/toggle stereo encoder (1 = on, 0 = off)\n");
#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
	printf("-g, --rds-signal[=1|0]    get/toggle RDS signal (1 = on, 0 = off)\n");
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
	printf("-r, --rds=PARM=VAL[,...]  set RDS parameters (see --help-rds)\n\n");
#endif /* PCIMAXFM_ENABLE_RDS */

	printf("-d, --device[=FILE]       pcimaxfm device (default: /dev/pcimaxfm0)\n");
	printf("-v, --verbose             verbose output\n");
	printf("-q, --quiet               no output\n");
	printf("-e, --version             print version and exit\n");
	printf("-h, --help                print this text and exit\n");
#if PCIMAXFM_ENABLE_RDS
	printf("-H, --help-rds            print list of valid RDS parameters\n\n");
#else
	printf("\n");
#endif /* PCIMAXFM_ENABLE_RDS */

	printf("Report bugs to <"PACKAGE_BUGREPORT">.\n");

	exit(status);
}

#if PCIMAXFM_ENABLE_RDS
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
#endif /* PCIMAXFM_ENABLE_RDS */

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

#if PCIMAXFM_ENABLE_TX_TOGGLE
void tx(char *arg)
{
	int tx;

	dev_open();
	if (arg) {
		if (sscanf(arg, "%u", &tx) < 1 || tx < 0 || tx > 1) {
			ERROR_MSG("Invalid transmitter power state. Got \"%s\", expected integer 1 or 0.", arg);
		}

		if (ioctl(fd, PCIMAXFM_TX_SET, &tx) == -1) {
			ERROR_MSG("Setting transmitter power state failed.");
		}
	} else {
		if (ioctl(fd, PCIMAXFM_TX_GET, &tx) == -1) {
			 ERROR_MSG("Reading transmitter power state failed.");
		}
	}

	NOTICE_MSG("Transmitter: %s", PCIMAXFM_STR_BOOL(tx));
}
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */

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
		if (sscanf(arg, "%u", &stereo) < 1 || stereo < 0 || stereo > 1) {
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

	NOTICE_MSG("Stereo encoder: %s", PCIMAXFM_STR_BOOL(stereo));
}

#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
void rds_signal(char *arg)
{
	int signal;

	dev_open();
	if (arg) {
		if (sscanf(arg, "%u", &signal) < 1 || signal < 0 || signal > 1) {
			ERROR_MSG("Invalid RDS signal state. Got \"%s\", expected integer 1 or 0.", arg);
		}

		if (ioctl(fd, PCIMAXFM_RDSSIGNAL_SET, &signal) == -1) {
			ERROR_MSG("Setting RDS signal state failed.");
		}
	} else {
		if (ioctl(fd, PCIMAXFM_RDSSIGNAL_GET, &signal) == -1) {
			ERROR_MSG("Reading RDS signal state failed.");
		}
	}

	NOTICE_MSG("RDS signal: %s", PCIMAXFM_STR_BOOL(signal));
}
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */

void rds(char *arg)
{
	int c;
	char *val, err[0xff];
	struct pcimaxfm_rds_set rds_set;

	while (*arg != '\0') {
		if ((c = (getsubopt(&arg, rds_params_name, &val))) == -1)
			ERROR_MSG("Invalid RDS parameter \"%s\".", val);

		if (validate_rds(c, val, sizeof(err), err)) {
			ERROR_MSG("%s", err);
		}

		dev_open();

		rds_set.param = c;
		rds_set.value = val;

		if(ioctl(fd, PCIMAXFM_RDS_SET, &rds_set) == -1) {
			ERROR_MSG("Writing RDS parameter %s = \"%s\" failed.",
					rds_params_name[rds_set.param],
					rds_set.value);
		}

		NOTICE_MSG("RDS: %-4s = \"%s\"",
				rds_params_name[rds_set.param], rds_set.value);
	}
}
#endif /* PCIMAXFM_ENABLE_RDS */

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

		c = getopt_long(argc, argv,
#if PCIMAXFM_ENABLE_TX_TOGGLE
				"t::"
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
				"f::p::s::"
#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
				"g::"
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
				"r:"
#endif /* PCIMAXFM_ENABLE_RDS */
				"d::vqehH",
				long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
#if PCIMAXFM_ENABLE_TX_TOGGLE
			case 't':
				tx(optarg);
				break;
#endif /* PCIMAXFM_ENABLE_TX_TOGGLE */
			case 'f':
				freq(optarg);
				break;
			case 'p':
				power(optarg);
				break;
			case 's':
				stereo(optarg);
				break;
#if PCIMAXFM_ENABLE_RDS
#if PCIMAXFM_ENABLE_RDS_TOGGLE
			case 'g':
				rds_signal(optarg);
				break;
#endif /* PCIMAXFM_ENABLE_RDS_TOGGLE */
			case 'r':
				rds(optarg);
				break;
#endif /* PCIMAXFM_ENABLE_RDS */
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
#if PCIMAXFM_ENABLE_RDS
			case 'H':
				print_help_rds(0);
#endif /* PCIMAXFM_ENABLE_RDS */
			default:
				exit(1);
		}
	}

	dev_close();

	return 0;
}
