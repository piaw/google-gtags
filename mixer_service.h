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
// An abstract interface for accepting connections from a client (e.g. emacs) to
// the mixer.
//
// The MixerServiceProvider should handle accepting connections on a port
// and forwarding them to the MixerRequestHandler for processing.
//
// Testing services usually requires the provider running on one thread while
// the main thread interacts with it.
// The servicing() method lets us know when the provider is actually ready to
// service incoming requests.  (Since the servicing actually requires executing
// a Loop(), servicing() usually indicates when we are ALMOST ready to service
// incoming requests.)
// The Stop() method is used to tell the provider to stop servicing requests.
// (This usually relates to forcefully exiting a permanent loop.)  This allows
// tests to properly clean up after themselves and avoids heap checkers
// complaining about memory leaks.

#ifndef TOOLS_TAGS_MIXER_SERVICE_H__
#define TOOLS_TAGS_MIXER_SERVICE_H__

class MixerRequestHandler;

namespace gtags {

class MixerServiceProvider {
 public:
  MixerServiceProvider(int port) : port_(port), servicing_(false) {}
  virtual ~MixerServiceProvider() {}

  virtual bool servicing() { return servicing_; }

  // Starts the mixer service on port_.
  virtual void Start(MixerRequestHandler *handler) = 0;

  // Stops the mixer service.
  virtual void Stop() = 0;

 protected:
  int port_;
  bool servicing_;
};

}  // namespace gtags

#endif  // TOOLS_TAGS_MIXER_SERVICE_H__
