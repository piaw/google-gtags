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
// gtagsmixer provides a network interface on user's local machine through which
// a client can talk to GTags servers on mulitple data centers using stubby/RPC.
//
// When an RPC call finishes, the callback will attempt to report the result to
// a ResultHolder object. There is a common ResultHolder object per Datasource
// per request. We are only interested in the fastest result from each
// DataSource, so only the first attempt to set the result on each ResultHolder
// object has any effects. All the results from different ResultHolders are then
// collected in a ResultMixer. ResultMixer is responsibly for mixing and ranking
// results from different DataSources and send them back to the client.
//
// See: gtagsmixermain.cc for details on gtagsmixer's initialization.
// See: mixerconnection.[h|cc] for details on network communication
// with the client.

#ifndef TOOLS_TAGS_GTAGSMIXER_H__
#define TOOLS_TAGS_GTAGSMIXER_H__

#include <list>

#include "callback.h"
#include "mutex.h"
#include "sexpression.h"

using gtags::Mutex;

// We mix tags results from the following sources. The ordering here determines
// ranking.
//
// proto refers to protocol buffer gtags server which we query with every
// request regardless of the language.
//
// remote_query refers to remote gtags servers on borg cell that are identified
// by the query language and caller type.
//
// TODO(stephenchen): We want to provide a web interface to make the ordering
//                    configurable.
enum SourceId { LOCAL, REMOTE, NUM_SOURCES_PER_REQUEST };

// ResultMixer is responsible for ranking and mixing tags results from
// different sources.
//
// When created, TagsMixer takes the number of results that it expects to handle
// as a parameter. Tags results (or failures) are reported by tags result
// holders Once all the responses arrived, ResultMixer will unpack all the
// results and recombine them. The ranking rule is based on the ordering of the
// sources in SourceId list.
//
// After mixing is done, it invokes a specified callback and self destructs.
class ResultMixer {
 protected:
  typedef gtags::Callback1<void, const string&> DoneCallback;

 public:
  ResultMixer(int num_sources, DoneCallback* callback)
     : num_sources_(num_sources), waiting_for_(num_sources),
       callback_(callback) {
    results_ = new string[num_sources];
    failures_ = new string[num_sources];
  }

  ~ResultMixer() {
    delete [] results_;
    delete [] failures_;
  }

  // Report result from one source to the mixer. If the caller is the last
  // source the mixer is waiting for, tag results are mixed and callback is
  // invoked.
  virtual void set_result(const string& result, SourceId id);

  // Report a failure from one source to the mixer. If the caller is the last
  // source the mixer is waiting for, tag results are mixed and callback is
  // invoked.
  virtual void set_failure(const string& reason, SourceId id);

 protected:
  // Mix and rank tag results from different sources.
  // Result is appended to output.
  virtual void MixResult(string* output);
  // Takes a number of SExpression lists and merge them into one list.
  // Result is appended to output.
  virtual void JoinResults(const list<const SExpression*>& sexpressions,
                           string* oupput);
  // Check if we all data from all the sources we need. If so, mix the
  // result and invoke the callback.
  // Note: caller is responsible for locking mu_.
  virtual bool CheckIfDone();

 private:
  // Mutex for thread control.
  Mutex mu_;
  // Number of sources we are expected to mix.
  int num_sources_;
  // Number of outstanding sources.
  int waiting_for_;
  // Function to invoke once we have a merged result.
  DoneCallback* callback_;
  // List of results.
  string* results_;
  // List of failure messages.
  string* failures_;
};

// Thread safe container for tag query result.
//
// ResultHolder is used in the callback function of the TagsService.GetTags
// stubby RPC call. Normally, there are multiple RPC calls happening
// concurrently in separate threads and MixerResultHolder is used to select the
// fastest result. When each RPC call finishes, the callback function will
// invoke set_tag with the result. Only the first call to set_tag triggers a
// response back to the client. Subsequent calls to set_tag have no
// effect. set_tag is guarded using a lock to ensure thread safety.
//
// ResultHolder self destructs when the total number of calls to set_result and
// set_failure reaches num_conn.
class ResultHolder {
 public:
  ResultHolder(SourceId id, int num_conn, ResultMixer* mixer) :
      mixer_(mixer), id_(id), num_conn_(num_conn), num_waiting_(num_conn),
      used_(false) {}

  ~ResultHolder() {}

  // Report result to mixer when called for the first time. Does nothing on
  // subsequent calls.
  void set_result(const string& result);

  // Report failure to mixer when called num_conn number of times.
  void set_failure(const string& reason);

 private:
  ResultMixer* mixer_;
  SourceId id_;
  int num_conn_;
  int num_waiting_;
  bool used_;
  Mutex mu_;
};

#endif  // TOOLS_TAGS_GTAGSMIXER_H__
