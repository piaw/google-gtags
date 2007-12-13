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
// Due to the repetitive nature of the testing, we take advantage of test
// fixtures.  We define a CallbackTest to contain most of the test code.  We
// then define a MemberCallbackTest and a StaticCallback#Test, each of which
// know how to Reset and CheckRunResults.  Lastly, we define a
// MemberCallback#Test and a StaticCallback#Test for each Callback# supported
// (where # is the number of arguments allowed).  These classes override the
// RunCallback and RunVoidCallback methods to perform the actual invoking of the
// Callback#.
// This test fixture structure will come in handy as we start extending the
// capabilities of Callbacks.

#include "gtagsunit.h"
#include "callback.h"

using gtags::CallbackFactory;

namespace {

class MockMemberCallee {
 public:
  MockMemberCallee() {
    Reset();
  }
  virtual ~MockMemberCallee() {}

  virtual void Reset() {
    invoked_ = false;
    result_ = 0;
  }

  virtual bool invoked() const { return invoked_; }
  virtual int result() const { return result_; }

  virtual void VoidInc() { Inc(); }
  virtual int Inc() { return Add(result_, 1); }

  virtual void VoidDouble(int a) { Double(a); }
  virtual int Double(int a) { return Add(a, a); }

  virtual void VoidAdd(int a, int b) { Add(a, b); }
  virtual int Add(int a, int b) { return Sum3(a, b, 0); }

  virtual void VoidSum3(int a, int b, int c) { Sum3(a, b, c); }
  virtual int Sum3(int a, int b, int c) { return Sum4(a, b, c, 0); }

  virtual void VoidSum4(int a, int b, int c, int d) { Sum4(a, b, c, d); }
  virtual int Sum4(int a, int b, int c, int d) { return Sum5(a, b, c, d, 0); }

  virtual void VoidSum5(int a, int b, int c, int d, int e) {
    Sum5(a, b, c, d, e);
  }
  virtual int Sum5(int a, int b, int c, int d, int e) {
    invoked_ = true;
    result_ = a + b + c + d + e;
    return result_;
  }

 private:
  bool invoked_;
  int result_;
};

class MockStaticCallee {
 public:
  virtual ~MockStaticCallee();

  static void Reset() {
    invoked_ = false;
    result_ = 0;
  }

  static bool invoked() { return invoked_; }
  static int result() { return result_; }

  static void VoidInc() { Inc(); }
  static int Inc() { return Add(result_, 1); }

  static void VoidDouble(int a) { Double(a); }
  static int Double(int a) { return Add(a, a); }

  static void VoidAdd(int a, int b) { Add(a, b); }
  static int Add(int a, int b) { return Sum3(a, b, 0); }

  static void VoidSum3(int a, int b, int c) { Sum3(a, b, c); }
  static int Sum3(int a, int b, int c) { return Sum4(a, b, c, 0); }

  static void VoidSum4(int a, int b, int c, int d) { Sum4(a, b, c, d); }
  static int Sum4(int a, int b, int c, int d) { return Sum5(a, b, c, d, 0); }

  static void VoidSum5(int a, int b, int c, int d, int e) {
    Sum5(a, b, c, d, e);
  }
  static int Sum5(int a, int b, int c, int d, int e) {
    invoked_ = true;
    result_ = a + b + c + d + e;
    return result_;
  }

 protected:
  static bool invoked_;
  static int result_;

