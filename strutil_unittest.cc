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
// Note: This file is branched from unittest of the same name
// in google3/tools/tags. Unless a change is specific for the
// open source version of the unittest, please make changes to
// the file in google3/tools/tags and downintegrate.


#include <string>
#include "strutil.h"
#include "test_incl.h"

BOOST_AUTO_UNIT_TEST(StrUtilTest_CEscape) {
  string buffer = "\n125\r\t\t\"\'\\abc";
  string result = CEscape(buffer);
  string expected = "\\n125\\r\\t\\t\\\"\\\'\\\\abc";
  BOOST_CHECK_EQUAL(result.size(), expected.size());
  BOOST_CHECK_EQUAL(result, expected);

  string emptyline = "";
  result = CEscape(emptyline);
  BOOST_CHECK_EQUAL(result, "");
}

BOOST_AUTO_UNIT_TEST(StrUtilTest_FastIntoa) {
  int largest_negative = -2147483647;
  int tendigitpositive = 1111111111;
  int zero = 0;
  int normal_negative = -547;
  int normal_positive = 1000;
  BOOST_CHECK_EQUAL("-2147483647", FastItoa(largest_negative));
  BOOST_CHECK_EQUAL("1111111111", FastItoa(tendigitpositive));
  BOOST_CHECK_EQUAL("0", FastItoa(zero));
  BOOST_CHECK_EQUAL("-547", FastItoa(normal_negative));
  BOOST_CHECK_EQUAL("1000", FastItoa(normal_positive));
}

BOOST_AUTO_UNIT_TEST(StrUtilTest_IsIntToken) {
  const char * too_large = "111111111111";
  const char * zero = "0";
  const char * negative = "-2831";
  const char * leading_zero = "00342";
  const char * leading_zero_negative = "-00564";
  const char * decimal = "2.45";
  const char * letter = "ab35";
  const char * letter_end = "43ab";
  const char *  empty_string = "";
  BOOST_CHECK(!IsIntToken(too_large));
  BOOST_CHECK(IsIntToken(zero));
  BOOST_CHECK(IsIntToken(negative));
  BOOST_CHECK(IsIntToken(leading_zero));
  BOOST_CHECK(IsIntToken(leading_zero_negative));
  BOOST_CHECK(!IsIntToken(decimal));
  BOOST_CHECK(!IsIntToken(letter));
  BOOST_CHECK(!IsIntToken(letter_end));
  BOOST_CHECK(!IsIntToken(empty_string));
}
