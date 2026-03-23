#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "catch/catch.hpp"
#include "catch_test_common.hpp"
#include "sys_utils.h"

extern bool fail_next_getloadavg;
extern std::string err_log_lines;

extern "C" {
#define g_logger mocked_g_logger
#define getloadavg mocked_getloadavg

#include "sys_utils.c"

#undef g_logger
#undef getloadavg
}

TEST_CASE("get_load_avg function", "[get_load_avg]")
{
	err_log_lines.clear();

	SECTION("get_load_avg", "should return positive") {
		double loadavg = get_load_avg();

		REQUIRE(loadavg > 0);
	}

	SECTION("get_load_avg with unobtainable load average", "should return 1000") {
		fail_next_getloadavg = true;

		double loadavg = get_load_avg();

#ifdef HAVE_GETLOADAVG
		REQUIRE(loadavg == 1000);
#else
		REQUIRE(loadavg > 0);
#endif
	}
}
