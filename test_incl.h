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
// Author: stephenchen@google.com (Stephen Chen)
//
// Header file that does some common set up / boilerplate
// stuff to all unit tests.

#ifndef TOOLS_TAGS_OPENSOURCE_TEST_INCL_H__
#define TOOLS_TAGS_OPENSOURCE_TEST_INCL_H__

// Set up boost test auto registration
#define BOOST_AUTO_TEST_MAIN
#include <boost/test/auto_unit_test.hpp>

// Set up logger for unit test to link against
#include "stderr_logger.h"
namespace {
StdErrLogger logger_instance;
};
GtagsLogger * logger = &logger_instance;

// Testing environment
#include "tagsoptionparser.h"
DEFINE_STRING(test_srcdir, TEST_SRC_DIR,
              "This should be the directory that contains testdata/")
DEFINE_STRING(test_tmpdir, "/tmp", "Directory that can store temporary data")


// Helper boost checks
#include <cstring>
using namespace std;

#define BOOST_CHECK_STREQUAL(str1, str2) BOOST_CHECK(strcmp(str1, str2) == 0)
#define BOOST_CHECK_FALSE(expression) BOOST_CHECK(!(expression))
#define BOOST_CHECK_NOT_EQUAL(a, b) BOOST_CHECK(!((a) == (b)))
#define BOOST_CHECK_GREATER_THAN(a, b) BOOST_CHECK((a) > (b))

#endif  // TOOLS_TAGS_OPENSOURCE_TEST_INCL_H__
