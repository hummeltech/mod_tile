#include <Magick++.h>
#include <Magick++/Image.h>
#include <Magick++/Montage.h>
#include <Magick++/STL.h>
#if IMAGEMAGICK_VERSION >= 7
#include <MagickCore/composite.h>
#include <MagickCore/pixel.h>
#else
#include <magick/composite.h>
#include <magick/pixel.h>
#endif
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <limits>
#include <string>

#include "g_logger.h"
#include "metatile.h"
#include "render_config.h"
#include "renderd_config.h"

using namespace Magick;
using namespace std::filesystem;

struct meta_layout *read_metatile_layout(path metatile_path, std::ifstream &metatile_file)
{
	unsigned int header_len = sizeof(struct meta_layout) + METATILE * METATILE * sizeof(struct entry);
	unsigned int header_pos = 0;
	struct meta_layout *metatile_layout = reinterpret_cast<struct meta_layout *>(malloc(header_len));

	while (header_pos < header_len) {
		size_t len = header_len - header_pos;
		metatile_file.read(reinterpret_cast<char *>(metatile_layout + header_pos), len);

		if (metatile_file.gcount() < 0) {
			g_logger(G_LOG_LEVEL_CRITICAL, "Failed to read complete header for metatile %s", metatile_path.c_str());
			metatile_file.close();
			free(metatile_layout);
			exit(1);
		} else if (metatile_file.gcount() > 0) {
			header_pos += metatile_file.gcount();
		} else {
			break;
		}
	}

	return metatile_layout;
}

Image read_metatile_tile(path metatile_path, std::ifstream &metatile_file, struct meta_layout *metatile_layout, uint32_t metatile_offset)
{
	char tile[metatile_layout->index[metatile_offset].size];
	metatile_file.seekg(metatile_layout->index[metatile_offset].offset);
	metatile_file.read(tile, metatile_layout->index[metatile_offset].size);

	if (metatile_file.gcount() != metatile_layout->index[metatile_offset].size) {
		g_logger(G_LOG_LEVEL_CRITICAL, "Failed to read expected bytes from metatile %s", metatile_path.c_str());
		metatile_file.close();
		free(metatile_layout);
		exit(1);
	}

	Blob blob(tile, metatile_layout->index[metatile_offset].size);
	return Image(blob);
}

Image read_metatile_tiles(path metatile_path, std::ifstream &metatile_file, struct meta_layout *metatile_layout)
{
	std::list<Image> imageCols;

	for (int x = 0; x < METATILE; x ++) {
		std::list<Image> imagesCol;

		for (int y = 0; y < METATILE; y ++) {
			int metatile_offset = x * METATILE + y;

			if (metatile_layout->index[metatile_offset].size == 0) {
				continue;
			}

			Image tile = read_metatile_tile(metatile_path, metatile_file, metatile_layout, metatile_offset);
			imagesCol.push_back(tile);
		}

		Image imageCol;
		appendImages(&imageCol, imagesCol.begin(), imagesCol.end(), true);
		imageCols.push_back(imageCol);
	}

	Image image;
	appendImages(&image, imageCols.begin(), imageCols.end());
	return image;
}

int count_metatile_tiles(struct meta_layout *metatile_layout)
{
	int metatile_tile_count = 0;

	for (int metatile_offset = 0; metatile_offset < metatile_layout->count; metatile_offset ++) {
		if (metatile_layout->index[metatile_offset].size > 0) {
			metatile_tile_count ++;
		}
	}

	return metatile_tile_count;
}

