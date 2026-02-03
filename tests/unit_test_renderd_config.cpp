#include <cstdlib>
#include <cstring>
#include <fstream>
#include <setjmp.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "catch/catch.hpp"
#include "catch_test_common.hpp"
#include "renderd_config.h"

#ifndef RENDERD_CONF
#define RENDERD_CONF "./etc/renderd/renderd.conf.examples"
#endif

extern int exit_status;
extern bool fail_next;
extern jmp_buf exit_jump;
extern std::string err_log_lines;

extern "C" {
#define asprintf mocked_asprintf
#define exit mocked_exit
#define g_logger mocked_g_logger
#define strndup mocked_strndup

#include "renderd_config.c"

#undef asprintf
#undef exit
#undef g_logger
#undef strndup
}

TEST_CASE("renderd_config function failure exit handling", "[renderd_config] [copy_string] [name_with_section]")
{
	err_log_lines.clear();
	exit_status = 0;

	SECTION("copy_string handles strndup failure by exiting", "should return 7") {
		const char *src = "test";
		const char *dest;

		fail_next = true;

		if (setjmp(exit_jump) == 0) {
			copy_string(src, &dest, strlen(src));
			FAIL("copy_string should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to strndup failure");
		}

		fail_next = false;

		REQUIRE(exit_status == 7);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("copy_string: strndup error"));
	}

	SECTION("copy_string with valid src and valid dest", "should succeed") {
		const char *src = "test";
		const char *dest = nullptr;

		REQUIRE(nullptr == dest);
		copy_string(src, &dest, strlen(src));
		REQUIRE(strcmp(src, dest) == 0);
	}


	SECTION("name_with_section handles asprintf failure by exiting") {
		const char *name = "socketname";
		const char *section = "renderd";
		char *value = nullptr;

		REQUIRE(nullptr == value);
		value = name_with_section(section, name);
		REQUIRE((std::string(section) + ":" + name) == value);
	}

	SECTION("name_with_section handles asprintf failure by exiting", "should return 7") {
		const char *section = "renderd";
		const char *name = "socketname";

		fail_next = true;

		if (setjmp(exit_jump) == 0) {
			name_with_section(section, name);
			FAIL("name_with_section should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}

		fail_next = false;

		REQUIRE(exit_status == 7);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("name_with_section: asprintf error"));
	}
}

