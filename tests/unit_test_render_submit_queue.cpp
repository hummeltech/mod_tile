#include <chrono>
#include <csetjmp>
#include <string>
#include <thread>
#include <unistd.h>

#include "catch/catch.hpp"
#include "catch_test_common.hpp"

#include "config.h"
#include "protocol.h"
#include "render_submit_queue.h"

extern bool fail_next_calloc;
extern bool fail_next_getloadavg;
extern bool fail_next_pthread_create;
extern bool fail_next_socket;
extern bool pass_next_connect;
extern int exit_status;
extern int fail_next_next_recv_reponse_size;
extern int fail_next_next_recv_reponse_version;
extern int fail_next_recv_reponse_cmd;
extern int fail_next_recv_reponse_size;
extern int fail_next_recv_reponse_version;
extern jmp_buf exit_jump;
extern std::string err_log_lines;

TEST_CASE("render_submit_queue.c", "[render_submit_queue]")
{
	maxLoad = 999;

	SECTION("check_load function")
	{
		err_log_lines.clear();

		auto start = std::chrono::high_resolution_clock::now();

		SECTION("check_load with max load of 999", "should return")
		{
			check_load();
		}

		SECTION("check_load with max load of 999 and unobtainable load average (which returns 1000)", "should return after sleeping 5 seconds")
		{
			fail_next_getloadavg = true;
			check_load();
			REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() >= 5);
		}
	}

	SECTION("process function")
	{
		int fd, ret;
		int pipefd[2];
		REQUIRE(pipe(pipefd) == 0);
		struct protocol *cmd = (struct protocol *)calloc(1, sizeof(struct protocol));

		cmd->cmd = cmdRender;
		cmd->ver = 3;
		cmd->x = 1024;
		cmd->y = 1024;
		cmd->z = 10;

		auto start = std::chrono::high_resolution_clock::now();

		fd = pipefd[0];

		err_log_lines.clear();

		SECTION("process with incomplete response", "should return 0")
		{
			ret = process(cmd, fd);

			REQUIRE(ret == 0);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Sending request"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Waiting for response"));
			REQUIRE_THAT(err_log_lines, !Catch::Matchers::Contains("Got response"));
		}

		SECTION("process with cmdNotDone response", "should return positive after sleeping 1 second")
		{
			fail_next_next_recv_reponse_size = 64;
			fail_next_next_recv_reponse_version = 3;
			fail_next_recv_reponse_size = sizeof(struct protocol) - 64;
			fail_next_recv_reponse_version = 3;
			fail_next_recv_reponse_cmd = cmdNotDone;

			ret = process(cmd, fd);

			REQUIRE(ret > 0);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Sending request"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Waiting for response"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Got response"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Rendering not done with command"));
			REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1);
		}

		SECTION("process with cmdDone response", "should return positive")
		{
			fail_next_next_recv_reponse_size = 64;
			fail_next_next_recv_reponse_version = 3;
			fail_next_recv_reponse_size = sizeof(struct protocol) - 64;
			fail_next_recv_reponse_version = 3;
			fail_next_recv_reponse_cmd = cmdDone;

			ret = process(cmd, fd);

			REQUIRE(ret > 0);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Sending request"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Waiting for response"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Got response"));
			REQUIRE_THAT(err_log_lines, !Catch::Matchers::Contains("Rendering not done with command"));
			REQUIRE(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() < 1);
		}
	}

// Tests need some work for FreeBSD & macOS
#if !defined(__FreeBSD__) && !defined(__MACH__)
	SECTION("fetch function")
	{
		struct protocol *fetch_response;

		err_log_lines.clear();
		exit_status = 0;
		pthread_mutex_unlock(&qLock);

		SECTION("qLen=0, work_complete=1", "should return NULL")
		{
			qLen = 0;
			work_complete = 1;

			fetch_response = fetch();

			REQUIRE(fetch_response == NULL);
		}

		SECTION("qLen=0, work_complete=0", "should return NULL")
		{
			qLen = 0;
			work_complete = 0;

			pthread_t worker;
			pthread_create(&worker, NULL, (void *(*)(void *))fetch, NULL);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			pthread_cond_broadcast(&qCondNotEmpty);

			pthread_cancel(worker);
			pthread_join(worker, (void **)fetch_response);
			REQUIRE(fetch_response == NULL);
		}

		SECTION("qHead!=NULL, qLen=2, work_complete=0", "should return NULL")
		{
			char *mapname = strdup("mapname");
			qHead = new qItem{mapname, 1024, 1024, 10, NULL};
			qLen = 2;
			work_complete = 0;

			pthread_t worker;
			pthread_create(&worker, NULL, (void *(*)(void *))fetch, NULL);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			pthread_cond_broadcast(&qCondNotEmpty);

			pthread_cancel(worker);
			pthread_join(worker, (void **)fetch_response);
			REQUIRE(fetch_response == NULL);
			REQUIRE(qLen == 1);
			REQUIRE(qHead == NULL);
		}

		SECTION("qHead!=NULL, qLen=1, work_complete=0", "should return NULL")
		{
			char *mapname = strdup("mapname");
			qHead = new qItem{mapname, 1024, 1024, 10, NULL};
			qLen = 1;
			work_complete = 0;

			pthread_t worker;
			pthread_create(&worker, NULL, (void *(*)(void *))fetch, NULL);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			pthread_cond_broadcast(&qCondNotEmpty);

			pthread_cancel(worker);
			pthread_join(worker, (void **)fetch_response);
			REQUIRE(fetch_response == NULL);
			REQUIRE(qLen == 0);
			REQUIRE(qHead == NULL);
		}

		SECTION("qHead=NULL, qLen=1, work_complete=0", "should exit 1")
		{
			qHead = NULL;
			qLen = 1;
			work_complete = 0;

			if (setjmp(exit_jump) == 0) {
				fetch_response = fetch();
				FAIL("fetch should have called exit() but didn't");
			} else {
				SUCCEED("Captured expected exit() call due to null qHead failure");
			}

			REQUIRE(exit_status == 1);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Queue failure, null qHead with " + std::to_string(qLen) + " items in list"));
		}

		pthread_mutex_unlock(&qLock);
	}
