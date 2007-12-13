// Fake cpp file to verify that etags_to_tags can open it and find the
// includes.

#include "tools/cpp/file3.h"

// This is a malformed include. It should be ignored and no
// corresponding (include ...) descriptor should be generated.
#include "tools/tags/file1.h>

// This should create an entry as if we defined a variable named
// FLAGS_sample_flag
DEFINE_bool(sample_flag, false, "");
