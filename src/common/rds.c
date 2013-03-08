/*
 * pcimaxfm - PCI MAX FM transmitter driver and tools
 * Copyright (C) 2007-2013 Daniel Stien <daniel@stien.org>
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

#include "rds.h"

#define RDS_MSG_ERR(fmt, ...) \
	if (err_len > 0) snprintf(err, err_len, fmt, ## __VA_ARGS__); return -1;

const char *rds_type_name[] = {
	[TEXT8]  = "8 chars",
	[TEXT64] = "64 chars",
	[INT10]  = "0 - 10"
};

char *const rds_params_name[] = {
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

typedef unsigned int size_t;

int snprintf(char *, size_t, const char *, ...);
size_t strlen(const char *);
int sscanf(const char *, const char *, ...);

int validate_rds(int param, char *val, int err_len, char *err)
{
	int len, integer;

	if (param < 0 || param >= RDS_PARAM_END) {
		RDS_MSG_ERR("Invalid RDS parameter (id %d)", param);
	}

	if (val == 0) {
		RDS_MSG_ERR("Value required for RDS paramater %s (%s).",
				rds_params_name[param],
				rds_type_name[rds_params_type[param]]);
	}

	switch (rds_params_type[param]) {
		case TEXT8:
			len = strlen(val);

			if (len < 1 || len > 8) {
				RDS_MSG_ERR("Invalid value for RDS parameter %s. Got \"%s\", expected 1-8 characters text string.", rds_params_name[param], val);
			}
			break;

		case TEXT64:
			len = strlen(val);

			if (len < 1 || len > 64) {
				RDS_MSG_ERR("Invalid value for RDS parameter %s. Got \"%s\", expected 1-64 characters text string.", rds_params_name[param], val);
			}
			break;

		case INT10:
			if (sscanf(val, "%u", &integer) < 1) {
				RDS_MSG_ERR("Invalid value for RDS parameter %s. Got \"%s\", expected integer in the range of 0-10.",  rds_params_name[param], val);
			}

			if (integer < 0 || integer > 10) {
				RDS_MSG_ERR("Integer value for RDS parameter %s out of range. Got %d, expected 0-10.", rds_params_name[param], integer);
			}

			/* Recreate clean string without potential garbage. */
			snprintf(val, sizeof(val), "%u", integer);
			break;

		default:
			RDS_MSG_ERR("Invalid RDS parameter type (parameter id %d).", param);
	}

	return 0;
}
