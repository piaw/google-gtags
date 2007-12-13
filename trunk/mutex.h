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
// A lightweight mutex wrapper for pthreads.

#ifndef TOOLS_TAGS_MUTEX_H__
#define TOOLS_TAGS_MUTEX_H__

#include <pthread.h>

#include "tagsutil.h"

// Undefine the MutexLock macro, which is defined in base/mutex.h
// They use a macro definition that evaluates to a compiler error to protect
// against creating MutexLocks that aren't stored in a variable (not sure why).
// That definition prevents us from creating our own MutexLock even though it's
// in our own namespace
#undef MutexLock

namespace gtags {

class Mutex {
 public:
  Mutex() { pthread_mutex_init(&mutex_, NULL); }
  ~Mutex() { pthread_mutex_destroy(&mutex_); }
  inline void Lock() { pthread_mutex_lock(&mutex_); }
  inline void Unlock() { pthread_mutex_unlock(&mutex_); }
  inline bool TryLock() { return pthread_mutex_trylock(&mutex_) == 0; }
 private:
  pthread_mutex_t mutex_;
  DISALLOW_EVIL_CONSTRUCTORS(Mutex);
};

class MutexLock {
 public:
  explicit MutexLock(Mutex *mu) : mu_(mu) { mu_->Lock(); }
  ~MutexLock() { mu_->Unlock(); }
 private:
  Mutex * const mu_;
  DISALLOW_EVIL_CONSTRUCTORS(MutexLock);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_MUTEX_H__
