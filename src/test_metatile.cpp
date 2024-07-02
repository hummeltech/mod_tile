#include <Magick++/Image.h>
#include <Magick++/Montage.h>
#include <MagickCore/composite.h>
#include <MagickCore/pixel.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>

#include <Magick++.h>
#include <Magick++/STL.h>
#include <string>

#include "g_logger.h"
// #include "metatile.h"
#include "metatile.h"
#include "render_config.h"
#include "renderd_config.h"
#include "request_queue.h"
// #include "store.h"
#include "store_file.h"

using namespace std;
using namespace Magick;

struct request_queue * render_request_queue;

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
	int write = 0;
	// const struct storage_backend *store;

	foreground = 1;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"config",      required_argument, 0, 'c'},
			{"map",         required_argument, 0, 'm'},
			{"max-zoom",    required_argument, 0, 'Z'},
			{"min-zoom",    required_argument, 0, 'z'},
			{"tile-dir",    required_argument, 0, 't'},
			{"verbose",     no_argument,       0, 'v'},
			{"write",       no_argument,       0, 'w'},

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

				struct stat buffer;

				if (stat(config_file_name, &buffer) != 0) {
					g_logger(G_LOG_LEVEL_CRITICAL, "Config file '%s' does not exist, please specify a valid file", config_file_name);
					return 1;
				}

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

			case 'w': /* -v, --write */
				write = 1;
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

	g_logger(G_LOG_LEVEL_INFO, "Started test_metatile with the following options:");

	if (config_file_name_passed) {
		g_logger(G_LOG_LEVEL_INFO, "\t--config      = '%s' (user-specified)", config_file_name);
	}

	g_logger(G_LOG_LEVEL_INFO, "\t--map         = '%s' (%s)", mapname, mapname_passed ? "user-specified" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--max-zoom    = '%i' (%s)", max_zoom, max_zoom_passed ? "user-specified/from config" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--min-zoom    = '%i' (%s)", min_zoom, min_zoom_passed ? "user-specified/from config" : "default");
	g_logger(G_LOG_LEVEL_INFO, "\t--tile-dir    = '%s' (%s)", tile_dir, tile_dir_passed ? "user-specified/from config" : "default");

	for (long z = min_zoom; z <= max_zoom; z ++) {
		int metatile_size = pow(METATILE, 2);
		long tiles = pow(pow(2, z), 2);
		long metatiles = tiles / metatile_size;
		int max_x = sqrt(tiles);
		int max_y = sqrt(tiles);

		g_logger(G_LOG_LEVEL_MESSAGE, "Zoom Level: %i", z);
		g_logger(G_LOG_LEVEL_MESSAGE, "\tTiles: %i", tiles);

		if (metatiles <= 1) {
			metatiles = 1;
		}

		g_logger(G_LOG_LEVEL_MESSAGE, "\tMetatiles: %i", metatiles);

		list<Image> imageRows;

		for (int y = 0; y < max_y; y ++) {
			list<Image> imagesRow;

			for (int x = 0; x < max_x; x ++) {
				char meta_magic[5];
				char meta_path[PATH_MAX];
				uint32_t meta_offset = xyzo_to_meta(meta_path, sizeof(meta_path), tile_dir, mapname, "", x, y, z);
				uint32_t tile_count;
				uint32_t tile_min_x;
				uint32_t tile_min_y;
				uint32_t tile_min_z;
				uint32_t tile_offset;
				uint32_t tile_size;

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\tTile '%i/%i/%i' is located in '%s' (offset %i)", z, x, y, meta_path, meta_offset);
				}

				ifstream metatile(meta_path, ifstream::binary);

				if (!metatile.good()) {
					if (verbose) {
						g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMetatile for '%i/%i/%i' does not exist: %s", z, x, y, meta_path);
					}

					metatile.close();
					continue;
				}

				metatile.read(meta_magic, 4);

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tMeta Magic: %s", meta_magic);
				}

				metatile.read(reinterpret_cast<char*>(&tile_count), sizeof(tile_count));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Count: %i", tile_count);
				}

				metatile.read(reinterpret_cast<char*>(&tile_min_x), sizeof(tile_min_x));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Min X: %i", tile_min_x);
				}

				metatile.read(reinterpret_cast<char*>(&tile_min_y), sizeof(tile_min_y));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Min Y: %i", tile_min_y);
				}

				metatile.read(reinterpret_cast<char*>(&tile_min_z), sizeof(tile_min_z));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Min Z: %i", tile_min_z);
				}

				metatile.seekg(meta_offset * 8, ios_base::cur);
				metatile.read(reinterpret_cast<char*>(&tile_offset), sizeof(tile_offset));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Offset: %i", tile_offset);
				}

				metatile.read(reinterpret_cast<char*>(&tile_size), sizeof(tile_size));

				if (verbose) {
					g_logger(G_LOG_LEVEL_MESSAGE, "\t\tTile Size: %i", tile_size);
				}

				char tile[tile_size];
				metatile.seekg(tile_offset);
				metatile.read(tile, tile_size);

				Blob blob(tile, tile_size);
				Image imageCell(blob);

				imagesRow.push_back(imageCell);

				metatile.close();
			}

			if (imagesRow.size() > 0 && write) {
				Image imageRow;
				appendImages(&imageRow, imagesRow.begin(), imagesRow.end());
				imageRows.push_back(imageRow);
			}
		}

		if (imageRows.size() > 0 && write) {
			Image image;
			string megatile_path((string)P_tmpdir + "/" + to_string(z) + "." + mapname + ".png");
			appendImages(&image, imageRows.begin(), imageRows.end(), true);
			g_logger(G_LOG_LEVEL_MESSAGE, "\tWriting MegaTile to: %s", megatile_path.c_str());
			image.write(megatile_path);
		}
	}
}
