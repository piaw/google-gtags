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
// TagsServer provides an abstract interface for the server that serves tags
// information over the network. Subclasses should only handle basic network I/O
// and use tags_request_handler to respond to requests. Implmentations of this
// abstract class should override Loop().

#ifndef TOOLS_TAGS_TAGSSERVER_H__
#define TOOLS_TAGS_TAGSSERVER_H__

#include <string>

class TagsRequestHandler;

// Abstract class for a server that responds to network requests. It uses
// a TagsRequestHandler to translate input into output and it relies on
// subclasses to implement init and loop method to do network I/O
class TagsServer {
 public:
  // Stores a reference to TagRequestHandler to do business logic.
  TagsServer(TagsRequestHandler * tags_request_handler) :
      tags_request_handler_(tags_request_handler) {}
  virtual ~TagsServer() {}

  // Implement this function to start listening
  // Should not return until explicitly killed
  virtual void Loop() = 0;

 protected:
  TagsRequestHandler * tags_request_handler_;

};

#endif  // TOOLS_TAGS_TAGSSERVER_H__
