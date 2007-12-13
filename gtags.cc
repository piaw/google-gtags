// Copyright 2004 Google Inc.
// All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// Authors: piaw@google.com (Piaw Na), psung@google.com (Phil Sung)
//
// Google Tags Server.
//
// Loads GTags format files (see wiki/Nonconf/GTagsTagsFormat for
// spec) and presents a network service to {emacs,vi,sh,python}
// clients.
//
// The network protocol is described at wiki/Nonconf/GTagsProtocol,
// but we also support the legacy protocol, in which each request is a
// one-character opcode (one of :;$!#@) followed by a tag or a
// path. For either protocol, the client makes a request, we return a
// list of matching tags, then we close the connection. (One tag per
// connection!)
//
// gtags.cc only handles network I/O at a low level. It passes control
// to TagsRequestHandler, which provides a method to transform input
// command strings to output strings.

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string>
#include <list>
#include <set>

#include "socket_server.h"
#include "stderr_logger.h"

#include "file.h"
#include "tagsoptionparser.h"
#include "tagsrequesthandler.h"

DEFINE_STRING(tags_file, "", "The file containing the tags information.");

DEFINE_STRING(logsaver_prefix, "alloc/gtags.queries.",
              "The directory in which to save important logs so that "
              "the logsaver can write them to gfs.");

// Enables lookup of tags by file. This may use a significant amount
// of space.
DEFINE_BOOL(fileindex, true, "Enable fileindex");

DEFINE_BOOL(gunzip, false, "Stream input file through gunzip");

DEFINE_STRING(corpus_root, "google3",
              "Root of the GTags corpus in Perforce (e.g. google3 or "
              "googleclient/wireless).");

using gtags::File;

GtagsLogger* logger;

// We only deal with the tags file through this TagsRequestHandler.
TagsRequestHandler* tags_request_handler;

// We serve Google Tags
int main(int argc, char **argv) {
  File::Init();
  ParseArgs(argc, argv);

  // tags_file is required in remote mode.
  if (GET_FLAG(tags_file) == "") {
    SetUsage("Usage: gtags --tags_file=<tagfile> ...");
    ShowUsage(argv[0]);
    // Exit if tags_file is not specified
    return -1;
  }

  logger = new StdErrLogger();

  tags_request_handler =
      new SingleTableTagsRequestHandler(GET_FLAG(tags_file),
                                        GET_FLAG(fileindex),
                                        GET_FLAG(gunzip),
                                        GET_FLAG(corpus_root));

  SocketServer tags_server(tags_request_handler);

  tags_server.Loop();

  delete tags_request_handler;
  delete logger;
}
