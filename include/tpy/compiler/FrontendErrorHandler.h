/*
    This file defines the interface that will be used to report errors from the
   frontend of the compiler.
*/

#ifndef TPY_COMPILER_FRONTENDERRORHANDLER_H
#define TPY_COMPILER_FRONTENDERRORHANDLER_H

#include "tpy/source/SourceFile.h"

namespace tpy::Compiler {
class FrontendErrorHandler {

    // This member tracks whether the compiler frontend has encountered an error
    // at all during the entire phase. If errors have been encountered,
    // compilation will stop at the end of each phase.

    static inline bool has_seen_error = false;

  public:
    static auto
    report_error_with_local_pos(Source::SourceFile *src_file,
                                size_t pos, size_t len,
                                const char *msg) -> void;

    static auto error() -> bool { return has_seen_error; }
};
} // namespace tpy::Compiler

#endif