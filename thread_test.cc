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
#include "thread.h"

namespace {

class UniqueIdTestThread : public gtags::Thread {
 public:
  UniqueIdTestThread() : wait_(true) {}
  bool wait_;
 protected:
  virtual void Run() {
    while (wait_) {}
  }
};

TEST(ThreadTest, UniqueIdTest) {
  UniqueIdTestThread t1, t2;
  t1.SetJoinable(true);
  t2.SetJoinable(true);
  t1.Start();
  t2.Start();

  EXPECT_NE(t1.tid(), t2.tid());

  t1.wait_ = t2.wait_ = false;
  t1.Join();
  t2.Join();
}

class IdTestThread : public gtags::Thread {
 protected:
  virtual void Run() {
    EXPECT_EQ(tid(), pthread_self());
  }
};

TEST(ThreadTest, IdTest) {
  const int kNumIdTests = 20;

  for (int i = 0; i < kNumIdTests; ++i) {
    IdTestThread t;
    t.SetJoinable(true);
    t.Start();
    t.Join();
  }
}

class JoinableTestThread : public gtags::Thread {
 public:
  JoinableTestThread() : done_(false) {}
  bool done_;
 protected:
  virtual void Run() {
    bool before = joinable_;
    SetJoinable(!before);
    EXPECT_EQ(joinable_, before);
    done_ = true;
  }
};

TEST(ThreadTest, JoinableTest) {
  {
    JoinableTestThread t;
    t.Start();
    // can't Join, so busy wait till done
    while (!t.done_) {}
  }

  {
    JoinableTestThread t;
    t.SetJoinable(true);
    t.Start();
    t.Join();
  }
}

static void Increment(int *x) { ++*x; }

TEST(ClosureThreadTest, RunTest) {
  int x = 2;
  gtags::ClosureThread ct(
      gtags::CallbackFactory::CreatePermanent(&Increment, &x));
  ct.SetJoinable(true);
  ct.Start();
  ct.Join();
  EXPECT_EQ(x, 3);
}

}  // namespace
