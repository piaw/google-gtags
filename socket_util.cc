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

#include "socket_util.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include "logging.h"

namespace gtags {

// We'll use ports from kMinPort to kMaxPort
// Ports below that are likely to be reserved.
const int kMinPort = 32678;
const int kMaxPort = 60000;

int FindAvailablePort() {
  // Though we'd like to use ASSERT_*s in this function, gUnit won't allow them
  // in non-void functions.  That's OK because failures here will simply cause
  // test failures later.

  int fd = socket(PF_INET, SOCK_STREAM, 0);

  // Setup the addr
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;

  for (int port = kMinPort; port < kMaxPort; ++port) {
    addr.sin_port = htons(port);

    // Determine availability by binding to the port.
    int result = bind(fd, (struct sockaddr*)&addr,
                  static_cast<socklen_t>(sizeof(addr)));
    if (result != -1) {
      // The port was not in use before.  Although it is in use now, it can be
      // bound again immediately since we haven't started listening on it.
      close(fd);
      LOG(INFO) << "Found available port " << port;
      return port;
    }
  }

  // Couldn't find an unused port.
  close(fd);
  return -1;
}

}  // namespace gtags
