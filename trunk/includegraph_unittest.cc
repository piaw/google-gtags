// Copyright 2006 Google Inc. All Rights Reserved.
// Author: psung@google.com (Phil Sung)
//
// Note: This file is branched from unittest of the same name
// in google3/tools/tags. Unless a change is specific for the
// open source version of the unittest, please make changes to
// the file in google3/tools/tags and downintegrate.

#include "tools/tags/filename.h"
#include "tools/tags/includegraph.h"
#include "tools/tags/opensource/test_incl.h"

struct Fixture {
  Fixture() {
    g = new IncludeGraph();

    f1 = new Filename("f1");
    f2 = new Filename("f2");
    f3 = new Filename("f3");
    f4 = new Filename("f4");

    g->AddCall(f1, f2);
    g->AddCall(f1, f3);
    g->AddCall(f2, f4);
    g->AddCall(f3, f4);
  }

  ~Fixture() {
    delete g;

    delete f1;
    delete f2;
    delete f3;
    delete f4;
  }

  IncludeGraph* g;
  Filename* f1;
  Filename* f2;
  Filename* f3;
  Filename* f4;
};

BOOST_AUTO_UNIT_TEST(IncludeGraphTest_Clear) {
  Fixture f;
  f.g->Clear();
  IncludeGraph::const_iterator iter = f.g->GetCallees(f.f2);
  BOOST_CHECK(iter.IsDone());
}

BOOST_AUTO_UNIT_TEST(IncludeGraphTest_CalleesList) {
  Fixture f;
  IncludeGraph::const_iterator iter = f.g->GetCallees(f.f2);
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK_EQUAL(iter->second, f.f4);
  ++iter;
  BOOST_CHECK(iter.IsDone());
}

BOOST_AUTO_UNIT_TEST(IncludeGraphTest_CallersList) {
  Fixture f;
  IncludeGraph::const_iterator iter = f.g->GetCallers(f.f2);
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK_EQUAL(iter->second, f.f1);
  ++iter;
  BOOST_CHECK(iter.IsDone());
}
BOOST_AUTO_UNIT_TEST(IncludeGraphTest_RecursiveCalleesList) {
  Fixture f;
  IncludeGraph::const_iterator iter = f.g->GetRecursiveCallees(f.f2);
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 0);
  BOOST_CHECK_EQUAL(iter->second, f.f2);
  ++iter;
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK_EQUAL(iter->second, f.f4);
  ++iter;
  BOOST_CHECK(iter.IsDone());
}

BOOST_AUTO_UNIT_TEST(IncludeGraphTest_RecursiveCallersList) {
  Fixture f;
  IncludeGraph::const_iterator iter = f.g->GetRecursiveCallers(f.f2);
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 0);
  BOOST_CHECK_EQUAL(iter->second, f.f2);
  ++iter;
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK_EQUAL(iter->second, f.f1);
  ++iter;
  BOOST_CHECK(iter.IsDone());
}

BOOST_AUTO_UNIT_TEST(IncludeGraphTest_DontRepeatNodes) {
  Fixture f;
  IncludeGraph::const_iterator iter = f.g->GetRecursiveCallees(f.f1);
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 0);
  BOOST_CHECK_EQUAL(iter->second, f.f1);
  ++iter;
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK((iter->second == f.f2) || (iter->second == f.f3));
  ++iter;
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 1);
  BOOST_CHECK((iter->second == f.f2) || (iter->second == f.f3));
  ++iter;
  BOOST_CHECK_FALSE(iter.IsDone());
  BOOST_CHECK_EQUAL(iter->first, 2);
  BOOST_CHECK_EQUAL(iter->second, f.f4);
  ++iter;
  // Node f.f4 should not appear here again
  BOOST_CHECK(iter.IsDone());
}
