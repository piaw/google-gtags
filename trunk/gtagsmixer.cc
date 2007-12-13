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

#include "gtagsmixer.h"

#include "sexpression.h"
#include "sexpression_util.h"
#include "sexpression_util-inl.h"
#include "stl_util.h"

using gtags::MutexLock;

void ResultMixer::set_result(const string& result, SourceId id) {
  {
    MutexLock lock(&mu_);
    results_[id] = result;
    failures_[id] = "";
    if (!CheckIfDone()) {
      return;
    }
  }
  delete this;
}

void ResultMixer::set_failure(const string& reason, SourceId id) {
  {
    MutexLock lock(&mu_);
    failures_[id] = reason;
    results_[id] = "()";
    if (!CheckIfDone()) {
      return;
    }
  }
  delete this;
}

void ResultMixer::MixResult(string* output) {
  list<SExpression*> references;  // references to all the objects we create.
  list<const SExpression*> to_join;  // references to tag return values.

  // Parse all the results from the servers and aggregate the return values in
  // a list.
  for (int i = 0; i < num_sources_; i++) {
    SExpression* server_response = SExpression::Parse(results_[i]);
    if (!server_response) {
      LOG(WARNING) << "ill-formed s-expression from server. data="
                   << results_[i];
      continue;
    }
    // Add server_response to references list so we can reclaim the resource
    // later.
    references.push_back(server_response);

    // Extract return value
    const SExpression* value = SExpressionAssocGet(server_response, "value");
    if (value && value->IsList()) {
      to_join.push_back(value);
    }
  }

  if (to_join.size() == 0) {
    // Did not receive any result with a return value. This means either all
    // requests to all sources failed, or the server sent us something without
    // a return value.
    if (failures_[REMOTE].size() > 0) {
      output->append("((error ((message \"");
      output->append(failures_[REMOTE]);
      output->append("\"))))");
    } else {
      output->append(results_[REMOTE]);
    }
  } else {
    output->append("((value ");
    JoinResults(to_join, output);
    output->append("))");
  }

  // delete all SExpression objects we created.
  STLDeleteElementContainer(&references);
}

void ResultMixer::JoinResults(const list<const SExpression*>& sexpressions,
                              string* output) {
  output->append("(");
  for (list<const SExpression*>::const_iterator i = sexpressions.begin();
       i != sexpressions.end();
       ++i) {
    for (SExpression::const_iterator j = (*i)->Begin(); j != (*i)->End(); ++j) {
      output->append(j->Repr());
    }
  }
  output->append(")");
}

// This should only be called in set_result or set_failure which are responsible
// for locking.
bool ResultMixer::CheckIfDone() {
  CHECK(!mu_.TryLock())
      << "Checking if done without first setting a result/failure.";
  waiting_for_--;
  if (waiting_for_ == 0) {
    string mixed_result;
    MixResult(&mixed_result);
    callback_->Run(mixed_result);
    return true;
  }
  return false;
}

void ResultHolder::set_result(const string& result) {
  {
    MutexLock l(&mu_);
    if (!used_) {
      used_ = true;
      LOG(INFO) << "Received result from source(" << id_ << "): " << result;
      mixer_->set_result(result, id_);
    }
    num_waiting_--;
    if (num_waiting_ > 0) {
      return;
    }
  }
  // We can only get here if num_waiting == 0.
  delete this;
}

// Report failure to mixer when called num_conn number of times.
void ResultHolder::set_failure(const string& reason) {
  LOG(WARNING) << "RPC failed: " << reason;
  {
    MutexLock l(&mu_);
    num_waiting_--;
    if (num_waiting_ > 0) {
      return;
    }

    if (!used_) {
      // We can only get here if all connections failed.
      mixer_->set_failure("Failed to connect to remote services.", id_);
    }
  }
  delete this;
}
