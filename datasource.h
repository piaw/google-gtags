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
// Each DataSource represents a collection of equivalent GTags servers. For
// example, all the GTags servers for C++ definition are considered to be one
// DataSource. Each DataSource is responsible for dispatching tag requests to
// all its servers via Stubby RPC. The RPC calls are asynchronous with a
// callback to be invoked when they complete. This callback is invoked by stubby
// in separate threads.

#ifndef TOOLS_TAGS_DATASOURCE_H__
#define TOOLS_TAGS_DATASOURCE_H__

#include <list>

#include "tagsutil.h"
#include "strutil.h"

class DataSourceRequest {
 public:
  DataSourceRequest() {}
  virtual ~DataSourceRequest() {}

  virtual void CopyFrom(const DataSourceRequest& other) {
    request_ = other.request_;
    language_ = other.language_;
    callers_ = other.callers_;
    client_path_ = other.client_path_;
    corpus_ = other.corpus_;
  }

  virtual const string& request() const { return request_; }
  virtual const string& language() const { return language_; }
  virtual bool callers() const { return callers_; }
  virtual const string& client_path() const { return client_path_; }
  virtual const string& corpus() const { return corpus_; }

  virtual void set_request(const string &request) { request_ = request; }
  virtual void set_language(const string &lang) { language_ = lang; }
  virtual void set_callers(bool callers) { callers_ = callers; }
  virtual void set_client_path(const string &path) { client_path_ = path; }
  virtual void set_corpus(const string &corpus) { corpus_ = corpus; }

 protected:
  string request_;
  string language_;
  bool callers_;
  string client_path_;
  string corpus_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DataSourceRequest);
};

// Forward declarations.
class ResultHolder;
namespace gtags {
class TagsServiceUser;
}

// DataSource represent several potential TagsService provider for a particular
// corpus. For example, all remote gtags servers for C++ definition would be one
// data source. All remote gtags server for java callgraph would be another.
// DataSource basically dispatches DataSourceRequest to service providers within
// a corpus and does all the related housekeeping cleanups.
class DataSource {
 public:
  virtual ~DataSource() {}

  virtual void GetTags(
      const DataSourceRequest& request, ResultHolder* holder) = 0;

  virtual int size() const = 0;

 protected:
  DataSource() {}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DataSource);
};

// DataSource for abstracting communication with remote GTags servers.
class RemoteDataSource : public DataSource {
 public:
  RemoteDataSource() {}
  virtual ~RemoteDataSource();

  void AddSource(gtags::TagsServiceUser *service);
  virtual void GetTags(const DataSourceRequest& request, ResultHolder* holder);

  virtual int size() const {
    return services_.size();
  }

 private:
  list<gtags::TagsServiceUser*> services_;

  DISALLOW_EVIL_CONSTRUCTORS(RemoteDataSource);
};

// Forward declaration for LocalDataSource.
class LocalTagsRequestHandler;

// DataSource for abstracting communication with local GTags servers.
class LocalDataSource : public DataSource {
 public:
  explicit LocalDataSource(LocalTagsRequestHandler* handler)
      : handler_(handler) {}
  virtual ~LocalDataSource() {}

  virtual void GetTags(const DataSourceRequest& request, ResultHolder* holder);

  // Only one source for local server.
  virtual int size() const { return 1; }

 private:
  LocalTagsRequestHandler* handler_;

  DISALLOW_EVIL_CONSTRUCTORS(LocalDataSource);
};

#endif  // TOOLS_TAGS_DATASOURCE_H__
