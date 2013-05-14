/*
 * bootheader.h
 *
 * Copyright 2012 Emilio López <turl@tuxfamily.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#include <stdint.h>

#ifndef __BOOTHEADER__
#define __BOOTHEADER__

#define CMDLINE_END   						(0x400)
#define PADDING1_SIZE						(0x1000-0x410)
#define BOOTSTUBSTACK_SIZE					(0x1000)

struct bootheader {
	char cmdline[CMDLINE_END];
	uint32_t bzImageSize;
	uint32_t initrdSize;
	uint32_t SPIUARTSuppression;
	uint32_t SPIType;
	char padding1[PADDING1_SIZE];
	char bootstubStack[BOOTSTUBSTACK_SIZE];
};

/* Sanity check for struct size */
typedef char z[(sizeof(struct bootheader) == 0x2000) ? 1 : -1];

#endif
