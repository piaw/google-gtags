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
#include "pcqueue.h"

#include "thread.h"

namespace {

const int DATA = 77;
const int DATA_A = 2;
const int DATA_B = 5;
const int DATA_C = 17;

TEST(PCQTest, PutTest) {
  int x = DATA;
  {
    gtags::ProducerConsumerQueue<int> pcq(1);
    pcq.Put(x);
    EXPECT_FALSE(pcq.TryPut(x));
  }
  {
    gtags::ProducerConsumerQueue<int> pcq(3);
    pcq.Put(x);
    pcq.Put(x);
    pcq.Put(x);
    EXPECT_FALSE(pcq.TryPut(x));
  }
}

TEST(PCQTest, GetTest) {
  int x = DATA;
  int *y = NULL;
  {
    gtags::ProducerConsumerQueue<int> pcq(1);
    pcq.Put(x);
    pcq.Get();
    EXPECT_FALSE(pcq.TryGet(y));
  }
  {
    gtags::ProducerConsumerQueue<int> pcq(3);
    pcq.Put(x);
    pcq.Put(x);
    pcq.Put(x);
    pcq.Get();
    pcq.Get();
    pcq.Get();
    EXPECT_FALSE(pcq.TryGet(y));
  }
}

TEST(PCQTest, ValueTest) {
  int x = DATA;
  const int *y;
  {
    y = NULL;
    gtags::ProducerConsumerQueue<const int*> pcq(1);
    pcq.Put(&x);
    y = pcq.Get();
    EXPECT_EQ(y, &x);
    EXPECT_EQ(*y, x);
  }
  {
    y = NULL;
    gtags::ProducerConsumerQueue<const int*> pcq(1);
    pcq.Put(&x);
    EXPECT_TRUE(pcq.TryGet(&y));
    EXPECT_EQ(y, &x);
    EXPECT_EQ(*y, x);
  }
  {
    y = NULL;
    gtags::ProducerConsumerQueue<const int*> pcq(1);
    EXPECT_TRUE(pcq.TryPut(&x));
    y = pcq.Get();
    EXPECT_EQ(y, &x);
    EXPECT_EQ(*y, x);
  }
  {
    y = NULL;
    gtags::ProducerConsumerQueue<const int*> pcq(1);
    EXPECT_TRUE(pcq.TryPut(&x));
    EXPECT_TRUE(pcq.TryGet(&y));
    EXPECT_EQ(y, &x);
    EXPECT_EQ(*y, x);
  }
}

TEST(PCQTest, TypeTest) {
  int x = DATA;
  {
    gtags::ProducerConsumerQueue<int> pcq(1);
    EXPECT_TRUE(pcq.TryPut(x));
    int y;
    EXPECT_TRUE(pcq.TryGet(&y));
  }
  {
    gtags::ProducerConsumerQueue<int*> pcq(1);
    EXPECT_TRUE(pcq.TryPut(&x));
    int *y;
    EXPECT_TRUE(pcq.TryGet(&y));
  }
  {
    gtags::ProducerConsumerQueue<const int*> pcq(1);
    EXPECT_TRUE(pcq.TryPut(&x));
    const int *y;
    EXPECT_TRUE(pcq.TryGet(&y));
  }
}

TEST(PCQTest, OrderTest) {
  int a = DATA_A;
  int b = DATA_B;
  int c = DATA_C;
  int y = 0;
  gtags::ProducerConsumerQueue<int> pcq(3);

  EXPECT_TRUE(pcq.TryPut(a));
  EXPECT_TRUE(pcq.TryPut(b));
  EXPECT_TRUE(pcq.TryPut(c));

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(y, a);

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(y, b);

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(y, c);
}

TEST(PCQTest, CountTest) {
  int a = DATA_A;
  int b = DATA_B;
  int c = DATA_C;
  int y = 0;
  gtags::ProducerConsumerQueue<int> pcq(3);
  EXPECT_EQ(pcq.count(), 0);

  EXPECT_TRUE(pcq.TryPut(a));
  EXPECT_EQ(pcq.count(), 1);

  EXPECT_TRUE(pcq.TryPut(b));
  EXPECT_EQ(pcq.count(), 2);

  EXPECT_TRUE(pcq.TryPut(c));
  EXPECT_EQ(pcq.count(), 3);

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(pcq.count(), 2);

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(pcq.count(), 1);

  EXPECT_TRUE(pcq.TryGet(&y));
  EXPECT_EQ(pcq.count(), 0);
}

}  // namespace
