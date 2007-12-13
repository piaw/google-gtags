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
#include "semaphore.h"

#include "thread.h"

namespace {

TEST(SemaphoreTest, TryLockTest) {
  {
    gtags::Semaphore s(1);
    EXPECT_TRUE(s.TryLock());
    s.Unlock();
  }
  {
    gtags::Semaphore s(1);
    s.Lock();
    EXPECT_FALSE(s.TryLock());
    s.Unlock();
  }
}

TEST(SemaphoreTest, LockTest) {
  {
    gtags::Semaphore s(1);
    s.Lock();
    EXPECT_FALSE(s.TryLock());
    s.Unlock();
  }
  {
    gtags::Semaphore s(1);
    s.Lock();
    s.Unlock();
    s.Lock();
    EXPECT_FALSE(s.TryLock());
    s.Unlock();
  }
  {
    gtags::Semaphore s(3);
    s.Lock();
    s.Lock();
    s.Lock();
    EXPECT_FALSE(s.TryLock());
    s.Unlock();
    s.Unlock();
    s.Unlock();
  }
}

TEST(SemaphoreTest, UnlockTest) {
  {
    gtags::Semaphore s(1);
    EXPECT_TRUE(s.TryLock());
    s.Unlock();
  }
  {
    gtags::Semaphore s(1);
    s.Lock();
    s.Unlock();
    EXPECT_TRUE(s.TryLock());
    s.Unlock();
  }
  {
    gtags::Semaphore s(1);
    s.Unlock();
    s.Unlock();
    s.Unlock();
    EXPECT_TRUE(s.TryLock());
    EXPECT_TRUE(s.TryLock());
    EXPECT_TRUE(s.TryLock());
    EXPECT_TRUE(s.TryLock());
    EXPECT_FALSE(s.TryLock());
  }
}

class IncrementThread : public gtags::Thread {
 public:
  IncrementThread(int *x, int count, gtags::Semaphore *s) :
      Thread(true), x_(*x), count_(count), s_(s) {}
 protected:
  virtual void Run() {
    for (int i = 0; i < count_; ++i) {
      if (s_) s_->Lock();
      Increment();
      if (s_) s_->Unlock();
    }
  }
  virtual void Increment() = 0;
  int &x_;
  int count_;
  gtags::Semaphore *s_;
};

class IncrementThreadSlow : public IncrementThread {
 public:
  IncrementThreadSlow(int *x, int count, gtags::Semaphore *s)
      : IncrementThread(x, count, s) {}
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
  IncrementThreadFast(int *x, int count, gtags::Semaphore *s)
      : IncrementThread(x, count, s) {}
 protected:
  virtual void Increment() {
    ++x_;
  }
};

TEST(SemaphoreTest, UnprotectedTest) {
  const int kInitialCount = 10;
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

  // Find the count for which we detect thread-unsafe behaviour due to
  // incorrectly protected access (overlapping writes to x).
  for (; x == 2 * count && count <= kMaximumCount;
       count *= kCountIncrementFactor) {
    x = 0;
    gtags::Semaphore s(2);
    IncrementThreadSlow its(&x, count, &s);
    IncrementThreadFast itf(&x, count, &s);
    its.Start();
    itf.Start();
    its.Join();
    itf.Join();
  }

  LOG(INFO) << "Finished incorrectly protected with count=" << count;
  EXPECT_LT(x, 2 * count);

  // Repeat again with thread-safe behaviour.
  x = 0;
  gtags::Semaphore s(1);
  IncrementThreadSlow its(&x, count, &s);
  IncrementThreadFast itf(&x, count, &s);
  its.Start();
  itf.Start();
  its.Join();
  itf.Join();

  EXPECT_EQ(x, 2 * count);
}

}  // namespace
