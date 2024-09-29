/*
    This is the main entry point for the tpy interpreter.
*/
#include <cstdio>

#include "tpy/source/SourceManager.h"

int main(int argc, char *argv[]) {
    tpy::Source::SourceManager src_mgr;

    auto src_file = src_mgr.open_py_src_file(argv[1]);

    // auto loc = src_file.get_loc_from_pos(13);
    try {
        auto loc = src_file->get_loc_from_pos(9);

        printf("line: %llu --> col: %llu\n", loc.line, loc.col);
    } catch (std::exception &e) {
        puts(e.what());
    }

    return 0;
}