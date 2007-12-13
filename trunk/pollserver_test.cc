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
#include "pollserver.h"

#include "callback.h"
#include "pollable.h"

namespace gtags {

class CallbackCounter {
 public:
  CallbackCounter() : count_(0) {}
  void Reset() { count_ = 0; }
  void Call() { count_++; }
  static void Ignore() {}
  int count_;
};

class LoopCountingPollServer : public PollServer {
  static const int kShortLoopCount = 5;
  static const int kPollTimeout = 5;

 public:
  LoopCountingPollServer(unsigned int max_fds) : PollServer(max_fds) {}
  virtual ~LoopCountingPollServer() {}

  virtual void ShortLoop() { LoopFor(kShortLoopCount); }
  virtual void LoopFor(int max_count) {
    for (int i = 0; i < max_count; ++i)
      LoopOnce(kPollTimeout);
  }
};

TEST(PollServerTest, RegistrationTest) {
  PollServer pollserver(2);

  {
    Pollable pollable1(0, &pollserver);
    EXPECT_TRUE(pollserver.IsRegistered(&pollable1));

    Pollable pollable2(1, &pollserver);
    EXPECT_TRUE(pollserver.IsRegistered(&pollable2));

    // Unregister pollable1
    EXPECT_TRUE(pollserver.Unregister(&pollable1));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable1));
    EXPECT_TRUE(pollserver.IsRegistered(&pollable2));

    // Unregister pollable1 (no longer registered)
    EXPECT_FALSE(pollserver.Unregister(&pollable1));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable1));
    EXPECT_TRUE(pollserver.IsRegistered(&pollable2));

    // Register pollable3, override pollable2's registration
    Pollable pollable3(1, &pollserver);
    EXPECT_TRUE(pollserver.IsRegistered(&pollable3));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable2));
    EXPECT_TRUE(pollserver.IsRegistered(1));

    // Unregistered pollable2 (no longer registered)
    EXPECT_FALSE(pollserver.Unregister(&pollable2));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable2));
    EXPECT_TRUE(pollserver.IsRegistered(&pollable3));
    EXPECT_TRUE(pollserver.IsRegistered(1));

    // Unregister pollable3
    EXPECT_TRUE(pollserver.Unregister(&pollable3));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable3));
    EXPECT_FALSE(pollserver.IsRegistered(&pollable2));
    EXPECT_FALSE(pollserver.IsRegistered(1));
  }  // guarantee destruction of Pollables before PollServer
}

TEST(PollServerTest, CapacityTest) {
  PollServer pollserver(1);
  // Fill beyond initial capacity
  {
    Pollable pollable1(0, &pollserver);
    Pollable pollable2(1, &pollserver);
    Pollable pollable3(2, &pollserver);
    Pollable pollable4(3, &pollserver);
    Pollable pollable5(4, &pollserver);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(PollServerTest, LoopCallbackTest) {
  LoopCountingPollServer pollserver(0);
  CallbackCounter counter;
  EXPECT_EQ(counter.count_, 0);

  // Non-permanent callback is not called
  pollserver.set_loop_callback(
      CallbackFactory::Create(&counter, &CallbackCounter::Call));
  EXPECT_EQ(counter.count_, 0);
  pollserver.LoopFor(1);
  EXPECT_EQ(counter.count_, 0);

  // Permanent callback is called once
  counter.Reset();
  pollserver.set_loop_callback(
      CallbackFactory::CreatePermanent(&counter, &CallbackCounter::Call));
  EXPECT_EQ(counter.count_, 0);
  pollserver.LoopFor(1);
  EXPECT_EQ(counter.count_, 1);

  // Permanent callback is called once per iteration
  counter.Reset();
  pollserver.set_loop_callback(
      CallbackFactory::CreatePermanent(&counter, &CallbackCounter::Call));
  EXPECT_EQ(counter.count_, 0);
  pollserver.LoopFor(3);
  EXPECT_EQ(counter.count_, 3);
}

}  // namespace gtags
