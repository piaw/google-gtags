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
// The PollServer's arrays need to be dynamically resized as necessary.  This
// requires the use of 'realloc', which means the space must initially be
// allocated through 'malloc' and finally released through 'free'.
//
// Unregistering is done by swapping the pollable to be unregistered with the
// last pollable.  This allows for constant-time removal.  Consequently, the
// arrays are reverse-iterated to ensure that we aren't accessing released
// memory.
//
// While PollServer is handling events, Pollables are only allowed to Unregister
// themselves.  This is to ensure that we aren't accessing invalid indices as we
// iterate through the array of Pollables to dispatch events.

#include "pollserver.h"

#include <poll.h>

#include "callback.h"
#include "pollable.h"

namespace gtags {

PollServer::PollServer(int max_fds)
    : max_fds_(max_fds), loop_callback_(NULL), current_fd_(-1) {
  CHECK_NE(max_fds_, -1);
  fds_ = (struct pollfd*) malloc(sizeof(struct pollfd) * max_fds_);
  pollables_ = (Pollable* *) malloc(sizeof(Pollable*) * max_fds_);
  num_fds_ = 0;
}

PollServer::~PollServer() {
  delete loop_callback_;
  CHECK_EQ(num_fds_, 0)
      << "PollServer closing with " << num_fds_
      << " Pollables still registered";
  free(pollables_);
  free(fds_);
}

void PollServer::Register(Pollable *pollable) {
  int index = LastIndexOf(pollable->fd());

  // If the fd isn't already registered and there's space, set index.
  // Otherwise, we leave index so that this registration overwrites the
  // previous registration.
  if (index == -1) {
    if (num_fds_ == max_fds_)
      DoubleCapacity();
    index = num_fds_;
    ++num_fds_;
  }

  // By now, index is set to the slot where this pollable should be placed.
  fds_[index].fd = pollable->fd();
  fds_[index].events = POLLIN | POLLOUT;
  pollables_[index] = pollable;
}

bool PollServer::Unregister(const Pollable *pollable) {
  // Ensure we are currently processing an fd other than this pollable's fd.
  // This prevents pollables from unregistering other pollables during the loop.
  CHECK(current_fd_ == -1 || pollable->fd() == -1
        || current_fd_ == pollable->fd())
      << "Attempting to unregister fd " << pollable->fd()
      << " while events for fd " << current_fd_ << " are being processed";

  for (int i = num_fds_-1; i >= 0; --i) {
    if (fds_[i].fd == pollable->fd()) {
      // Ignore if this pollable is no longer registered with its fd.
      if (pollables_[i] != pollable)
        return false;

      // Move the last fd/pollable into this vacated spot and clear its revents
      // to avoid sending it any pending events.
      fds_[i].fd = fds_[num_fds_-1].fd;
      pollables_[i] = pollables_[num_fds_-1];
      fds_[i].revents = 0;
      --num_fds_;
      return true;
    }
  }
  return false;
}

bool PollServer::IsRegistered(const Pollable *pollable) const {
  int i = LastIndexOf(pollable->fd());
  return i >= 0 && pollables_[i] == pollable;
}

void PollServer::Loop() {
  loop_ = true;
  while (loop_)
    LoopOnce();
}

void PollServer::LoopOnce(int timeout) {
  int result = poll(fds_, num_fds_, timeout);
  if (result == -1) {
    LOG(WARNING) << "Error occurred while polling";
  } else if (result == 0) {
    // poll timed out
  } else if (result > 0) {
    HandlePollEvents(result);
  }
  if (loop_callback_)
    loop_callback_->Run();
}

void PollServer::set_loop_callback(LoopCallback *loop_callback) {
  if (!loop_callback->IsRepeatable()) {
    delete loop_callback;
    return;
  }

  delete loop_callback_;
  loop_callback_ = loop_callback;
}

void PollServer::HandlePollEvents(int num_events) {
  for (int i = num_fds_-1; i >= 0 && num_events > 0; --i) {
    // Set the current_fd_ to allow the current Pollable to Unregister
    current_fd_ = fds_[i].fd;

    bool had_event = false;

    if (fds_[i].revents & POLLIN) {
      pollables_[i]->HandleRead();
      had_event = true;
    }
    // The pollable could have unregistered during HandleRead so check that the
    // index is still valid.
    if (i < num_fds_ && fds_[i].revents & (POLLOUT | POLLHUP)) {
      pollables_[i]->HandleWrite();
      had_event = true;
    }

    if (had_event)
      --num_events;
  }
  // Reset to -1 to allow Pollables to Unregister when PollServer isn't handling
  // events.
  current_fd_ = -1;
}

int PollServer::LastIndexOf(int fd) const {
  for (int i = num_fds_-1; i >= 0; --i) {
    if (fds_[i].fd == fd)
      return i;
  }
  return -1;
}

void PollServer::DoubleCapacity() {
  LOG(INFO) << "Out of space, doubling capacity.";
  max_fds_ *= 2;
  fds_ = (struct pollfd*) realloc(fds_, sizeof(struct pollfd) * max_fds_);
  pollables_ = (Pollable* *) realloc(pollables_, sizeof(Pollable*) * max_fds_);
}

}  // namespace gtags
