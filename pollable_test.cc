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
#include "pollable.h"

#include "pollserver.h"

namespace gtags {

class MockPollServer : public PollServer {
 public:
  MockPollServer() : PollServer(1), registered_(false) {}
  virtual ~MockPollServer() {}

  virtual void Register(Pollable *pollable) {
    registered_ = true;
  }
  virtual bool Unregister(const Pollable *pollable) {
    registered_ = false;
    return true;
  }

  bool registered_;
};

TEST(PollableTest, AutoRegistrationTest) {
  MockPollServer mps;
  EXPECT_FALSE(mps.registered_);

  Pollable *p = new Pollable(0, &mps);
  EXPECT_TRUE(mps.registered_);

  delete p;
  EXPECT_FALSE(mps.registered_);
}

}  // namespace
