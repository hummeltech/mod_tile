#include <cstdint>
#include <cstring>
#include <stdlib.h>
#include <string>
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
#include "parameterize_style.hpp"

#ifndef MAPNIK_XML
#define MAPNIK_XML "./utils/example-map/mapnik.xml"
#endif

#ifndef MAPNIK_PLUGINS_DIR
#define MAPNIK_PLUGINS_DIR "/usr/local/lib64/mapnik/input"
#endif

extern bool fail_next_strtok;
extern std::string err_log_lines;

#define g_logger mocked_g_logger
#define strtok mocked_strtok

#include "parameterize_style.cpp"

#undef g_logger
#undef strtok

TEST_CASE("parameterize_map_language function failure exit handling", "[parameterize_map_language]")
{
	const char * parameter = "en,de,_";
	mapnik::datasource_cache::instance().register_datasources(MAPNIK_PLUGINS_DIR);
	mapnik::Map map(256, 256);
	mapnik::load_map(map, MAPNIK_XML);
	mapnik::layer layer = map.get_layer(0);
	map.remove_all();
	mapnik::parameters parameters = layer.datasource()->params();
	parameters["table"] = ",name";
	layer.set_datasource(mapnik::datasource_cache::instance().create(parameters));
	map.add_layer(layer);

	err_log_lines.clear();

	SECTION("parameterize_map_language handles strtok failure", "should return") {
		fail_next_strtok = true;

		parameterize_map_language(map, (char *)parameter);

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Internationalizing map to language parameter: en,de,_"));
	}

	SECTION("parameterize_map_language modifies 'table' parameter", "should return") {
		layer = map.get_layer(0);
		REQUIRE(layer.datasource()->params().get<std::string>("table") == std::string(",name"));

		parameterize_map_language(map, (char *)parameter);

		layer = map.get_layer(0);
		REQUIRE(layer.datasource()->params().get<std::string>("table") != std::string(",name"));

		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Internationalizing map to language parameter: en,de,_"));
	}
}

TEST_CASE("init_parameterization_function function", "[init_parameterization_function]")
{
	err_log_lines.clear();

	SECTION("init_parameterization_function with empty function_name", "should return NULL") {
		parameterize_function_ptr response = init_parameterization_function("");

		REQUIRE(response == NULL);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Parameterize_style not specified (or empty string specified)"));
	}

	SECTION("init_parameterization_function with non-'language' function_name", "should return NULL") {
		parameterize_function_ptr response = init_parameterization_function("doesnotexist");

		REQUIRE(response == NULL);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("unknown parameterization function for 'doesnotexist'"));
	}

	SECTION("init_parameterization_function with 'language' function_name", "should return parameterize_map_language") {
		parameterize_function_ptr response = init_parameterization_function("language");

		REQUIRE(response == parameterize_map_language);
		REQUIRE_THAT(err_log_lines, Catch::Matchers::Contains("Loading parameterization function for 'language'"));
	}
}
