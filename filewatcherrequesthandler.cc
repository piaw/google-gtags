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

#include "filewatcherrequesthandler.h"

#include "filewatcher.h"
#include "pcqueue.h"
#include "tagsrequesthandler.h"

namespace gtags {

// With each Push, user can specify a list of directories to exclude from
// this operation. These exclude commands are local to this request only. We
// register all exclude dirs with the watcher first, perform the operation, and
// then unregister the exclude dirs so they don't affect future operations.
void FileWatcherRequestHandler::Push(const list<string> &dirs,
                                     const list<string> &excludes,
                                     WatcherCommandCode code) {
  // Register list of directory names to ignore.
  PushDirectories(excludes, EXCLUDE);
  // Recursively add or remove directories.
  PushDirectories(dirs, code);
  // Unregister list of directory names to ignore.
  PushDirectories(excludes, REMOVE_EXCLUDE);
}

void FileWatcherRequestHandler::PushDirectories(
    const list<string> &dirs, WatcherCommandCode code) {
  for (list<string>::const_iterator iter = dirs.begin();
       iter != dirs.end(); ++iter) {
    PushDirectory(*iter, code);
  }
}

void FileWatcherRequestHandler::PushDirectory(
    const string &dir, WatcherCommandCode code) {
  // Command will be deallocated by FileWatcherRequestWorker.
  struct WatcherCommand* command = new struct WatcherCommand();
  command->code = code;
  command->directory = dir;
  pc_queue_->Put(command);
}

void FileWatcherRequestWorker::Run() {
  struct WatcherCommand* command;
  while ((command = static_cast<struct WatcherCommand*>(pc_queue_->Get()))) {
    switch (command->code) {
      case EXCLUDE:
        watcher_->AddExcludeDirectory(command->directory.c_str());
        break;
      case REMOVE_EXCLUDE:
        watcher_->RemoveExcludeDirectory(command->directory.c_str());
        break;
      case ADD:
        watcher_->AddDirectoryRecursive(command->directory.c_str());
        break;
      case REMOVE:
        watcher_->RemoveDirectoryRecursive(command->directory.c_str());
        def_handler_->UnloadFilesInDir(command->directory);
        call_handler_->UnloadFilesInDir(command->directory);
        break;
    }
    // Free command
    delete command;
  }
};

}  // namespace gtags
