#include <chrono>
#include <csetjmp>

#include "catch/catch.hpp"
#include "catch_test_common.hpp"

#include "render_submit_queue.h"

extern bool fail_next_getloadavg;
extern bool fail_next_socket;
extern int exit_status;
extern jmp_buf exit_jump;
extern std::string err_log_lines;

TEST_CASE("render_submit_queue.c", "[render_submit_queue]")
{
	SECTION("check_load function") {
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

	SECTION("process function") {
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
			// REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("send error: Success"));
		}
	}

	SECTION("make_connection function") {
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
}
