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
//
// This header file is used as an indirection for all unit tests that need to be
// opensourced.  The internal version leaves almost everything to gUnit.
//
// The TEST_DATA_DIR should be used to point to the 'testdata' directory.
// Fixtures should be defined as follows:
//   GTAGS_FIXTURE(FixtureClassName) {
//    public:
//     GTAGS_FIXTURE_SETUP(FixtureClassName) {
//       ...
//     }
//     GTAGS_FIXTURE_TEARDOWN(FixtureClassName) {
//       ...
//     }
//   }

#ifndef TOOLS_TAGS_GTAGSUNIT_H__
#define TOOLS_TAGS_GTAGSUNIT_H__

// Set up boost test auto registration
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

#include <cstring>
using std::string;

// Testing environment
#include "tagsoptionparser.h"
DEFINE_STRING(test_srcdir, TEST_SRC_DIR,
              "This should be the directory that contains testdata/")
DEFINE_STRING(test_tmpdir, TEST_TMP_DIR,
              "Directory that can store temporary data")

#define TEST_DATA_DIR GET_FLAG(test_srcdir) + "/testdata"

// Define test case & fixture macros
#define TEST(test_case, test) BOOST_AUTO_TEST_CASE(test_case##_##test)
#define TEST_F(fixture, test) BOOST_FIXTURE_TEST_CASE(fixture##_##test, fixture)

#define GTAGS_FIXTURE(class_name) class class_name
#define GTAGS_FIXTURE_SETUP(class_name) class_name()
#define GTAGS_FIXTURE_TEARDOWN(class_name) virtual ~class_name()


// Define EXPECT_* macros
#define EXPECT_TRUE(expression) BOOST_CHECK(expression)
#define EXPECT_FALSE(expression) EXPECT_TRUE(!(expression))

#define EXPECT_EQ(a, b) BOOST_CHECK_EQUAL(a, b)
#define EXPECT_NE(a, b) EXPECT_TRUE((a) != (b))

#define EXPECT_LE(a, b) EXPECT_TRUE((a) <= (b))
#define EXPECT_GE(a, b) EXPECT_TRUE((a) >= (b))
#define EXPECT_LT(a, b) EXPECT_TRUE((a) < (b))
#define EXPECT_GT(a, b) EXPECT_TRUE((a) > (b))

#define EXPECT_STREQ(str1, str2) EXPECT_TRUE(strcmp(str1, str2) == 0)
#define EXPECT_STRNE(str1, str2) EXPECT_TRUE(strcmp(str1, str2) != 0)


// Define ASSERT_* macros
#define ASSERT_TRUE(expression) BOOST_REQUIRE(expression)
#define ASSERT_FALSE(expression) ASSERT_TRUE(!(expression))

#define ASSERT_EQ(a, b) BOOST_REQUIRE_EQUAL(a, b)
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))

#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))

#define ASSERT_STREQ(str1, str2) ASSERT_TRUE(strcmp(str1, str2) == 0)
#define ASSERT_STRNE(str1, str2) ASSERT_TRUE(strcmp(str1, str2) != 0)

#endif  // TOOLS_TAGS_GTAGSUNIT_H__
