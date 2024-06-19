/*
 * Copyright (c) 2007 - 2023 by mod_tile contributors (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see http://www.gnu.org/licenses/.
 */

#ifndef DIR_UTILS_H
#define DIR_UTILS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "render_config.h"

/* Build parent directories for the specified file name
 * Note: the part following the trailing / is ignored
 * e.g. mkdirp("/a/b/foo.png") == shell mkdir -p /a/b
 */
int mkdirp(const char *path);

int path_to_xyz(const char *tilepath, const char *path, char *xmlconfig, int *px, int *py, int *pz);

/* Returns the path to the metatile and the offset within the metatile */
int xyz_to_meta(char *path, size_t len, const char *tile_dir, const char *xmlconfig, int x, int y, int z);

#ifdef __cplusplus
}
#endif

#endif
