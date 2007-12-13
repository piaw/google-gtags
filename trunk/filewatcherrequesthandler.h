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
// Since watching an entire directory tree involves tranversing through all of
// the sub dirs (a lot of disk i/o on NFS), we will do the actual work in a
// separate worker thread.
// We communicate with the worker thread via a producer/consumer queue.

#ifndef TOOLS_TAGS_FILEWATCHERREQUESTHANDLER_H__
#define TOOLS_TAGS_FILEWATCHERREQUESTHANDLER_H__

#include <list>

#include "thread.h"
#include "strutil.h"

class InotifyFileWatcher;
class LocalTagsRequestHandler;

namespace gtags {
struct WatcherCommand;
template<typename T> class ProducerConsumerQueue;
}
typedef gtags::ProducerConsumerQueue<struct gtags::WatcherCommand*>
WatcherCommandPCQueue;

namespace gtags {

// Enum and struct for passing information between FileWatcherRequestHandler and
// FileWatcherRequestWorker.
enum WatcherCommandCode { ADD, REMOVE, EXCLUDE, REMOVE_EXCLUDE };
struct WatcherCommand {
  WatcherCommandCode code;
  string directory;
};

class FileWatcherRequestHandler {
 public:
  FileWatcherRequestHandler(WatcherCommandPCQueue *pc_queue)
      : pc_queue_(pc_queue) {}
  virtual ~FileWatcherRequestHandler() {}

  virtual void Add(const list<string> &dirs,
                   const list<string> &excludes) {
    Push(dirs, excludes, ADD);
  }

  virtual void Remove(const list<string> &dirs,
                      const list<string> &excludes) {
    Push(dirs, excludes, REMOVE);
  }

  virtual void Push(const list<string> &dirs,
                    const list<string> &excludes,
                    WatcherCommandCode code);

 protected:
  virtual void PushDirectories(const list<string> &dirs,
                               WatcherCommandCode code);
  virtual void PushDirectory(const string &dir, WatcherCommandCode code);

  WatcherCommandPCQueue *pc_queue_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FileWatcherRequestHandler);
};

// Worker thread for FileWatcherRequestHandler.
class FileWatcherRequestWorker : public Thread {
 public:
  FileWatcherRequestWorker(InotifyFileWatcher* watcher,
                           WatcherCommandPCQueue* pc_queue,
                           LocalTagsRequestHandler* def_handler,
                           LocalTagsRequestHandler* call_handler) :
      watcher_(watcher), pc_queue_(pc_queue), def_handler_(def_handler),
      call_handler_(call_handler) {
    SetJoinable(true);
  }

  virtual ~FileWatcherRequestWorker() {}

 protected:
  // For each directory in the queue, we add it to the watcher recursively.
  virtual void Run();

  InotifyFileWatcher* watcher_;
  WatcherCommandPCQueue* pc_queue_;
  LocalTagsRequestHandler* def_handler_;
  LocalTagsRequestHandler* call_handler_;

  DISALLOW_EVIL_CONSTRUCTORS(FileWatcherRequestWorker);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_FILEWATCHERREQUESTHANDLER_H__
