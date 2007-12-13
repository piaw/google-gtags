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
// This is a mock socket library to simplify socket-based testing.

#ifndef TOOLS_TAGS_MOCK_SOCKET_H__
#define TOOLS_TAGS_MOCK_SOCKET_H__

#include "pollserver.h"
#include "socket.h"

namespace gtags {

extern const char* kBadAddress;
extern const char* kLocalhostName;
extern const char* kLocalhostIP;

class LoopCountingPollServer : public PollServer {
  static const int kShortLoopCount = 5;
  static const int kMediumLoopCount = 50;

 public:
  LoopCountingPollServer(unsigned int max_fds);
  virtual ~LoopCountingPollServer() {}

  virtual void ShortLoop() { LoopFor(kShortLoopCount); }
  virtual void MediumLoop() { LoopFor(kMediumLoopCount); }
  virtual void LoopFor(int max_count);
};

class MockClientSocket : public Socket {
 public:
  MockClientSocket(int fd, PollServer *pollserver, int port);
  virtual ~MockClientSocket();

  virtual void HandleWrite();

  struct sockaddr_in addr_;
  bool connected_;
};

class MockListenerSocket : public Socket {
 public:
  MockListenerSocket(int fd, PollServer *pollserver,
                     int port, bool keep_alive = false);
  virtual ~MockListenerSocket();

  virtual void HandleRead();

  int accepted_;
  int accepted_fd_;
  bool keep_alive_;
};

class MockConnectedSocket : public ConnectedSocket {
 public:
  MockConnectedSocket(int fd, PollServer *pollserver);
  virtual ~MockConnectedSocket() {}

  virtual void HandleDisconnected();

  int disconnected_count_;

  // publicize ConnectedSocket's buffers
  using ConnectedSocket::inbuf_;
  using ConnectedSocket::outbuf_;
};

}  // namespace gtags

#endif  // TOOLS_TAGS_MOCK_SOCKET_H__
