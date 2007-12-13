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
#include "mixerrequesthandler.h"

#include "callback.h"
#include "datasource.h"
#include "gtagsmixer.h"
#include "sexpression.h"
#include "tagsoptionparser.h"

DECLARE_STRING(default_corpus);
DECLARE_STRING(default_language);
DECLARE_BOOL(default_callers);

const char* kCorpus = "corpus1";
const char* kMixerTestConfigFile = "/mixer_test_socket_config";

namespace {

// Delete all DataSource*'s in the given DataSourceMap.
void DeleteSources(const DataSourceMap& sources) {
  for (DataSourceMap::const_iterator it = sources.begin();
       it != sources.end(); ++it) {
    for (LanguageMap::const_iterator it2 = it->second.begin();
        it2 != it->second.end(); it2++) {
      if (it2->second.first)
        delete it2->second.first;
      if (it2->second.second)
        delete it2->second.second;
    }
  }
}

TEST(MixerRequestHandlerTest, CreateDataSourceRequest) {
  Settings::Load(TEST_DATA_DIR + kMixerTestConfigFile);
  MixerRequestHandler handler(NULL);
  // Normal cases
  string data = "(blah blah (corpus \"foo\") (language \"c++\") (callers 1))";
  SExpression* sexpr = SExpression::Parse(data);
  DataSourceRequest* request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("foo"), request->corpus());
  EXPECT_EQ(string("c++"), request->language());
  EXPECT_TRUE(request->callers());
  delete request;
  delete sexpr;

  data = "(blah blah (language \"c++\") (callers 1))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(GET_FLAG(default_corpus), request->corpus());
  EXPECT_EQ(string("c++"), request->language());
  EXPECT_TRUE(request->callers());
  delete request;
  delete sexpr;

  data = "(blah blah (corpus \"foo\") (language \"java\") (callers nil))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("foo"), request->corpus());
  EXPECT_EQ(string("java"), request->language());
  EXPECT_FALSE(request->callers());
  delete request;
  delete sexpr;

  // Missing corpus option.
  data = "((language \"c++\") (callers nil))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(GET_FLAG(default_corpus), request->corpus());
  EXPECT_EQ(string("c++"), request->language());
  EXPECT_FALSE(request->callers());
  delete request;
  delete sexpr;

  // Missing language option.
  data = "((corpus \"foo\") (callers nil))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("foo"), request->corpus());
  EXPECT_EQ(GET_FLAG(default_language), request->language());
  EXPECT_FALSE(request->callers());
  delete request;
  delete sexpr;

  // Missing callers option.
  data = "(blah blah (corpus \"foo\") (language \"c++\"))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("foo"), request->corpus());
  EXPECT_EQ(string("c++"), request->language());
  EXPECT_EQ(GET_FLAG(default_callers), request->callers());
  delete request;
  delete sexpr;

  // More than one language specified.
  data = "((language \"python\") (language \"c++\"))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("python"), request->language());
  delete request;
  delete sexpr;

  // Client does not modify current-file.  (It used to remove the path
  // prefix up to kCorpus.)
  data = "((language \"c++\")(current-file \"/home/user/google3/gtags.cc\"))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string("/home/user/google3/gtags.cc"), request->client_path());
  delete request;
  delete sexpr;

  // No current file
  data = "((language \"c++\")(file \"/home/user/google3/gtags.cc\"))";
  sexpr = SExpression::Parse(data);
  request = handler.CreateDataSourceRequest(sexpr);
  EXPECT_EQ(sexpr->Repr(), request->request());
  EXPECT_EQ(string(""), request->client_path());
  delete request;
  delete sexpr;

  DeleteSources(Settings::instance()->sources());
  Settings::Free();
}

// A DataSource that always returns a given result string.
class DataSourceStub : public DataSource {
 public:
  explicit DataSourceStub(string result) : result_(result) {}
  virtual ~DataSourceStub() {}
  virtual void GetTags(const DataSourceRequest& request, ResultHolder* holder) {
    holder->set_result(result_);
  }
  virtual int size() const {
    return 1;
  }

 private:
  string result_;
  DISALLOW_EVIL_CONSTRUCTORS(DataSourceStub);
};

// A ResponseCallback that stores the response in a target string.
void StoreResponse(string *target, const string &response) {
  *target = response;
}

TEST(MixerRequestHandlerTest, Ping) {
  DataSourceMap sources;
  MixerRequestHandler handler(&sources);
  string response;
  handler.Execute("(ping (language \"c++\"))",
                  gtags::CallbackFactory::Create(&StoreResponse, &response));
  EXPECT_EQ(response, string("((value t))"));
}

TEST(MixerRequestHandlerTest, Execute) {
  Settings::Load(TEST_DATA_DIR + kMixerTestConfigFile);
  DataSourceMap sources;
  LanguageMap& lang_map = sources[kCorpus];
  lang_map["c++"] = make_pair(new DataSourceStub("((value (((tag cpp)))))"),
                              new DataSourceStub(
                                  "((value (((tag cpp_call)))))"));
  MixerRequestHandler handler(&sources);
  string response;
  handler.Execute("((language \"c++\"))",
                   gtags::CallbackFactory::Create(&StoreResponse, &response));
  EXPECT_EQ(response,
            string("((value (((tag cpp)))))"));

  handler.Execute("((language \"c++\") (callers 1))",
                   gtags::CallbackFactory::Create(&StoreResponse, &response));
  EXPECT_EQ(response, string(
                "((value (((tag cpp_call)))))"));
  DeleteSources(sources);
  DeleteSources(Settings::instance()->sources());
  Settings::Free();
}

TEST(MixerRequestHandlerTest, ExecuteWithLocal) {
  Settings::Load(TEST_DATA_DIR + kMixerTestConfigFile);
  DataSourceMap sources;
  LanguageMap& lang_map = sources[kCorpus];
  lang_map["c++"] = make_pair(new DataSourceStub("((value (((tag cpp)))))"),
                              new DataSourceStub(
                                  "((value (((tag cpp_call)))))"));
  lang_map["local"] = make_pair(new DataSourceStub("((value (((tag local)))))"),
                                new DataSourceStub(
                                    "((value (((tag local_call)))))"));
  MixerRequestHandler handler(&sources);
  string response;
  handler.Execute("((language \"c++\"))",
                   gtags::CallbackFactory::Create(&StoreResponse, &response));
  EXPECT_EQ(response,
            string("((value (((tag local))((tag cpp)))))"));

  handler.Execute("((language \"c++\") (callers 1))",
                   gtags::CallbackFactory::Create(&StoreResponse, &response));
  EXPECT_EQ(response, string(
                "((value (((tag local_call))((tag cpp_call)))))"));
  DeleteSources(sources);
  DeleteSources(Settings::instance()->sources());
  Settings::Free();
}

}  // namespace
