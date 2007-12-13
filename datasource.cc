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

#include "datasource.h"

#include "gtagsmixer.h"
#include "stl_util.h"
#include "tags_service.h"
#include "tagsrequesthandler.h"

RemoteDataSource::~RemoteDataSource() {
  STLDeleteElementContainer(&services_);
}

void RemoteDataSource::AddSource(gtags::TagsServiceUser *service) {
  services_.push_back(service);
}

void RemoteDataSource::GetTags(const DataSourceRequest& request,
                               ResultHolder* holder) {
  // Make a copy of the request before we make any calls.
  // We need to do this because any RPC call we make later in this function may
  // cause request to be deallocated.
  DataSourceRequest request_copy;
  request_copy.CopyFrom(request);

  // Make non-blocking RPC
  for (list<gtags::TagsServiceUser*>::iterator iter = services_.begin();
       iter != services_.end(); ++iter) {
    (*iter)->GetTags(request_copy.request(), holder);
  }
}

void LocalDataSource::GetTags(const DataSourceRequest& request,
                              ResultHolder* holder) {
  string result = handler_->Execute(request.request().c_str(),
                                    request.language(),
                                    request.client_path());
  holder->set_result(result);
}