TEST_CASE("renderd.conf file processing exit handling", "[renderd_config] [process_config_file] [process_renderd_sections] [process_mapnik_section] [process_map_sections]")
{
	err_log_lines.clear();
	exit_status = 0;

	SECTION("valid renderd.conf file with invalid active renderd section", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_config_file(RENDERD_CONF, MAX_SLAVES, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Active renderd section (" + std::to_string(MAX_SLAVES) + ") must be between 0 and " + std::to_string(MAX_SLAVES - 1) + "."));
	}

	SECTION("valid renderd.conf file with nonexistent active renderd section", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_config_file(RENDERD_CONF, (MAX_SLAVES - 1), 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Active renderd section (" + std::to_string(MAX_SLAVES - 1) + ") does not exist."));
	}

	SECTION("nonexistent renderd.conf file with valid active renderd section", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_config_file("doesnotexist", 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Failed to load config file (process_config_file): 'doesnotexist'"));
	}

	SECTION("nonexistent renderd.conf file with valid active renderd section (process_renderd_sections)", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_renderd_sections(NULL, "doesnotexist", config_slaves);
			FAIL("process_renderd_sections should have called exit(), but did not");
		} else {
			SUCCEED("process_renderd_sections captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Failed to load config file (process_renderd_sections): 'doesnotexist'"));
	}

	SECTION("nonexistent renderd.conf file with valid active renderd section (process_mapnik_section)", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_mapnik_section(NULL, "doesnotexist", config_slaves);
			FAIL("process_mapnik_section should have called exit(), but did not");
		} else {
			SUCCEED("process_mapnik_section captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Failed to load config file (process_mapnik_section): 'doesnotexist'"));
	}

	SECTION("nonexistent renderd.conf file with valid active renderd section (process_map_sections)", "should return 1") {
		if (setjmp(exit_jump) == 0) {
			process_map_sections(NULL, "doesnotexist", maps, "", 0);
			FAIL("process_map_sections should have called exit(), but did not");
		} else {
			SUCCEED("process_map_sections captured expected exit() call");
		}

		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Failed to load config file (process_map_sections): 'doesnotexist'"));
	}

	SECTION("valid renderd.conf file with valid active renderd section (process_config_file)", "should return 0") {
		if (setjmp(exit_jump) == 0) {
			process_config_file(RENDERD_CONF, 0, 0);
			SUCCEED("process_config_file did not call exit(), as expected");
		} else {
			FAIL("process_config_file captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
	}

	SECTION("valid renderd.conf file with valid active renderd section (process_renderd_sections)", "should return 0") {
		if (setjmp(exit_jump) == 0) {
			process_renderd_sections(NULL, RENDERD_CONF, config_slaves);
			SUCCEED("process_renderd_sections did not call exit(), as expected");
		} else {
			FAIL("process_renderd_sections captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
	}

	SECTION("valid renderd.conf file with valid active renderd section (process_mapnik_section)", "should return 0") {
		if (setjmp(exit_jump) == 0) {
			process_mapnik_section(NULL, RENDERD_CONF, config_slaves);
			SUCCEED("process_mapnik_section did not call exit(), as expected");
		} else {
			FAIL("process_mapnik_section captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
	}

	SECTION("valid renderd.conf file with valid active renderd section (process_map_sections)", "should return 0") {
		if (setjmp(exit_jump) == 0) {
			process_map_sections(NULL, RENDERD_CONF, maps, "", 0);
			SUCCEED("process_map_sections did not call exit(), as expected");
		} else {
			FAIL("process_map_sections captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
	}

	std::string renderd_conf_path = std::string(P_tmpdir) + "/renderd.conf_XXXXXX";
	int fd = mkstemp(&renderd_conf_path[0]);

	if (fd == -1) {
		FAIL("mkstemp failed");
	}

	close(fd);
	std::ofstream renderd_conf_file(renderd_conf_path);

	SECTION("renderd.conf with too many map sections", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";

		for (int i = 0; i <= XMLCONFIGS_MAX; i++) {
			renderd_conf_file << "[map" + std::to_string(i) + "]\n";
		}

		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Can't handle more than " + std::to_string(XMLCONFIGS_MAX) + " map config sections"));
	}

	SECTION("renderd.conf without map sections", "should return 1") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("No map config sections were found in file: " + renderd_conf_path));
	}

	SECTION("renderd.conf without mapnik section", "should return 1") {
		renderd_conf_file << "[map]\n[renderd]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("No mapnik config section was found in file: " + renderd_conf_path));
	}

	SECTION("renderd.conf with invalid renderd sections", "should return 7") {
		std::string renderd_conf_renderd_section_name = "renderdinvalid";

		renderd_conf_file << "[mapnik]\n[map]\n[" + renderd_conf_renderd_section_name + "]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid renderd section name: " + renderd_conf_renderd_section_name));
	}

	SECTION("renderd.conf with too many renderd sections", "should return 7") {
		renderd_conf_file << "[mapnik]\n[map]\n";

		for (int i = 0; i <= MAX_SLAVES; i++) {
			renderd_conf_file << "[renderd" + std::to_string(i) + "]\n";
		}

		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Can't handle more than " + std::to_string(MAX_SLAVES) + " renderd config sections"));
	}

	SECTION("renderd.conf without renderd sections", "should return 1") {
		renderd_conf_file << "[map]\n[mapnik]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 1);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("No renderd config sections were found in file: " + renderd_conf_path));
	}

	SECTION("renderd.conf map section scale too small", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nscale=0.0\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified scale factor (0.000000) is too small, must be greater than or equal to 0.100000."));
	}

	SECTION("renderd.conf map section scale too large", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nscale=8.1\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified scale factor (8.100000) is too large, must be less than or equal to 8.000000."));
	}

	SECTION("renderd.conf map section maxzoom too small", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nmaxzoom=-1\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified max zoom (-1) is too small, must be greater than or equal to 0."));
	}

	SECTION("renderd.conf map section maxzoom too large", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nmaxzoom=" << MAX_ZOOM + 1 << "\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified max zoom (" + std::to_string(MAX_ZOOM + 1) + ") is too large, must be less than or equal to " + std::to_string(MAX_ZOOM) + "."));
	}

	SECTION("renderd.conf map section minzoom too small", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nminzoom=-1\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified min zoom (-1) is too small, must be greater than or equal to 0."));
	}

	SECTION("renderd.conf map section minzoom too large", "should return 7") {
		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\nminzoom=" << MAX_ZOOM + 1 << "\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified min zoom (" + std::to_string(MAX_ZOOM + 1) + ") is larger than max zoom (" + std::to_string(MAX_ZOOM) + ")."));
	}

	SECTION("renderd.conf map section type has too few parts", "should return 7") {
		std::string renderd_conf_map_type = "a";

		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\ntype=" + renderd_conf_map_type + "\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified type (" + renderd_conf_map_type + ") has too few parts, there must be at least 2, e.g., 'png image/png'."));
	}

	SECTION("renderd.conf map section type has too many parts", "should return 7") {
		std::string renderd_conf_map_type = "a b c d";

		renderd_conf_file << "[mapnik]\n[renderd]\n";
		renderd_conf_file << "[map]\ntype=" + renderd_conf_map_type + "\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified type (" + renderd_conf_map_type + ") has too many parts, there must be no more than 3, e.g., 'png image/png png256'."));
	}

	SECTION("renderd.conf map section type has two parts", "should return 0") {
		renderd_conf_file << "[mapnik]\n[map]\ntype=png image/png\n";
		renderd_conf_file << "[renderd]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			SUCCEED("process_config_file did not call exit(), as expected");
		} else {
			FAIL("process_config_file captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Read map:type: 'png image/png'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Read map:type:file_extension: 'png'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Read map:type:mime_type: 'image/png'"));
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Read map:type:output_format: 'png256'"));
	}

	SECTION("renderd.conf renderd section socketname is too long", "should return 7") {
		int renderd_socketname_maxlen = sizeof(((struct sockaddr_un *)0)->sun_path);
		std::string renderd_socketname = "/" + std::string(renderd_socketname_maxlen, 'A');

		renderd_conf_file << "[mapnik]\n[map]\n";
		renderd_conf_file << "[renderd]\nsocketname=" << renderd_socketname << "\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Specified socketname (" + renderd_socketname + ") exceeds maximum allowed length of " + std::to_string(renderd_socketname_maxlen) + "."));
	}

	SECTION("renderd.conf duplicate renderd section names", "should return 7") {
		renderd_conf_file << "[mapnik]\n[map]\n";
		renderd_conf_file << "[renderd0]\n[renderd]\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			FAIL("process_config_file should have called exit(), but did not");
		} else {
			SUCCEED("process_config_file captured expected exit() call");
		}

		REQUIRE(exit_status == 7);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Duplicate renderd config section names for section 0: renderd0 & renderd"));
	}

	SECTION("renderd.conf renderd section num_threads is -1", "should return 0") {
		renderd_conf_file << "[mapnik]\n[map]\n";
		renderd_conf_file << "[renderd0]\nnum_threads=-1\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			SUCCEED("process_config_file did not call exit(), as expected");
		} else {
			FAIL("process_config_file captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd(0): num_threads = '" + std::to_string(sysconf(_SC_NPROCESSORS_ONLN)) + "'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd: num_threads = '" + std::to_string(sysconf(_SC_NPROCESSORS_ONLN)) + "'"));
	}

	SECTION("renderd.conf slave renderd sections' num_threads sum equals num_slave_threads", "should return 0") {
		renderd_conf_file << "[mapnik]\n[map]\n";
		renderd_conf_file << "[renderd0]\nnum_threads=-1\n[renderd1]\nnum_threads=2\n[renderd2]\nnum_threads=2\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			SUCCEED("process_config_file did not call exit(), as expected");
		} else {
			FAIL("process_config_file captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd(1): num_threads = '2'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd(2): num_threads = '2'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd: num_slave_threads = '4'"));
	}

	SECTION("renderd.conf renderd section not using unix socketname", "should return 0") {
		renderd_conf_file << "[mapnik]\n[map]\n";
		renderd_conf_file << "[renderd0]\niphostname=hostname\nipport=9999\n";
		renderd_conf_file.close();

		if (setjmp(exit_jump) == 0) {
			process_config_file(renderd_conf_path.c_str(), 0, 0);
			SUCCEED("process_config_file did not call exit(), as expected");
		} else {
			FAIL("process_config_file captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd(0): ip socket = 'hostname:9999'"));
        REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("renderd: ip socket = 'hostname:9999'"));
	}

	std::remove(renderd_conf_path.c_str());
}

