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
// A PollServer that polls a list of Pollables, notifying them when their file
// descriptors are ready for reading/writing so that they can read and/or write
// without blocking.
//
// To do some work through each iteration of the PollServer's loop, set a loop
// callback.  This will be called at the end of a loop iteration.
//
// To use a PollServer, first create a PollServer, then create all your
// Pollables, passing them a reference to your PollServer in their constructors.
// To start the PollServer, call Loop() on your PollServer.  This should be the
// last thing you do as Loop() will never return on its own.  The only way to
// cause Loop() to terminate is with a call to ForceLoopExit().
//
// Example:
//   int main(int argc, char** argv) {
//     PollServer my_pollserver(3);
//
//     // Acquire some file descriptors for fd1, fd2, & fd3.
//     int fd1 = ...,
//         fd2 = ...,
//         fd3 = ...;
//
//     // Create Pollables to use the PollServer
//     MyPollable
//         my_pollable1(fd1, &my_pollserver),
//         my_pollable2(fd2, &my_pollserver),
//         my_pollable3(fd3, &my_pollserver);
//
//     // Run forever (or at least until ForceLoopExit())
//     my_pollserver.Loop();
//   }

#ifndef TOOLS_TAGS_POLLSERVER_H__
#define TOOLS_TAGS_POLLSERVER_H__

#include "tagsutil.h"

const int kDefaultPollTimeout = 5000;

struct pollfd;

namespace gtags {

template<typename T> class Callback0;
class Pollable;

// Manages all Pollables, notifying them when they can read or write without
// blocking.
class PollServer {
  typedef gtags::Callback0<void> LoopCallback;

 public:
  // Allocates enough space for max_fds.
  PollServer(int max_fds);
  virtual ~PollServer();

  // Adds pollable to the managed Pollables.
  // If the pollable's fd is already registered, overwrites that registration
  // with this one.
  virtual void Register(Pollable *pollable);
  // Removes pollable from the managed Pollables.
  // Fails if PollServer is currently processing an fd other than this
  // pollable's fd.
  // Returns true iff the pollable and it's fd were registered
  virtual bool Unregister(const Pollable *pollable);
  // Returns true iff the fd is registered.
  virtual bool IsRegistered(int fd) const {
    return LastIndexOf(fd) >= 0;
  }
  // Returns true iff the pollable and it's fd were registered.
  virtual bool IsRegistered(const Pollable *pollable) const;

  // Runs forever, repeatedly calling LoopOnce().
  // Can be prematurely terminated by calling ForceLoopExit().
  virtual void Loop();

  // Causes Loop() to to terminate on it's next iteration.
  virtual void ForceLoopExit() {
    loop_ = false;
  }

  // If the specified callback is repeatable, deletes the previous one and sets
  // the loop callback.
  // Otherwise, deletes loop_callback.
  // The loop callback will be deleted automatically by PollServer.
  virtual void set_loop_callback(LoopCallback *loop_callback);

 protected:
  // Polls all the managed Pollables, notifying them (by calling HandleRead
  // and/or HandleWrite) when they can read/write.
  // Also executes the loop callback (if it is set).
  // A timeout can be specified to allow for faster unittests.
  virtual void LoopOnce(int timeout = kDefaultPollTimeout);

  virtual void HandlePollEvents(int num_events);

  virtual int LastIndexOf(int fd) const;

  virtual void DoubleCapacity();

  // Indicates whether or not the PollServer should continue looping.
  bool loop_;

  struct pollfd *fds_;
  Pollable* *pollables_;
  int max_fds_;
  int num_fds_;
  LoopCallback *loop_callback_;

  // The fd that is currently being processed.
  // Set to -1 when execution is outside LoopOnce.
  // This prevents pollables from unregistering each other during the poll loop.
  int current_fd_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PollServer);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_POLLSERVER_H__
