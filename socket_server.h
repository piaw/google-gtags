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
// SocketServer is an implementation of TagsServer using sockets

#ifndef TOOLS_TAGS_SOCKET_SERVER_H__
#define TOOLS_TAGS_SOCKET_SERVER_H__

#include "tagsserver.h"

class SocketServer : public TagsServer {
 public:
  SocketServer(TagsRequestHandler * handler) : TagsServer(handler) {}

  void Loop();
};

#endif  // TOOLS_TAGS_SOCKET_SERVER_H__
