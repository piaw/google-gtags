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
// A semaphore-based solution to the standard producer-consumer problem.

#ifndef TOOLS_TAGS_PCQUEUE_H__
#define TOOLS_TAGS_PCQUEUE_H__

#include <deque>

#include "mutex.h"
#include "semaphore.h"
#include "tagsutil.h"

namespace gtags {

template<typename T>
class ProducerConsumerQueue {
 public:
  ProducerConsumerQueue(int capacity) : full_(0), empty_(capacity) {}
  ~ProducerConsumerQueue() {}

  virtual void Put(T elem) {
    empty_.Lock();
    {
      gtags::MutexLock lock(&mu_);
      PutInQueue(elem);
    }
    full_.Unlock();
  }

  virtual T Get() {
    T elem;
    full_.Lock();
    {
      gtags::MutexLock lock(&mu_);
      GetFromQueue(&elem);
    }
    empty_.Unlock();
    return elem;
  }

  virtual bool TryPut(T elem) {
    bool success = false;
    if (empty_.TryLock()) {
      if (mu_.TryLock()) {
        PutInQueue(elem);
        mu_.Unlock();
        success = true;
      }
      full_.Unlock();
    }
    return success;
  }

  virtual bool TryGet(T *elem) {
    bool success = false;
    if (full_.TryLock()) {
      if (mu_.TryLock()) {
        GetFromQueue(elem);
        mu_.Unlock();
        success = true;
      }
      empty_.Unlock();
    }
    return success;
  }

  int count() {
    gtags::MutexLock lock(&mu_);
    return queue_.size();
  }

 private:
  void PutInQueue(T elem) {
    queue_.push_back(elem);
  }

  void GetFromQueue(T *elem) {
    *elem = queue_.front();
    queue_.pop_front();
  }

  deque<T> queue_;
  gtags::Mutex mu_;
  Semaphore full_;
  Semaphore empty_;
  DISALLOW_EVIL_CONSTRUCTORS(ProducerConsumerQueue);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_PCQUEUE_H__
