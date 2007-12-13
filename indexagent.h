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
// Author: stephenchen@google.com (Stephen Chen)
//
// IndexAgent is a thread that listens on a ProducerConsumerQueue for index
// requests, invokes indexer on these files, and refresh the local GTags server
// with the indexer output.

#ifndef TOOLS_TAGS_INDEXAGENT_H__
#define TOOLS_TAGS_INDEXAGENT_H__

#include <list>

#include "thread.h"

// Forward declarations.
class LocalTagsRequestHandler;

namespace gtags {
template<typename T> class ProducerConsumerQueue;
}
typedef gtags::ProducerConsumerQueue<char*> FilenamePCQueue;

namespace gtags {

class IndexAgent : public Thread {
 public:
  IndexAgent(FilenamePCQueue* queue,
             LocalTagsRequestHandler* handler,
             LocalTagsRequestHandler* callgraph_handler)
      : request_queue_(queue), definition_handler_(handler),
        callgraph_handler_(callgraph_handler) {}
  virtual ~IndexAgent() {}

  virtual void Run();

  // Read index request from request_queue_ and populate file_list.
  void GetRequests(list<const char*>* file_list);
  // Call indexer files in file_list. Outputs two strings pointing to tags files
  // produced by the indexer.
  virtual void Index(const list<const char*>& file_list, string* def_file,
                     string* callgraph_file);
  // Refresh local GTags server using the file specified by filename.
  virtual void UpdateLocalServer(LocalTagsRequestHandler* local_handler,
                                 const string& filename);
  // Clean up after requests.
  void DoneRequests(list<const char*>* file_list, string* def_file,
                    string* callgraph_file);

 private:
  FilenamePCQueue* request_queue_;
  LocalTagsRequestHandler* definition_handler_;
  LocalTagsRequestHandler* callgraph_handler_;

  DISALLOW_EVIL_CONSTRUCTORS(IndexAgent);
};

}  // namespace tools_tags

#endif  // TOOLS_TAGS_INDEXAGENT_H__
