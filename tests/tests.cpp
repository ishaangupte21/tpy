/*
    Catch2 based testing rig.
*/
#define CATCH_CONFIG_MAIN

#include "catch2/catch_test_macros.hpp"
#include "tpy/source/SourceManager.h"

TEST_CASE("Source Location is being tested", "[src_location]") {
    tpy::Source::SourceManager src_mgr;

    SECTION("Source locations for one line and no unicode") {
        auto &src_file =
            src_mgr.open_py_src_file("./tests/source_location/one_line.py");

        auto src_loc = src_file.get_loc_from_pos(14);
        REQUIRE(src_loc.line == 1);
        REQUIRE(src_loc.col == 15);
    }

    SECTION("Source locations for multiple lines and no unicode") {
        auto &src_file = src_mgr.open_py_src_file(
            "./tests/source_location/multiple_lines.py");

        auto src_loc = src_file.get_loc_from_pos(15);
        REQUIRE(src_loc.line == 2);
        REQUIRE(src_loc.col == 6);
    }

    SECTION("Source locations for unicode") {
        auto &src_file =
            src_mgr.open_py_src_file("./tests/source_location/unicode.py");

        auto src_loc = src_file.get_loc_from_pos(5);
        REQUIRE(src_loc.line == 1);
        REQUIRE(src_loc.col == 5);
    }

    SECTION("Source locations with a UTF-8 BOM") {
        auto &src_file =
            src_mgr.open_py_src_file("./tests/source_location/utf8_bom.py");

        auto src_loc = src_file.get_loc_from_pos(3);
        REQUIRE(src_loc.line == 1);
        REQUIRE(src_loc.col == 1);

        auto src_loc_2 = src_file.get_loc_from_pos(25);
        REQUIRE(src_loc_2.line == 2);
        REQUIRE(src_loc_2.col == 1);
    }

    SECTION("Source Locations without local file") {
        src_mgr.open_py_src_file("./tests/source_location/one_line.py");
        src_mgr.open_py_src_file("./tests/source_location/multiple_lines.py");
        src_mgr.open_py_src_file("./tests/source_location/unicode.py");
        src_mgr.open_py_src_file("./tests/source_location/utf8_bom.py");

        auto src_loc_1 = src_mgr.get_loc_from_pos(20);
        REQUIRE(src_loc_1.line == 1);
        REQUIRE(src_loc_1.col == 1);

        auto src_loc_2 = src_mgr.get_loc_from_pos(49);
        REQUIRE(src_loc_2.line == 1);
        REQUIRE(src_loc_2.col == 5);

        auto src_loc_3 = src_mgr.get_loc_from_pos(50);
        REQUIRE(src_loc_3.line == 1);
        REQUIRE(src_loc_3.col == 6);

        auto src_loc_4 = src_mgr.get_loc_from_pos(34);
        REQUIRE(src_loc_4.line == 2);
        REQUIRE(src_loc_4.col == 5);

        auto src_loc_5 = src_mgr.get_loc_from_pos(56);
        REQUIRE(src_loc_5.line == 1);
        REQUIRE(src_loc_5.col == 3);
    }
}