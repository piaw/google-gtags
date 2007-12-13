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

// The boost libraries aren't as decoupled as gunit, so we just include the
// mixer service test implementation directly rather than linking to it.
#include "mixer_service_test.cc"

#include "socket_mixer_service.h"

#include "socket_util.h"

namespace gtags {

TEST(SocketMixerServiceTest, ServiceTest) {
  const int port = FindAvailablePort();
  SocketMixerServiceProvider mixer_provider(port);
  RunServiceTest(&mixer_provider, port);
}

}  // namespace gtags
