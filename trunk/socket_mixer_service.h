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

#ifndef TOOLS_TAGS_SOCKET_MIXER_SERVICE_H__
#define TOOLS_TAGS_SOCKET_MIXER_SERVICE_H__

#include "mixer_service.h"

#include "tagsutil.h"

namespace gtags {

class PollServer;

class SocketMixerServiceProvider : public MixerServiceProvider {
 public:
  SocketMixerServiceProvider(int port)
      : MixerServiceProvider(port), pollserver_(NULL) {}

  virtual void Start(MixerRequestHandler *handler);
  virtual void Stop();

 protected:
  PollServer *pollserver_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SocketMixerServiceProvider);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_SOCKET_MIXER_SERVICE_H__
