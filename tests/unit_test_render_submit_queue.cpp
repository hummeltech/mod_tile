#include <chrono>
#include <fcntl.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "catch/catch.hpp"
#include "catch_test_common.hpp"
#include "render_submit_queue.h"
#include "sys_utils.h"

extern int exit_status;
extern bool fail_next_getloadavg;
extern bool fail_next_socket;
extern jmp_buf exit_jump;
extern std::string err_log_lines;

extern "C" {
#define exit mocked_exit
#define g_logger mocked_g_logger
#define getloadavg mocked_getloadavg
#define socket mocked_socket

#include "protocol_helper.c"
#include "render_submit_queue.c"
#include "sys_utils.c"

#undef exit
#undef g_logger
#undef getloadavg
#undef socket
}

TEST_CASE("check_load function", "[check_load]")
{
	err_log_lines.clear();

	maxLoad = 999;
	auto start = std::chrono::high_resolution_clock::now();

	SECTION("check_load with max load of 999", "should return") {
		check_load();
	}

	SECTION("check_load with max load of 999 and unobtainable load average (which returns 1000)", "should return after sleeping 5 seconds") {
		fail_next_getloadavg = true;
		check_load();
		REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() >= 5);
	}
}

TEST_CASE("process function", "[process]")
{
	int fd, ret;
	int pipefd[2];
	pipe(pipefd);
	struct protocol *cmd = (struct protocol *)malloc(sizeof(struct protocol));

	err_log_lines.clear();

	fd = pipefd[0];

	SECTION("process", "should return positive") {
		ret = process(cmd, fd);

		REQUIRE(ret == 0);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Sending request"));
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("send error: Success"));
	}
}

TEST_CASE("make_connection function", "[make_connection]")
{
	int ret;
	std::string socket_path = std::string(P_tmpdir) + "/renderd.sock";

	err_log_lines.clear();
	exit_status = 0;

	SECTION("make_connection", "should return positive") {
		ret = make_connection(socket_path.c_str());

		// REQUIRE(ret > 0);
	}

	SECTION("make_connection handles socket failure by exiting", "should exit 2") {
		fail_next_socket = true;

		if (setjmp(exit_jump) == 0) {
			make_connection(socket_path.c_str());
			FAIL("make_connection should have called exit() but didn't");
		} else {
			SUCCEED("Captured expected exit() call due to socket failure");
		}

		REQUIRE(exit_status == 2);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("failed to create unix socket"));
	}
}
