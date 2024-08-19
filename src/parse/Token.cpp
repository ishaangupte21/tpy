/*
    This file implements the object that represents a single token within a
   Python source file, and the list of tokens.
*/
#include "tpy/parse/Token.h"

namespace tpy::Parse {

#define F(x) #x,
extern const char *token_names[] = {TOKEN_LIST(F)};
#undef F

} // namespace tpy::Parse