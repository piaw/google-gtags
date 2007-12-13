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

#include "gtagsunit.h"
#include "gtagsmixer.h"

#include "callback.h"

namespace {

GTAGS_FIXTURE(ResultMixerTest) {
 public:
  GTAGS_FIXTURE_SETUP(ResultMixerTest) {
    mixer = new ResultMixer(
	NUM_SOURCES_PER_REQUEST,
	gtags::CallbackFactory::Create(
	    this, &ResultMixerTest::Callback));
    calledback = false;
    result = "";
  }

  GTAGS_FIXTURE_TEARDOWN(ResultMixerTest) {}

  void Callback(const string& s) {
    result = s;
    calledback = true;
  }

 protected:
  ResultMixer* mixer;
  bool calledback;
  string result;
};

TEST_F(ResultMixerTest, set_result) {
  EXPECT_FALSE(calledback);
  mixer->set_result("((value (((tag tag1)))))", REMOTE);
  EXPECT_FALSE(calledback);
  mixer->set_result("((value (((tag tag3)))))", LOCAL);
  EXPECT_TRUE(calledback);
  EXPECT_EQ("((value (((tag tag3))((tag tag1)))))", result);
}

TEST_F(ResultMixerTest, partial_failure) {
  EXPECT_FALSE(calledback);
  mixer->set_failure("failed", REMOTE);
  EXPECT_FALSE(calledback);
  mixer->set_result("((value (((tag tag3)))))", LOCAL);
  EXPECT_TRUE(calledback);
  EXPECT_EQ("((value (((tag tag3)))))", result);
}

TEST_F(ResultMixerTest, no_tag_returned) {
  EXPECT_FALSE(calledback);
  mixer->set_result("((value t))", REMOTE);
  EXPECT_FALSE(calledback);
  mixer->set_result("((value t))", LOCAL);
  EXPECT_TRUE(calledback);
  EXPECT_EQ("((value t))", result);
}

TEST_F(ResultMixerTest, no_return_values) {
  EXPECT_FALSE(calledback);
  mixer->set_result("((balh t))", REMOTE);
  EXPECT_FALSE(calledback);
  mixer->set_failure("((hmm nil))", LOCAL);
  EXPECT_TRUE(calledback);
  EXPECT_EQ("((balh t))", result);
}

TEST_F(ResultMixerTest, all_failures) {
  EXPECT_FALSE(calledback);
  mixer->set_failure("remote", REMOTE);
  EXPECT_FALSE(calledback);
  mixer->set_failure("local", LOCAL);
  EXPECT_TRUE(calledback);
  EXPECT_EQ("((error ((message \"remote\"))))", result);
}

GTAGS_FIXTURE(ResultHolderTest) {
 public:
  GTAGS_FIXTURE_SETUP(ResultHolderTest) {
    mixer = new MockMixer();
    mixer->result = "";
    mixer->failure = "";
    holder = new ResultHolder(REMOTE, 3, mixer);
  }

  GTAGS_FIXTURE_TEARDOWN(ResultHolderTest) {
    delete mixer;
  }

  class MockMixer : public ResultMixer {
   public:
    MockMixer() : ResultMixer(0, NULL) {}
    virtual ~MockMixer() {}

    virtual void set_result(const string& result, SourceId id) {
      this->result = result;
      this->id = id;
    }
    virtual void set_failure(const string& failure, SourceId id) {
      this->failure = failure;
      this->id = id;
    }

    string result;
    string failure;
    SourceId id;
  };

 protected:
  MockMixer* mixer;
  ResultHolder* holder;
};

TEST_F(ResultHolderTest, set_result) {
  EXPECT_NE("result1", mixer->result);
  holder->set_result("result1");
  EXPECT_EQ("result1", mixer->result);
  EXPECT_EQ(REMOTE, mixer->id);

  holder->set_result("result2");
  EXPECT_EQ("result1", mixer->result);
  EXPECT_EQ(REMOTE, mixer->id);

  holder->set_result("result3");
  EXPECT_EQ("result1", mixer->result);
  EXPECT_EQ(REMOTE, mixer->id);
}

TEST_F(ResultHolderTest, set_failure) {
  holder->set_failure("failure1");
  EXPECT_EQ("", mixer->result);
  EXPECT_EQ("", mixer->failure);
  holder->set_failure("failure2");
  EXPECT_EQ("", mixer->result);
  EXPECT_EQ("", mixer->failure);
  holder->set_failure("failure3");
  EXPECT_EQ("", mixer->result);
  EXPECT_EQ("Failed to connect to remote services.", mixer->failure);
  EXPECT_EQ(REMOTE, mixer->id);
}

}  // namespace
