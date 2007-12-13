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

#include <vector>

#include "gtagsunit.h"
#include "stl_util.h"

namespace {

const int NUM_ITEMS = 3;
const int ARRAY_SIZES[NUM_ITEMS] = { 2, 5, 17 };

static void PushElements(vector<int*> *v) {
  for (int i = 0; i < NUM_ITEMS; ++i)
    v->push_back(new int());
}

static void PushArrays(vector<int*> *v) {
  for (int i = 0; i < NUM_ITEMS; ++i)
    v->push_back(new int[ARRAY_SIZES[i]]);
}

TEST(DeleteContainerElementsTest, EmptyTest) {
  vector<int*> v;
  STLDeleteContainerElements(v.begin(), v.end());
  EXPECT_EQ(v.size(), 0);
}

TEST(DeleteContainerElementsTest, NonEmptyTest) {
  vector<int*> v;
  PushElements(&v);
  STLDeleteContainerElements(v.begin(), v.end());
  EXPECT_EQ(v.size(), 3);
}

TEST(DeleteElementContainerTest, EmptyTest) {
  vector<int*> v;
  STLDeleteElementContainer(&v);
  EXPECT_EQ(v.size(), 0);
}

TEST(DeleteElementContainerTest, NonEmptyTest) {
  vector<int*> v;
  PushElements(&v);
  STLDeleteElementContainer(&v);
  EXPECT_EQ(v.size(), 0);
}

TEST(DeleteContainerArraysTest, EmptyTest) {
  vector<int*> v;
  STLDeleteContainerArrays(v.begin(), v.end());
  EXPECT_EQ(v.size(), 0);
}

TEST(DeleteContainerArraysTest, NonEmptyTest) {
  vector<int*> v;
  PushArrays(&v);
  STLDeleteContainerArrays(v.begin(), v.end());
  EXPECT_EQ(v.size(), 3);
}

TEST(DeleteArrayContainerTest, EmptyTest) {
  vector<int*> v;
  STLDeleteArrayContainer(&v);
  EXPECT_EQ(v.size(), 0);
}

TEST(DeleteArrayContainerTest, NonEmptyTest) {
  vector<int*> v;
  PushArrays(&v);
  STLDeleteArrayContainer(&v);
  EXPECT_EQ(v.size(), 0);
}

}
