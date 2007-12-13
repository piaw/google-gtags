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
// Execution entry point for gtagsmixer.
// Parse the config file, create data sources and start the server.

#include <unistd.h>
#include <algorithm>

#include "datasource.h"
#include "file.h"
#include "filewatcher.h"
#include "filewatcherrequesthandler.h"
#include "indexagent.h"
#include "mixerrequesthandler.h"
#include "pcqueue.h"
#include "sexpression.h"
#include "tagsoptionparser.h"
#include "tagsrequesthandler.h"

#include "socket_filewatcher_service.h"
#include "socket_mixer_service.h"
#include "socket_version_service.h"

DEFINE_INT32(port, 2220, "Port the mixer is listening on.");
DEFINE_INT32(version_port, 2221, "rpc port for versioning communication.");
DEFINE_INT32(rpc_port, 2222, "rpc port for communication with file watcher.");

DEFINE_BOOL(daemon, true, "Run GTags mixer in daemon mode.");
DEFINE_STRING(config_file,
              "./gtagsmixer_socket_config",
              "User configuration file");
DEFINE_BOOL(fileindex, true, "Enable fileindex");
DEFINE_BOOL(gunzip, false, "Stream input file through gunzip");
DEFINE_BOOL(enable_local_indexing, false, "Enable local indexing");
DEFINE_BOOL(replace, false, "Set this flag to replace any existing instance of"
            " gtagsmixer regardless of its version.");

using gtags::File;
using gtags::Thread;

const int kTagsMixerVersion = 2;

// size of the ProducerConsumerQueue for turning inotify events into RPC calls
// to indexer.
const int kIndexQueueSize = 1000;

// TODO(stephenchen): Move this into an s-expression configuration file.
void AddExtensions(FileExtensionEventFilter* filter) {
  filter->AddExtension(".cc");
  filter->AddExtension(".c");
  filter->AddExtension(".cpp");
  filter->AddExtension(".h");
  filter->AddExtension(".lex");
  filter->AddExtension(".java");
  filter->AddExtension(".py");
}

void StartWatcher(LocalTagsRequestHandler* definition,
                  LocalTagsRequestHandler* callgraph) {
  InotifyFileWatcher watcher;
  FilenamePCQueue index_request_queue(kIndexQueueSize);

  // Event handler for dispatching indexing requests.
  IndexEventHandler index_handler(&watcher, &index_request_queue);
  // Event handler for keep directory watch list in sync with the file system.
  DirectoryTracker tracker(&watcher);

  // Add filters to handlers.
  FileExtensionEventFilter ext_filter;
  AddExtensions(&ext_filter);
  PrefixFilter prefix_filter;
  index_handler.AddFilter(&ext_filter);
  index_handler.AddFilter(&prefix_filter);

  DirectoryEventFilter dir_filter;
  tracker.AddFilter(&dir_filter);

  // Register event handlers with watcher.
  watcher.AddEventHandler(&tracker);
  watcher.AddEventHandler(&index_handler);

  gtags::IndexAgent index_agent(&index_request_queue, definition,
                                     callgraph);
  index_agent.Start();

  WatcherCommandPCQueue pc_queue(100);
  gtags::FileWatcherRequestHandler handler(&pc_queue);

  // Start file watcher service.
  gtags::FileWatcherServiceProvider *filewatcher_provider =
      new gtags::SocketFileWatcherServiceProvider(
          GET_FLAG(rpc_port), &handler);

  filewatcher_provider->SetJoinable(true);
  filewatcher_provider->Start();

  gtags::FileWatcherRequestWorker worker_thread(&watcher, &pc_queue,
                                                definition, callgraph);
  worker_thread.Start();

  watcher.Loop();  // Loop never actually exits.

  // Clean up file watcher service.
  filewatcher_provider->Join();
  delete filewatcher_provider;
}

// Check if there is a running instance of mixer already.
void CheckSingleInstance() {
  gtags::VersionServiceUser* version_user =
      new gtags::SocketVersionServiceUser(GET_FLAG(version_port));

  int version;

  // If there is no running instance, RPC would fail and we just return
  // and start our own mixer.
  if (version_user->GetVersion(&version)) {
    LOG(INFO) << "Detected running GTags mixer version " << version
              << ". Self version is " << kTagsMixerVersion << ".";
    // Otherwise, check who's more up to date.
    if (!GET_FLAG(replace) && version >= kTagsMixerVersion) {
      LOG(INFO) << "Exiting.";
      exit(0);  // running mixer is at least as up to date as us. Exit.
    } else {
      // Shutdown the old mixer.
      LOG(INFO) << "Shutting down running instance.";
      version_user->ShutDown();
    }
  } else {
    LOG(INFO) << "No running GTags mixer detected.";
  }

  delete version_user;
}

int main(int argc, char **argv) {
  ParseArgs(argc, argv);

  if (GET_FLAG(daemon)) {
    daemon(0, 0);
  }

  // This has to come after daemonizing. Otherwise, filewatcher_service won't
  // start.
  CheckSingleInstance();

  File::Init();

  // Load settings from config file.
  Settings::Load(GET_FLAG(config_file));
  const DataSourceMap& sources = Settings::instance()->sources();

  // Create local GTags server.
  LocalTagsRequestHandler local_tags_handler(GET_FLAG(fileindex),
                                             GET_FLAG(gunzip), "");
  LocalDataSource local_data_source(&local_tags_handler);

  LocalTagsRequestHandler local_callgraph_handler(GET_FLAG(fileindex),
                                                  GET_FLAG(gunzip), "");
  LocalDataSource local_callgraph_source(&local_callgraph_handler);

  // Inject local GTags server into sources for all corpuses.
  for (DataSourceMap::const_iterator it = sources.begin();
       it != sources.end(); ++it) {
    Settings::instance()->AddDataSource(
        it->first, "local", pair<DataSource*, DataSource*>(
            &local_data_source, &local_callgraph_source));
  }

  // Start the watcher service.
  Thread* watcher_thread = NULL;
  if (GET_FLAG(enable_local_indexing)) {
    watcher_thread =
      new gtags::ClosureThread(
          gtags::CallbackFactory::CreatePermanent(
              &StartWatcher, &local_tags_handler, &local_callgraph_handler));
    watcher_thread->SetJoinable(true);
    watcher_thread->Start();
  }

  // Start the version service.
  gtags::VersionServiceProvider* version_provider =
      new gtags::SocketVersionServiceProvider(
          GET_FLAG(version_port), kTagsMixerVersion);

  version_provider->SetJoinable(true);
  version_provider->Start();

  // Start the mixer service.
  MixerRequestHandler handler(&sources);
  gtags::MixerServiceProvider *mixer_provider =
      new gtags::SocketMixerServiceProvider(GET_FLAG(port));

  mixer_provider->Start(&handler);  // Never actually exits.

  // Clean up the mixer service.
  delete mixer_provider;

  // Clean up the version service.
  version_provider->Join();
  delete version_provider;

  // Clean up watcher service.
  if (watcher_thread) {
    watcher_thread->Join();
    delete watcher_thread;
  }

  for (DataSourceMap::const_iterator it = sources.begin();
       it != sources.end(); ++it) {
    for(LanguageMap::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); ++it2) {
      // Local data sources are created on stack separately from the remote
      // ones, so don't delete them.
      if (it2->first != "local") {
        if (it2->second.first)
          delete it2->second.first;
        if (it2->second.second)
          delete it2->second.second;
      }
    }
  }
  Settings::Free();

  return 0;
}
