// Copyright 2007 Google Inc. All Rights Reserved.
// Author: nigdsouza@google.com (Nigel D'souza)
//
// A sample .cc file for local_gentags testing.

#include "tools/tags/logging.h"
#include "tools/tags/tagsoptionparser.h"

void HelloWorld(const char* name) {
  LOG(INFO) << "Hello World, " << name << '\n';
}

int main(int argc, char **argv) {
  ParseArgs(argc, argv);
  HelloWorld(argv[1]);
}
