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
// The Pollable interface to be implemented for use with a PollServer.
//
// Example Pollable:
//   class MyPollable : public Pollable {
//    public:
//     MyPollable(int fd, PollServer *pollserver) : Pollable(fd, pollserver) {}
//    protected:
//     virtual void HandleRead() {
//       // Perform my reading operations
//     }
//     virtual void HandleWrite() {
//       // Perform my writing operations
//     }
//   };

#ifndef TOOLS_TAGS_POLLABLE_H__
#define TOOLS_TAGS_POLLABLE_H__

#include "tagsutil.h"

namespace gtags {

class PollServer;

// The interface for all Pollable classes in order for them to interact with
// the PollServer
class Pollable {
  friend class PollServer;

 public:
  // Sets this Pollable's file descriptor and Registers with the PollServer
  Pollable(int fd, PollServer *pollserver);
  // Unregisters from this Pollable's PollServer
  virtual ~Pollable();

  virtual int fd() const { return fd_; }

 protected:
  // We provide empty implementations for these handlers because not all
  // Pollables need to read/write.  This allows subclasses to override handlers
  // when needed.
  virtual void HandleRead() {}
  virtual void HandleWrite() {}

  int fd_;
  PollServer *ps_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Pollable);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_POLLABLE_H__
