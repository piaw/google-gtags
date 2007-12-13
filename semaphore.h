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
// A lightweight semaphore wrapper for pthreads.

#ifndef TOOLS_TAGS_SEMAPHORE_H__
#define TOOLS_TAGS_SEMAPHORE_H__

#include <semaphore.h>

#include "tagsutil.h"

namespace gtags {

class Semaphore {
 public:
  Semaphore(unsigned int value) { sem_init(&sem_, 0, value); }
  ~Semaphore() { sem_destroy(&sem_); }
  inline void Lock() { sem_wait(&sem_); }
  inline void Unlock() { sem_post(&sem_); }
  inline bool TryLock() { return sem_trywait(&sem_) == 0; }
 private:
  sem_t sem_;
  DISALLOW_EVIL_CONSTRUCTORS(Semaphore);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_SEMAPHORE_H__