 private:
  MockStaticCallee();
};

// We use the default g++ compiler for opensource building, which is different
// to our internal build tools.  g++ requires that static members are declared
// outside of the class definition IN ADDITION TO being declared inside the
// class.  The class definition is compiled into references, so these are the
// actual variables.
bool MockStaticCallee::invoked_;
int MockStaticCallee::result_;

GTAGS_FIXTURE(CallbackTest) {
 protected:
  void RunNonPermanentTests() {
    Reset();
    RunCallback();
    CheckRunResults();

    Reset();
    RunVoidCallback();
    CheckRunResults();
  }

  void RunPermanentTests() {
    RunNonPermanentTests();
    RunNonPermanentTests();
  }

  virtual void Reset() = 0;
  virtual void CheckRunResults() = 0;

  virtual void RunCallback() = 0;
  virtual void RunVoidCallback() = 0;

  int result_;
  int expected_;
};

class MemberCallbackTest : public CallbackTest {
 protected:
  virtual void Reset() {
    callee_.Reset();
    EXPECT_FALSE(callee_.invoked());
    EXPECT_EQ(callee_.result(), 0);
  }

  virtual void CheckRunResults() {
    EXPECT_TRUE(callee_.invoked());
    EXPECT_EQ(result_, expected_);
  }

  MockMemberCallee callee_;
};

class StaticCallbackTest : public CallbackTest {
 protected:
  virtual void Reset() {
    MockStaticCallee::Reset();
    EXPECT_FALSE(MockStaticCallee::invoked());
    EXPECT_EQ(MockStaticCallee::result(), 0);
  }

  virtual void CheckRunResults() {
    EXPECT_TRUE(MockStaticCallee::invoked());
    EXPECT_EQ(result_, expected_);
  }
};

const int DATA_A = 4;
const int DATA_B = 9;
const int DATA_C = 23;
const int DATA_D = 31;
const int DATA_E = 42;
const int RESULT_INC = 1;
const int RESULT_DOUBLE = 8;
const int RESULT_ADD = 13;
const int RESULT_SUM3 = 36;
const int RESULT_SUM4 = 67;
const int RESULT_SUM5 = 109;

// *** Base 0-Argument Callback Tests *** //

class MemberCallback0Test : public MemberCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run();
  }
  virtual void RunVoidCallback() {
    voidcb_->Run();
    result_ = callee_.result();
  }

  void RunClosureTest() {
    Reset();
    closure_->Run();
    result_ = callee_.result();
    CheckRunResults();
  }

  gtags::Callback0<int> *cb_;
  gtags::Callback0<void> *voidcb_;
  gtags::Closure *closure_;
};

class StaticCallback0Test : public StaticCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run();
  }
  virtual void RunVoidCallback() {
    voidcb_->Run();
    result_ = MockStaticCallee::result();
  }

  void RunClosureTest() {
    Reset();
    closure_->Run();
    result_ = MockStaticCallee::result();
    CheckRunResults();
  }

  gtags::Callback0<int> *cb_;
  gtags::Callback0<void> *voidcb_;
  gtags::Closure *closure_;
};

// *** 0-Prebound 1-Argument Callback Tests *** //

TEST_F(MemberCallback0Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Inc);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidInc);
  expected_ = RESULT_INC;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidInc);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(MemberCallback0Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Inc);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidInc);
  expected_ = RESULT_INC;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidInc);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback0Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(&MockStaticCallee::Inc);
  voidcb_ = CallbackFactory::Create(&MockStaticCallee::VoidInc);
  expected_ = RESULT_INC;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(&MockStaticCallee::VoidInc);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(StaticCallback0Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::Inc);
  voidcb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidInc);
  expected_ = RESULT_INC;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidInc);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}

// *** 1-Prebound 0-Argument Callback Tests *** //

TEST_F(MemberCallback0Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Double, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidDouble, DATA_A);
  expected_ = RESULT_DOUBLE;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidDouble, DATA_A);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(MemberCallback0Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Double, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidDouble, DATA_A);
  expected_ = RESULT_DOUBLE;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidDouble, DATA_A);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback0Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Double, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidDouble, DATA_A);
  expected_ = RESULT_DOUBLE;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(&MockStaticCallee::VoidDouble, DATA_A);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(StaticCallback0Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Double, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidDouble , DATA_A);
  expected_ = RESULT_DOUBLE;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidDouble, DATA_A);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}

// *** 2-Prebound 0-Argument Callback Tests *** //

