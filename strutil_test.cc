// Copyright 2007 Google Inc. All Rights Reserved.
// Author: stephenchen@google.com (Stephen Chen)

#include "gtagsunit.h"
#include "strutil.h"

TEST(StrUtilTest, EscapeTest) {
  string buffer = "\n125\r\t\t\"\'\\abc";
  string result = CEscape(buffer);
  string expected = "\\n125\\r\\t\\t\\\"\\\'\\\\abc";
  EXPECT_EQ(result.size(), expected.size());
  EXPECT_EQ(result, expected);

  string emptyline = "";
  result = CEscape(emptyline);
  EXPECT_EQ(result, "");
}

TEST(StrUtilTest, FastItoaTest) {
  int largest_negative = -2147483647;
  int tendigitpositive = 1111111111;
  int zero = 0;
  int normal_negative = -547;
  int normal_positive = 1000;
  EXPECT_EQ("-2147483647", FastItoa(largest_negative));
  EXPECT_EQ("1111111111", FastItoa(tendigitpositive));
  EXPECT_EQ("0", FastItoa(zero));
  EXPECT_EQ("-547", FastItoa(normal_negative));
  EXPECT_EQ("1000", FastItoa(normal_positive));
}

TEST(StrUtilTest, IsIntToken) {
  const char * too_large = "111111111111";
  const char * zero = "0";
  const char * negative = "-2831";
  const char * leading_zero = "00342";
  const char * leading_zero_negative = "-00564";
  const char * decimal = "2.45";
  const char * letter = "ab35";
  const char * letter_end = "43ab";
  const char *  empty_string = "";
  EXPECT_FALSE(IsIntToken(too_large));
  EXPECT_TRUE(IsIntToken(zero));
  EXPECT_TRUE(IsIntToken(negative));
  EXPECT_TRUE(IsIntToken(leading_zero));
  EXPECT_TRUE(IsIntToken(leading_zero_negative));
  EXPECT_FALSE(IsIntToken(decimal));
  EXPECT_FALSE(IsIntToken(letter));
  EXPECT_FALSE(IsIntToken(letter_end));
  EXPECT_FALSE(IsIntToken(empty_string));
}
