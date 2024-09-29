/*
    This file implements the interface for reporting errors within the frontend
   of the compiler.
*/

#include <cstdio>

#include "tpy/compiler/FrontendErrorHandler.h"

namespace tpy::Compiler {
/*
    This method will report error messages to the user. Here, we must provide
   the user with a good error message and an accurate source location.
*/
auto FrontendErrorHandler::report_error_with_local_pos(
    Source::SourceFile *src_file, size_t pos, size_t len,
    const char *msg) -> void {
    // Tell the frontend that we have seen errors.
    has_seen_error = true;

    // First, we must get the source location of the desired position.
    auto src_loc = src_file->get_loc_from_pos(pos);

    fprintf(stderr, "error: %s\n --> %s at line %llu, col %llu\n", msg,
            src_file->path.c_str(), src_loc.line, src_loc.col);

    putchar('\n');
}
} // namespace tpy::Compiler