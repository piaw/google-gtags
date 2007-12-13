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
// An abstract interface for making tags RPC queries to the GTags server.
//
// The TagsServiceUser should send the request and, upon RPC completion, set the
// result or failure of the ResultHolder.  No format for the target is provided
// enforced here as string addresses are used internally while address-port
// pairs are used externally.
//
// The tags request is made using only a string as that is all that the
// TagsRequestHandler uses to process the request on the server.

#ifndef TOOLS_TAGS_TAGS_SERVICE_H__
#define TOOLS_TAGS_TAGS_SERVICE_H__

#include "strutil.h"

class ResultHolder;

namespace gtags {

class TagsServiceUser {
 public:
  virtual ~TagsServiceUser() {}

  virtual void GetTags(const string &request, ResultHolder *holder) = 0;
};

}  // namespace gtags

#endif  // TOOLS_TAGS_TAGS_SERVICE_H__
