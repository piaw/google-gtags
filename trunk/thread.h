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
// A lightweight thread wrapper for pthreads.

#ifndef TOOLS_TAGS_THREAD_H__
#define TOOLS_TAGS_THREAD_H__

#include <pthread.h>

#include "callback.h"
#include "tagsutil.h"

namespace gtags {

class Thread {
 public:
  Thread(bool joinable = false) : started_(false), joinable_(joinable) {}
  virtual ~Thread() {}

  pthread_t tid() const { return tid_; }

  void SetJoinable(bool joinable) {
    if (!started_)
      joinable_ = joinable;
  }

  void Start() {
    pthread_attr_t attr;
    CHECK_EQ(pthread_attr_init(&attr), 0);
    CHECK_EQ(
        pthread_attr_setdetachstate(
            &attr,
            joinable_ ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED),
        0);

    int result = pthread_create(&tid_, &attr, &ThreadRunner, this);
    CHECK_EQ(result, 0) << "Could not create thread (" << result << ")";

    CHECK_EQ(pthread_attr_destroy(&attr), 0);

    started_ = true;
  }

  void Join() {
    CHECK(joinable_) << "Thread is not joinable";
    int result = pthread_join(tid_, NULL);
    CHECK_EQ(result, 0) << "Could not join thread (" << result << ")";
  }

 protected:
  virtual void Run() = 0;

  static void* ThreadRunner(void* arg) {
    Thread *t = reinterpret_cast<Thread*>(arg);
    t->Run();
    return NULL;
  }

  pthread_t tid_;
  bool started_;
  bool joinable_;
};

class ClosureThread : public gtags::Thread {
 public:
  ClosureThread(gtags::Closure *closure) : closure_(closure) {
    CHECK(closure->IsRepeatable());
  }
  virtual ~ClosureThread() { delete closure_; }
 protected:
  virtual void Run() {
    closure_->Run();
  }
  gtags::Closure *closure_;
};

}  // namespace

#endif  // TOOLS_TAGS_THREAD_H__