#endif

	SECTION("make_connection function")
	{
		int ret;

		err_log_lines.clear();
		exit_status = 0;

		SECTION("Unix domain socket")
		{
			std::string socket_path = std::string(P_tmpdir) + "/renderd.sock";

			SECTION("make_connection with unix domain socket", "should return positive")
			{
				pass_next_connect = true;

				ret = make_connection(socket_path.c_str());

				REQUIRE(ret > 0);
			}

			SECTION("make_connection handles socket failure by exiting", "should exit 2")
			{
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

		SECTION("Network socket")
		{
			std::string socket_path = "127.0.0.1:1234";

			SECTION("make_connection with network socket", "should return positive")
			{
				pass_next_connect = true;

				ret = make_connection(socket_path.c_str());

				REQUIRE(ret > 0);
				REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Connecting to 127.0.0.1, port 1234/tcp"));
				REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Trying 127.0.0.1:1234"));
				REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Connected to 127.0.0.1:1234"));
			}
		}
	}

// Tests need some work for FreeBSD & macOS
#if !defined(__FreeBSD__) && !defined(__MACH__)
	SECTION("thread_main function")
	{
		void *ret;
		std::string socket_path = std::string(P_tmpdir) + "/renderd.sock";

		err_log_lines.clear();
		exit_status = 0;

		SECTION("thread_main with unix domain socket", "should return")
		{
			ret = thread_main((void *)socket_path.c_str());

			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("connect failed for: " + socket_path));
		}

		pass_next_connect = true;

		SECTION("thread_main with passing connect", "should return")
		{
			qLen = 0;
			work_complete = 1;

			ret = thread_main((void *)socket_path.c_str());
			REQUIRE(ret == NULL);
		}

		char *mapname = strdup("mapname");

		qHead = new qItem{mapname, 1024, 1024, 10, NULL};
		qLen = 1;
		work_complete = 0;

		SECTION("thread_main with passing connect and failing process", "should not return after 30 seconds")
		{
			pthread_t worker;
			pthread_create(&worker, NULL, thread_main, (void *)socket_path.c_str());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			pthread_cond_broadcast(&qCondNotEmpty);

			std::this_thread::sleep_for(std::chrono::milliseconds(30000));
			pthread_cancel(worker);
			pthread_join(worker, (void **)ret);
			REQUIRE(qLen == 0);
			REQUIRE(qHead == NULL);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("connection to renderd lost"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("sleeping for 30 seconds"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("attempting to reconnect"));
		}

		SECTION("thread_main with passing connect and passing process", "should return")
		{
			fail_next_next_recv_reponse_size = 64;
			fail_next_next_recv_reponse_version = 3;
			fail_next_recv_reponse_cmd = cmdDone;
			fail_next_recv_reponse_size = sizeof(struct protocol) - 64;
			fail_next_recv_reponse_version = 3;

			pthread_t worker;
			pthread_create(&worker, NULL, thread_main, (void *)socket_path.c_str());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			pthread_cond_broadcast(&qCondNotEmpty);

			pthread_cancel(worker);
			pthread_join(worker, (void **)ret);
			REQUIRE(qLen == 0);
			REQUIRE(qHead == NULL);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Got response"));
		}
	}
#endif

	SECTION("spawn_workers function")
	{
		int thread_num = 2;
		std::string socket_path = std::string(P_tmpdir) + "/renderd.sock";

		err_log_lines.clear();
		exit_status = 0;

		SECTION("spawn_workers with non-existent unix domain socket", "should return")
		{
			spawn_workers(thread_num, socket_path.c_str(), maxLoad);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Starting " + std::to_string(thread_num) + " rendering threads"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("connect failed for: " + socket_path));
		}

		SECTION("spawn_workers handles calloc failure by exiting", "should exit 1")
		{
			fail_next_calloc = true;

			if (setjmp(exit_jump) == 0) {
				spawn_workers(thread_num, socket_path.c_str(), maxLoad);
				FAIL("spawn_workers should have called exit() but didn't");
			} else {
				SUCCEED("Captured expected exit() call due to calloc failure");
			}

			REQUIRE(exit_status == 1);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Starting " + std::to_string(thread_num) + " rendering threads"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Error allocating worker memory: Cannot allocate memory"));
		}

		SECTION("spawn_workers handles pthread_create failure by exiting", "should exit 1")
		{
			fail_next_pthread_create = true;

			if (setjmp(exit_jump) == 0) {
				spawn_workers(thread_num, socket_path.c_str(), maxLoad);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				FAIL("spawn_workers should have called exit() but didn't");
			} else {
				SUCCEED("Captured expected exit() call due to pthread_create failure");
			}

			REQUIRE(exit_status == 1);
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Starting " + std::to_string(thread_num) + " rendering threads"));
			REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Thread creation failed: Resource temporarily unavailable"));
		}
	}
}
