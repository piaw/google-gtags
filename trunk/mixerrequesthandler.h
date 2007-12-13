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

#ifndef TOOLS_TAGS_MIXERREQUESTHANDLER_H__
#define TOOLS_TAGS_MIXERREQUESTHANDLER_H__

#include <list>
#include <ext/hash_map>

#include "callback.h"
#include "settings.h"

// Forward declarations.
class SExpression;
class DataSourceRequest;

// Handler for TagsMixerConnection. TagsMixerConnection invokes the Execute
// function on user input which maps the request to a subset of data_sources_.
// Invoke Done function when we have results to send back the client.
class MixerRequestHandler {
 protected:
  typedef gtags::Callback1<void, const string&> ResponseCallback;

 public:
  MixerRequestHandler(const DataSourceMap* sources)
      : data_sources_(sources) {}
  virtual ~MixerRequestHandler() {}

  // Invoked by TagsMixerConnection. Basically forward command to the
  // appropriate DataSources for asynchronous RPC calls.
  virtual void Execute(const char* command,
                       ResponseCallback* response_callback) const;

  // Construct an instance of DataSourceRequest from command string. Language
  // and caller settings are extracted from command string and cached in
  // DataSourceRequest.
  // Caller is responsible for deleting the object.
  DataSourceRequest* CreateDataSourceRequest(const SExpression* sexpr) const;

 protected:
  // Send response over conn back the client and clean up.
  static void Done(ResponseCallback* response_callback,
                   DataSourceRequest* request,
                   const string& response);

 private:
  bool IsCommandPing(const SExpression* sexpr) const;
  const DataSourceMap* data_sources_;

  DISALLOW_EVIL_CONSTRUCTORS(MixerRequestHandler);
};

#endif  // TOOLS_TAGS_MIXERREQUESTHANDLER_H__
