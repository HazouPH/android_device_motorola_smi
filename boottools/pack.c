/*
 * pack.c
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <endian.h>

#include "bootheader.h"

#define ERROR(...) do { fprintf(stderr, __VA_ARGS__); return 1; } while(0)

int main(int argc, char *argv[])
{
	char *origin;
	char *bzImage;
	char *ramdisk;
	char *cmdline;
	char *output;
	FILE *forigin;
	FILE *foutput;
	FILE *fbzImage;
	FILE *framdisk;
	FILE *fcmdline;
	struct stat st;
	uint32_t tmp;
	char buf[BUFSIZ];
	size_t size;
	struct bootheader *file;

	if (argc != 6)
		ERROR("Usage: %s <valid image> <bzImage> <ramdisk> <cmdline> <output>\n", argv[0]);

	origin = argv[1];
	bzImage = argv[2];
	ramdisk = argv[3];
	cmdline = argv[4];
	output = argv[5];

	forigin = fopen(origin, "r");
	fbzImage = fopen(bzImage, "r");
	framdisk = fopen(ramdisk, "r");
	fcmdline = fopen(cmdline, "r");
	foutput = fopen(output, "w");
	if (!forigin || !foutput)
		ERROR("ERROR: failed to open origin or output image\n");

	/* Allocate memory and copy bootstub to it */
	file = calloc(sizeof(struct bootheader), sizeof(char));
	if (file == NULL)
		ERROR("ERROR allocating memory\n");

	if (fread(file, sizeof(struct bootheader), 1, forigin) != 1)
		ERROR("ERROR reading bootstub\n");

	/* Figure out the bzImage size and set it */
	if (stat(bzImage, &st) == 0) {
		tmp = st.st_size;
		file->bzImageSize = htole32(tmp);
	} else
		ERROR("ERROR reading bzImage size\n");

	/* Figure out the ramdisk size and set it */
	if (stat(ramdisk, &st) == 0) {
		tmp = st.st_size;
		file->initrdSize = htole32(tmp);
	} else
		ERROR("ERROR reading ramdisk\n");

	/* Write the patched bootstub with old cmdline to the new image */
	if (fwrite(file, sizeof(struct bootheader), 1, foutput) != 1)
		ERROR("ERROR writing image\n");

	/* Then copy the new bzImage */
	while ((size = fread(buf, 1, BUFSIZ, fbzImage))) {
		fwrite(buf, 1, size, foutput);
	}

	/* Then copy the new ramdisk */
	while ((size = fread(buf, 1, BUFSIZ, framdisk))) {
		fwrite(buf, 1, size, foutput);
	}

	/* And finally copy the new cmdline if exist */
	/* by rewinding to beginning of output file */
	if (fcmdline) {
		rewind(foutput);
		while ((size = fread(buf, 1, BUFSIZ, fcmdline))) {
			fwrite(buf, 1, size, foutput);
		}
	}

	return 0;
}
