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

#include "gtagsunit.h"
#include "socket_version_service.h"

#include "socket_util.h"

namespace gtags {

TEST(SocketVersionServiceTest, NoServiceTest) {
  const int port = FindAvailablePort();

  SocketVersionServiceUser version_user(port);
  int version = 0;
  bool success = version_user.GetVersion(&version);

  EXPECT_FALSE(success);
}

TEST(SocketVersionServiceTest, ServiceTest) {
  static const int kVersion = 77;
  const int port = FindAvailablePort();

  SocketVersionServiceProvider version_provider(port, kVersion);
  version_provider.SetJoinable(true);
  version_provider.Start();

  // Busy wait until the provider has started servicing
  while (!version_provider.servicing()) {}

  SocketVersionServiceUser version_user(port);
  int version = 0;
  bool success = version_user.GetVersion(&version);

  EXPECT_TRUE(success);
  EXPECT_EQ(version, kVersion);
}

}