int main(int argc, char **argv)
{
	const char *config_file_name_default = RENDERD_CONFIG;
	const char *mapname_default = XMLCONFIG_DEFAULT;
	const char *tile_dir_default = RENDERD_TILE_DIR;
	int max_zoom_default = MAX_ZOOM;
	int min_zoom_default = 0;

	const char *config_file_name = config_file_name_default;
	const char *mapname = mapname_default;
	const char *tile_dir = tile_dir_default;
	int max_zoom = max_zoom_default;
	int min_zoom = min_zoom_default;

	int config_file_name_passed = 0;
	int mapname_passed = 0;
	int max_zoom_passed = 0;
	int min_zoom_passed = 0;
	int tile_dir_passed = 0;

	int verbose = 0;
	int display = 0;
	path tile_dir_path;

	foreground = 1;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"config",      required_argument, 0, 'c'},
			{"display",     no_argument,       0, 'D'},
			{"map",         required_argument, 0, 'm'},
			{"max-zoom",    required_argument, 0, 'Z'},
			{"min-zoom",    required_argument, 0, 'z'},
			{"tile-dir",    required_argument, 0, 't'},
			{"verbose",     no_argument,       0, 'v'},

			{"help",        no_argument,       0, 'h'},
			{"version",     no_argument,       0, 'V'},
			{0, 0, 0, 0}
		};

		int c = getopt_long(argc, argv, "c:m:Z:z:t:vwhV", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'c': /* -c, --config */
				config_file_name = strndup(optarg, PATH_MAX);
				config_file_name_passed = 1;

				if (!exists(config_file_name)) {
					g_logger(G_LOG_LEVEL_CRITICAL, "Config file '%s' does not exist, please specify a valid file", config_file_name);
					return 1;
				}

				break;

			case 'D': /* -D, --display */
				display = 1;
				break;

			case 'm': /* -m, --map */
				mapname = strndup(optarg, XMLCONFIG_MAX);
				mapname_passed = 1;
				break;

			case 'Z': /* -Z, --max-zoom */
				max_zoom = min_max_int_opt(optarg, "maximum zoom", 0, MAX_ZOOM);
				max_zoom_passed = 1;
				break;

			case 'z': /* -z, --min-zoom */
				min_zoom = min_max_int_opt(optarg, "minimum zoom", 0, MAX_ZOOM);
				min_zoom_passed = 1;
				break;

			case 't': /* -t, --tile-dir */
				tile_dir = strndup(optarg, PATH_MAX);
				tile_dir_passed = 1;
				break;

			case 'v': /* -v, --verbose */
				verbose = 1;
				break;

			case 'h': /* -h, --help */
				fprintf(stderr, "Usage: test_metatile [OPTION] ...\n");
				fprintf(stderr, "  -c, --config=CONFIG               specify the renderd config file (default is off)\n");
				fprintf(stderr, "  -m, --map=MAP                     render tiles in this map (default is '%s')\n", mapname_default);
				fprintf(stderr, "  -t, --tile-dir=TILE_DIR           tile cache directory (default is '%s')\n", tile_dir_default);
				fprintf(stderr, "  -Z, --max-zoom=ZOOM               only render tiles less than or equal to this zoom level (default is '%d')\n", max_zoom_default);
				fprintf(stderr, "  -z, --min-zoom=ZOOM               only render tiles greater than or equal to this zoom level (default is '%d')\n", min_zoom_default);
				fprintf(stderr, "\n");
				fprintf(stderr, "  -h, --help                        display this help and exit\n");
				fprintf(stderr, "  -V, --version                     display the version number and exit\n");
				return 0;

			case 'V': /* -V, --version */
				fprintf(stdout, "%s\n", VERSION);
				return 0;

			default:
				g_logger(G_LOG_LEVEL_CRITICAL, "unhandled char '%c'", c);
				return 1;
		}
	}

	if (max_zoom < min_zoom) {
		g_logger(G_LOG_LEVEL_CRITICAL, "Specified min zoom (%i) is larger than max zoom (%i).", min_zoom, max_zoom);
		return 1;
	}

	if (config_file_name_passed) {
		int map_section_num = -1;
		process_config_file(config_file_name, 0, G_LOG_LEVEL_DEBUG);

		for (int i = 0; i < XMLCONFIGS_MAX; ++i) {
			if (maps[i].xmlname && strcmp(maps[i].xmlname, mapname) == 0) {
				map_section_num = i;
			}
		}

		if (map_section_num < 0) {
			g_logger(G_LOG_LEVEL_CRITICAL, "Map section '%s' does not exist in config file '%s'.", mapname, config_file_name);
			return 1;
		}

		if (!max_zoom_passed) {
			max_zoom = maps[map_section_num].max_zoom;
			max_zoom_passed = 1;
		}

		if (!min_zoom_passed) {
			min_zoom = maps[map_section_num].min_zoom;
			min_zoom_passed = 1;
		}

		if (!tile_dir_passed) {
			tile_dir = strndup(maps[map_section_num].tile_dir, PATH_MAX);
			tile_dir_passed = 1;
		}
	}

	// store = init_storage_backend(tile_dir);

	// if (store == NULL) {
	// 	g_logger(G_LOG_LEVEL_CRITICAL, "Failed to initialise storage backend %s", tile_dir);
	// 	return 1;
	// }

	tile_dir_path = path(tile_dir) / mapname;

	if (exists(tile_dir_path)) {
		current_path(tile_dir_path);
	} else {
		g_logger(G_LOG_LEVEL_CRITICAL, "Tile directory '%s' does not exist", tile_dir);
		return 1;
	}

	g_logger(G_LOG_LEVEL_INFO, "Started test_metatile with the following options:");

	if (config_file_name_passed) {
		g_logger(G_LOG_LEVEL_INFO, "\t--config      = '%s' (user-specified)", config_file_name);
	}

	g_logger(G_LOG_LEVEL_INFO, "\t--map         = '%s' (%s)", mapname, mapname_passed ? "user-specified" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--max-zoom    = '%i' (%s)", max_zoom, max_zoom_passed ? "user-specified/from config" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--min-zoom    = '%i' (%s)", min_zoom, min_zoom_passed ? "user-specified/from config" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--tile-dir    = '%s' (%s)", tile_dir, tile_dir_passed ? "user-specified/from config" : "default");

	for (long z = min_zoom; z <= max_zoom; z++) {
		int metatile_size = pow(METATILE, 2);
		long found_tiles = 0;
		long found_metatiles = 0;
		long total_tiles = pow(pow(2, z), 2);
		long total_metatiles = total_tiles / metatile_size;
		int max_x = sqrt(total_tiles);
		int max_y = sqrt(total_tiles);

		if (total_metatiles <= 1) {
			total_metatiles = 1;
		}

		g_logger(G_LOG_LEVEL_MESSAGE, "Zoom Level: %i", z);
		g_logger(G_LOG_LEVEL_MESSAGE, "\tTotal Metatiles: %ld", total_metatiles);
		g_logger(G_LOG_LEVEL_MESSAGE, "\tTotal Tiles: %ld", total_tiles);

		if (exists(path(std::to_string(z)))) {
			for (const directory_entry& dir_entry : recursive_directory_iterator(std::to_string(z))) {
				if (dir_entry.is_regular_file()) {
					std::ifstream metatile_file(dir_entry.path(), std::ifstream::binary);
					struct meta_layout *metatile_layout = read_metatile_layout(dir_entry, metatile_file);
					struct entry metatile_layout_entry = metatile_layout->index[metatile_size - 1];
					int expected_file_size = metatile_layout_entry.offset + metatile_layout_entry.size;

					if (verbose) {
						g_logger(G_LOG_LEVEL_MESSAGE, "\tMetatile path: '%s'", dir_entry.path().c_str());
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile Magic: %.4s", metatile_layout->magic);
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile Tile Count: %i", metatile_layout->count);
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile Min Tile X: %i", metatile_layout->x);
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile Min Tile Y: %i", metatile_layout->y);
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile Min Tile Z: %i", metatile_layout->z);
					}

					if (dir_entry.file_size() == expected_file_size) {
						if (verbose) {
							g_logger(G_LOG_LEVEL_MESSAGE, "\tMetatile size is correct: %i Bytes", dir_entry.file_size());
						}

						found_metatiles ++;
						found_tiles += count_metatile_tiles(metatile_layout);
					} else {
						g_logger(G_LOG_LEVEL_WARNING, "\tMetatile size is incorrect: %i Bytes (%i Bytes expected)", dir_entry.file_size(), expected_file_size);
					}

					if (display) {
						read_metatile_tiles(dir_entry, metatile_file, metatile_layout).display();
					}

					metatile_file.close();
				}
			}
		}

		g_logger(G_LOG_LEVEL_MESSAGE, "\tFound Tiles: %ld", found_tiles);
		g_logger(G_LOG_LEVEL_MESSAGE, "\tFound Metatiles: %ld", found_metatiles);
	}
}
