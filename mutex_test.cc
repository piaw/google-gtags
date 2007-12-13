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
#include "mutex.h"

#include "thread.h"

namespace {

TEST(MutexTest, TryLockTest) {
  {
    gtags::Mutex m;
    EXPECT_TRUE(m.TryLock());
    m.Unlock();
  }
  {
    gtags::Mutex m;
    m.Lock();
    EXPECT_FALSE(m.TryLock());
    m.Unlock();
  }
}

TEST(MutexTest, UnlockTest) {
  {
    gtags::Mutex m;
    EXPECT_TRUE(m.TryLock());
    m.Unlock();
  }
  {
    gtags::Mutex m;
    m.Lock();
    m.Unlock();
    EXPECT_TRUE(m.TryLock());
    m.Unlock();
  }
  {
    gtags::Mutex m;
    m.Unlock();
    m.Unlock();
    m.Unlock();
    EXPECT_TRUE(m.TryLock());
    EXPECT_FALSE(m.TryLock());
    m.Unlock();
  }
}

class IncrementThread : public gtags::Thread {
 public:
  IncrementThread(int *x, int count, gtags::Mutex *m) :
      Thread(true), x_(*x), count_(count), m_(m) {}
 protected:
  virtual void Run() {
    for (int i = 0; i < count_; ++i) {
      if (m_) m_->Lock();
      Increment();
      if (m_) m_->Unlock();
    }
  }
  virtual void Increment() = 0;
  int &x_;
  int count_;
  gtags::Mutex *m_;
};

class IncrementThreadSlow : public IncrementThread {
 public:
  IncrementThreadSlow(int *x, int count, gtags::Mutex *m)
      : IncrementThread(x, count, m) {}
 protected:
  virtual void Increment() {
    const int kSlowFactor = 10000;
    int y = x_;
    y++;
    for (int i = 0; i < kSlowFactor; ++i) {
      EXPECT_GT(i * y, -1);  // prevent optimization of method to ++x_
    }
    x_ = y;
  }
};

class IncrementThreadFast : public IncrementThread {
 public:
  IncrementThreadFast(int *x, int count, gtags::Mutex *s)
      : IncrementThread(x, count, s) {}
 protected:
  virtual void Increment() {
    ++x_;
  }
};

TEST(MutexTest, ProtectionTest) {
  const int kInitialCount = 1;
  const int kCountIncrementFactor = 10;
  const int kMaximumCount = 1000;

  int x = 0;
  int count = kInitialCount;

  // Find the count for which we detect thread-unsafe behaviour due to
  // unprotected access (overlapping writes to x).
  for (; x == 2 * count && count <= kMaximumCount;
       count *= kCountIncrementFactor) {
    x = 0;
    IncrementThreadSlow its(&x, count, NULL);
    IncrementThreadFast itf(&x, count, NULL);
    its.Start();
    itf.Start();
    its.Join();
    itf.Join();
  }

  LOG(INFO) << "Finished unprotected with count=" << count;
  EXPECT_LT(x, 2 * count);

  // Repeat again with thread-safe behaviour.
  x = 0;
  gtags::Mutex m;
  IncrementThreadSlow its(&x, count, &m);
  IncrementThreadFast itf(&x, count, &m);
  its.Start();
  itf.Start();
  its.Join();
  itf.Join();

  EXPECT_EQ(x, 2 * count);
}

}  // namespace
