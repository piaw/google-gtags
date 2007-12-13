// Copyright 2007 Google Inc. All Rights Reserved.
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
// Author: nigdsouza@google.com (Nigel D'souza)
//
// Execution entry point for gtagswatcher.

#include <unistd.h>
#include <algorithm>

#include <list>

#include "file.h"
#include "socket_filewatcher_service.h"
#include "tagsoptionparser.h"

DEFINE_INT32(port, 2222, "rpc port for communication with GTags mixer.");
DEFINE_MULTISTRING(add, list<string>(),
                   "List of directories to add to the GTags mixer's watch"
                   "list.");
DEFINE_MULTISTRING(remove, list<string>(),
                   "List of directories to remove from the GTags mixer's watch"
                   "list.");
DEFINE_MULTISTRING(excludes, list<string>(1, string("genfiles")),
                   "List of directories to exclude from add/remove"
                   " operations.");

int main(int argc, char **argv) {
  ParseArgs(argc, argv);

  // Nothing to do.
  if (GET_FLAG(add).size() == 0 && GET_FLAG(remove).size() == 0) {
    SetUsage("Usage: gtagswatcher_cc --add /dir1 --add /dir2"
             " --exclude /dir1/dir3");
    ShowUsage(argv[0]);
    return -1;
  }

  LOG(INFO) << "add dirs: " << GET_FLAG(add).size();
  for (list<string>::const_iterator iter = GET_FLAG(add).begin();
       iter != GET_FLAG(add).end(); ++iter) {
    LOG(INFO) << "  " << *iter;
  }

  gtags::FileWatcherServiceUser *filewatcher_user =
      new gtags::SocketFileWatcherServiceUser(GET_FLAG(port));

  if (GET_FLAG(add).size() > 0) {
    filewatcher_user->Add(GET_FLAG(add), GET_FLAG(excludes));
  }

  delete filewatcher_user;

  return 0;
}