TEST_F(MemberCallback0Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Add, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A, DATA_B);
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A, DATA_B);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(MemberCallback0Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Add, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A, DATA_B);
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A, DATA_B);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback0Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Add, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidAdd, DATA_A, DATA_B);
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(&MockStaticCallee::VoidAdd, DATA_A, DATA_B);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(StaticCallback0Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Add, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidAdd, DATA_A, DATA_B);
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidAdd, DATA_A, DATA_B);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}

// *** 3-Prebound 0-Argument Callback Tests *** //

TEST_F(MemberCallback0Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum3, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B, DATA_C);
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B, DATA_C);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(MemberCallback0Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum3, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B, DATA_C);
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B, DATA_C);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback0Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum3, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B, DATA_C);
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();

  closure_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B, DATA_C);

  EXPECT_FALSE(closure_->IsRepeatable());

  RunClosureTest();
}
TEST_F(StaticCallback0Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum3, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B, DATA_C);
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  closure_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B, DATA_C);

  EXPECT_TRUE(closure_->IsRepeatable());

  RunClosureTest();
  RunClosureTest();

  EXPECT_TRUE(closure_->IsRepeatable());

  delete closure_;
  delete voidcb_;
  delete cb_;
}


// *** Base 1-Argument Callback Test Classes *** //

class MemberCallback1Test : public MemberCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run(arg1_);
  }
  virtual void RunVoidCallback() {
    voidcb_->Run(arg1_);
    result_ = callee_.result();
  }
  gtags::Callback1<int, int> *cb_;
  gtags::Callback1<void, int> *voidcb_;
  int arg1_;
};

class StaticCallback1Test : public StaticCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run(arg1_);
  }
  virtual void RunVoidCallback() {
    voidcb_->Run(arg1_);
    result_ = MockStaticCallee::result();
  }
  gtags::Callback1<int, int> *cb_;
  gtags::Callback1<void, int> *voidcb_;
  int arg1_;
};

// *** 0-Prebound 1-Argument Callback Tests *** //

TEST_F(MemberCallback1Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Double);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidDouble);
  arg1_ = DATA_A;
  expected_ = RESULT_DOUBLE;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback1Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Double);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidDouble);
  arg1_ = DATA_A;
  expected_ = RESULT_DOUBLE;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback1Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(&MockStaticCallee::Double);
  voidcb_ = CallbackFactory::Create(&MockStaticCallee::VoidDouble);
  arg1_ = DATA_A;
  expected_ = RESULT_DOUBLE;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback1Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::Double);
  voidcb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidDouble);
  arg1_ = DATA_A;
  expected_ = RESULT_DOUBLE;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 1-Prebound 1-Argument Callback Tests *** //

TEST_F(MemberCallback1Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Add, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A);
  arg1_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback1Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Add, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidAdd, DATA_A);
  arg1_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback1Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Add, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidAdd, DATA_A);
  arg1_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback1Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Add, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidAdd , DATA_A);
  arg1_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 2-Prebound 1-Argument Callback Tests *** //

TEST_F(MemberCallback1Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum3, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B);
  arg1_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback1Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum3, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A, DATA_B);
  arg1_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback1Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum3, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B);
  arg1_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback1Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum3, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum3, DATA_A, DATA_B);
  arg1_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 3-Prebound 1-Argument Callback Tests *** //

TEST_F(MemberCallback1Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum4, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum4, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback1Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum4, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum4, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback1Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum4, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum4, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback1Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum4, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum4, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}


// *** Base 2-Argument Callback Test Classes *** //

class MemberCallback2Test : public MemberCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run(arg1_, arg2_);
  }
  virtual void RunVoidCallback() {
    voidcb_->Run(arg1_, arg2_);
    result_ = callee_.result();
  }
  gtags::Callback2<int, int, int> *cb_;
  gtags::Callback2<void, int, int> *voidcb_;
  int arg1_, arg2_;
};

