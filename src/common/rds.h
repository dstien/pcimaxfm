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

#ifndef _PCIMAXFM_COMMON_RDS_H
#define _PCIMAXFM_COMMON_RDS_H

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
	TEXT8, TEXT64, INT10, RDS_TYPE_END
};

extern const char *rds_type_name[RDS_TYPE_END];
extern char *const rds_params_name[RDS_PARAM_END];
extern const char *rds_params_description[RDS_PARAM_END];
extern const int rds_params_type[RDS_PARAM_END];

int validate_rds(int, char*, int, char *);

#endif /* _PCIMAXFM_COMMON_RDS_H */
