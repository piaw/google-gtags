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

#include "indexagent.h"

#include <fcntl.h>

#include "file.h"
#include "pcqueue.h"
#include "strutil.h"
#include "tagsoptionparser.h"
#include "tagsrequesthandler.h"

DEFINE_STRING(gentags_local,
              "./local_gentags.py",
              "Path to local_gentags");
DEFINE_INT32(index_pending_timer, 100, "Amount of time in ms IndexAgent will "
             "wait for FileWatcher to put additional file requests on the "
             "queue.");
DEFINE_BOOL(index_callgraph, false, "Whether or not to include callgraphs "
            "locally. This is not recommended because callgraphs tend to take "
            "a lot of memory.");

namespace {

void MakeTempFile(string *filename) {
  char* name;
  int fd = -1;
  while (fd == -1) {
    name = tempnam("/tmp", "gtags");
    CHECK(name != NULL) << "Unable to generate a unique temp filename";

    // Record the filename.
    *filename = name;

    // Create the file exclusively to help reduce the chance of someone else
    // using that filename before the indexer gets to it
    fd = open(name, O_CREAT | O_EXCL, 0600);

    // tempnam(..) allocates memory for name using malloc, so we must free the
    // returned name once we're done with it.
    free(name);
  }
  LOG(INFO) << "Temp file " << *filename << " created for indexer output";
  close(fd);
}

};  // namespace.

void gtags::IndexAgent::Run() {
  list<const char*> file_list;
  string def_file;
  string callgraph_file;
  while (true) {
    GetRequests(&file_list);
    LOG(INFO) << "Sending " << file_list.size() << " file(s) to indexer";
    Index(file_list, &def_file, &callgraph_file);

    UpdateLocalServer(definition_handler_, def_file);
    UpdateLocalServer(callgraph_handler_, callgraph_file);
    DoneRequests(&file_list, &def_file, &callgraph_file);
  }
}

void gtags::IndexAgent::GetRequests(list<const char*>* file_list) {
  // Blocking Get.
  const char* file = static_cast<const char*>(request_queue_->Get());
  file_list->push_back(file);

  // Give some time for file watcher to populate the queue with additional
  // requests.
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = GET_FLAG(index_pending_timer);
  nanosleep(&ts, NULL);

  // Try read everything else on the queue without blocking.
  char* f;
  while(request_queue_->TryGet(&f)) {
    file_list->push_back(static_cast<const char*>(f));
  }
}

void gtags::IndexAgent::DoneRequests(list<const char*>* file_list,
                                          string* def_file,
                                          string* callgraph_file) {
  for (list<const char*>::const_iterator i = file_list->begin();
       i != file_list->end();) {
    const char* tmp = *i;
    ++i;
    delete [] tmp;
  }
  file_list->clear();

  File::Delete(*def_file);
  def_file->clear();

  File::Delete(*callgraph_file);
  callgraph_file->clear();
}

void gtags::IndexAgent::UpdateLocalServer(
    LocalTagsRequestHandler* local_handler, const string& filename) {
  if (File::Exists(filename)) {
    LOG(INFO) << "Updating local gtags with file: " << filename;
    local_handler->Update(filename);
  }
}

void gtags::IndexAgent::Index(const list<const char*>& file_list,
                                   string* def_file, string* callgraph_file) {
  vector<string> args;

  // Copy files into a vector string (as expected by SubProcess).
  for (list<const char*>::const_iterator i = file_list.begin();
       i != file_list.end(); ++i) {
    args.push_back(*i);
  }

  // Sort and remove duplicates.
  sort(args.begin(), args.end());
  vector<string>::iterator end_of_unique = unique(args.begin(), args.end());
  args.erase(end_of_unique, args.end());

  // argv[0] needs to be the program name.
  args.insert(args.begin(), GET_FLAG(gentags_local));

  string files = " ";
  for (int i = 1; i < args.size(); ++i)
    files += args[i] + ' ';

  int result;
  string cmd;

  MakeTempFile(def_file);

  cmd = GET_FLAG(gentags_local);
  cmd += " --output_file=";
  cmd += *def_file;
  cmd += files;

  LOG(INFO) << "Indexing tags with " << cmd;
  result = system(cmd.c_str());
  if (result != 0) {
    LOG(WARNING) << "Failed to generate local tags ("
                 << GET_FLAG(gentags_local) << ")";
    File::Delete(*def_file);
    *def_file = "";
  }

  if (GET_FLAG(index_callgraph)) {
    MakeTempFile(callgraph_file);

    cmd = GET_FLAG(gentags_local);
    cmd += " --callgraph --output_file=";
    cmd += *callgraph_file;
    cmd += files;

    LOG(INFO) << "Indexing callgraph with " << cmd;
    result = system(cmd.c_str());
    if (result != 0) {
      LOG(WARNING) << "Failed to generate local tags callgraph ("
                   << GET_FLAG(gentags_local) << ")";
      File::Delete(*callgraph_file);
      *callgraph_file = "";
    }
  }
}