class StaticCallback2Test : public StaticCallbackTest {
 protected:
  virtual void RunCallback() {
    result_ = cb_->Run(arg1_, arg2_);
  }
  virtual void RunVoidCallback() {
    voidcb_->Run(arg1_, arg2_);
    result_ = MockStaticCallee::result();
  }
  gtags::Callback2<int, int, int> *cb_;
  gtags::Callback2<void, int, int> *voidcb_;
  int arg1_, arg2_;
};

// *** 0-Prebound 2-Argument Callback Tests *** //

TEST_F(MemberCallback2Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Add);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidAdd);
  arg1_ = DATA_A;
  arg2_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback2Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Add);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidAdd);
  arg1_ = DATA_A;
  arg2_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback2Test, P0_NonPermanent) {
  cb_ = CallbackFactory::Create(&MockStaticCallee::Add);
  voidcb_ = CallbackFactory::Create(&MockStaticCallee::VoidAdd);
  arg1_ = DATA_A;
  arg2_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback2Test, P0_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::Add);
  voidcb_ = CallbackFactory::CreatePermanent(&MockStaticCallee::VoidAdd);
  arg1_ = DATA_A;
  arg2_ = DATA_B;
  expected_ = RESULT_ADD;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 1-Prebound 2-Argument Callback Tests *** //

TEST_F(MemberCallback2Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum3, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A);
  arg1_ = DATA_B;
  arg2_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback2Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum3, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum3, DATA_A);
  arg1_ = DATA_B;
  arg2_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

TEST_F(StaticCallback2Test, P1_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum3, DATA_A);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum3, DATA_A);
  arg1_ = DATA_B;
  arg2_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback2Test, P1_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum3, DATA_A);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum3, DATA_A);
  arg1_ = DATA_B;
  arg2_ = DATA_C;
  expected_ = RESULT_SUM3;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 2-Prebound 2-Argument Callback Tests *** //

TEST_F(MemberCallback2Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum4, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum4, DATA_A, DATA_B);
  arg1_ = DATA_C;
  arg2_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback2Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum4, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum4, DATA_A, DATA_B);
  arg1_ = DATA_C;
  arg2_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback2Test, P2_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum4, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum4, DATA_A, DATA_B);
  arg1_ = DATA_C;
  arg2_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback2Test, P2_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum4, DATA_A, DATA_B);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum4, DATA_A, DATA_B);
  arg1_ = DATA_C;
  arg2_ = DATA_D;
  expected_ = RESULT_SUM4;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

// *** 3-Prebound 2-Argument Callback Tests *** //

TEST_F(MemberCallback2Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::Sum5, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &callee_, &MockMemberCallee::VoidSum5, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  arg2_ = DATA_E;
  expected_ = RESULT_SUM5;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(MemberCallback2Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::Sum5, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &callee_, &MockMemberCallee::VoidSum5, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  arg2_ = DATA_E;
  expected_ = RESULT_SUM5;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}
TEST_F(StaticCallback2Test, P3_NonPermanent) {
  cb_ = CallbackFactory::Create(
      &MockStaticCallee::Sum5, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::Create(
      &MockStaticCallee::VoidSum5, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  arg2_ = DATA_E;
  expected_ = RESULT_SUM5;

  EXPECT_FALSE(cb_->IsRepeatable());
  EXPECT_FALSE(voidcb_->IsRepeatable());

  RunNonPermanentTests();
}
TEST_F(StaticCallback2Test, P3_Permanent) {
  cb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::Sum5, DATA_A, DATA_B, DATA_C);
  voidcb_ = CallbackFactory::CreatePermanent(
      &MockStaticCallee::VoidSum5, DATA_A, DATA_B, DATA_C);
  arg1_ = DATA_D;
  arg2_ = DATA_E;
  expected_ = RESULT_SUM5;

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  RunPermanentTests();

  EXPECT_TRUE(cb_->IsRepeatable());
  EXPECT_TRUE(voidcb_->IsRepeatable());

  delete voidcb_;
  delete cb_;
}

}  // namespace
