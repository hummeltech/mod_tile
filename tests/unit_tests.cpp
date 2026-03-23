#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <setjmp.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mapnik/datasource_cache.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/params.hpp>

#if MAPNIK_MAJOR_VERSION < 4
#include <boost/optional/optional_io.hpp>
#endif

#include "catch/catch.hpp"
#include "catch_test_common.hpp"

#include "config.h"

#include "metatile.h"
#include "parameterize_style.hpp"
#include "protocol.h"
#include "protocol_helper.h"
#include "render_submit_queue.h"
#include "store_file.h"
#include "sys_utils.h"

#ifndef MAPNIK_XML
#define MAPNIK_XML "./utils/example-map/mapnik.xml"
#endif

#ifndef MAPNIK_PLUGINS_DIR
#define MAPNIK_PLUGINS_DIR "/usr/local/lib64/mapnik/input"
#endif

extern bool fail_next_asprintf;
extern bool fail_next_connect;
extern bool fail_next_getaddrinfo;
extern bool fail_next_getaddrinfo_empty_res;
extern bool fail_next_getloadavg;
extern bool fail_next_malloc;
extern bool fail_next_mkdir;
extern bool fail_next_open;
extern bool fail_next_socket;
extern bool fail_next_strndup;
extern bool fail_next_strtok;
extern bool fail_next_write;
extern int exit_status;

extern jmp_buf exit_jump;
extern std::string err_log_lines;

extern "C" {
	bool fail_next_recv = false;
	int fail_next_recv_reponse_size = -1;
	int fail_next_recv_reponse_version = -1;
	int fail_next_next_recv_reponse_size = -1;
	int fail_next_next_recv_reponse_version = -1;

	struct storage_backend * init_storage_backend(const char * options)
	{
		struct storage_backend * store = init_storage_file(options);
		return store;
	}

	ssize_t mocked_recv(int fd, void *buf, size_t n, int flags)
	{
		if (fail_next_recv) {
			fail_next_recv = false;
			return -1;
		}

		if (fail_next_recv_reponse_size != -1) {
			int reponse_size = fail_next_recv_reponse_size;
			fail_next_recv_reponse_size = (fail_next_next_recv_reponse_size != -1) ? fail_next_next_recv_reponse_size : -1;
			fail_next_next_recv_reponse_size = -1;

			if (fail_next_recv_reponse_version != -1) {
				struct protocol *cmd = (struct protocol *)malloc(sizeof(struct protocol));
				cmd->ver = fail_next_recv_reponse_version;
				fail_next_recv_reponse_version = (fail_next_next_recv_reponse_version != -1) ? fail_next_next_recv_reponse_version : -1;
				fail_next_next_recv_reponse_version = -1;

				memcpy(buf, cmd, reponse_size);

				free(cmd);
			}

			return reponse_size;
		}

		return recv(fd, buf, n, flags);
	}
}

#define connect mocked_connect
#define exit mocked_exit
#define g_logger mocked_g_logger
#define getaddrinfo mocked_getaddrinfo
#define getloadavg mocked_getloadavg
#define malloc mocked_malloc
#define mkdir mocked_mkdir
#define open mocked_open
#define recv mocked_recv
#define socket mocked_socket
#define strtok mocked_strtok
#define write mocked_write

extern "C" {
#include "cache_expire.c"
#include "protocol_helper.c"
#include "render_submit_queue.c"
#include "store_file.c"
#include "store_file_utils.c"
#include "sys_utils.c"
}

#include "metatile.cpp"
#include "parameterize_style.cpp"

#undef connect
#undef exit
#undef g_logger
#undef getaddrinfo
#undef getloadavg
#undef malloc
#undef mkdir
#undef open
#undef recv
#undef socket
#undef strtok
#undef write

#include "unit_test_cache_expire.cpp"
#include "unit_test_metatile.cpp"
#include "unit_test_parameterize_style.cpp"
#include "unit_test_protocol_helper.cpp"
#include "unit_test_render_submit_queue.cpp"
#include "unit_test_renderd_config.cpp"
#include "unit_test_sys_utils.cpp"