TEST_CASE("renderd_config minimum and maximum function exit handling", "[renderd_config] [min_max_double_opt] [min_max_int_opt]")
{
    const char *opt_type_name = "value";
    double dmax = 1.15;
    double dmin = 1.10;

	err_log_lines.clear();
	exit_status = 0;

	SECTION("min_max_double_opt success", "should return 0") {
        const char *opt_arg = "1.125";

		if (setjmp(exit_jump) == 0) {
		    min_max_double_opt(opt_arg, opt_type_name, dmin, dmax);
			SUCCEED("min_max_double_opt did not call exit(), as expected");
		} else {
			FAIL("min_max_double_opt captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
		REQUIRE(err_log_lines == "");
	}

	SECTION("min_max_double_opt exceeds max", "should return 1") {
        const char *opt_arg = "2";

		if (setjmp(exit_jump) == 0) {
		    min_max_double_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_double_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be <= " + std::to_string(dmax) + " (" + std::string(opt_arg) + " was provided)"));
	}

	SECTION("min_max_double_opt less than min", "should return 1") {
        const char *opt_arg = "1";

		if (setjmp(exit_jump) == 0) {
		    min_max_double_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_double_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be >= " + std::to_string(dmin) + " (" + std::string(opt_arg) + " was provided)"));
	}

	SECTION("min_max_double_opt exceeds max", "should return 1") {
        const char *opt_arg = "fail";

		if (setjmp(exit_jump) == 0) {
		    min_max_double_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_double_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be a double (" + std::string(opt_arg) + " was provided)"));
	}


	SECTION("min_max_int_opt success", "should return 0") {
        const char *opt_arg = "1";

		if (setjmp(exit_jump) == 0) {
		    min_max_int_opt(opt_arg, opt_type_name, dmin, dmax);
			SUCCEED("min_max_int_opt did not call exit(), as expected");
		} else {
			FAIL("min_max_int_opt captured unexpected exit() call");
		}

		REQUIRE(exit_status == 0);
		REQUIRE(err_log_lines == "");
	}

	SECTION("min_max_int_opt exceeds max", "should return 1") {
        const char *opt_arg = "2";

		if (setjmp(exit_jump) == 0) {
		    min_max_int_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_int_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be <= " + std::to_string((int)dmax) + " (" + std::string(opt_arg) + " was provided)"));
	}

	SECTION("min_max_int_opt less than min", "should return 1") {
        const char *opt_arg = "0";

		if (setjmp(exit_jump) == 0) {
		    min_max_int_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_int_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be >= " + std::to_string((int)dmin) + " (" + std::string(opt_arg) + " was provided)"));
	}

	SECTION("min_max_int_opt exceeds max", "should return 1") {
        const char *opt_arg = "fail";

		if (setjmp(exit_jump) == 0) {
		    min_max_int_opt(opt_arg, opt_type_name, dmin, dmax);
			FAIL("min_max_int_opt should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to asprintf failure");
		}
		REQUIRE(exit_status == 1);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Invalid " + std::string(opt_type_name) + ", must be an integer (" + std::string(opt_arg) + " was provided)"));
	}
}
