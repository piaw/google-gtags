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

#include "mock_socket.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace gtags {

const char* kBadAddress = "x.7.y.7.z";
const char* kLocalhostName = "localhost";
const char* kLocalhostIP = "127.0.0.1";

const int kPollTimeout = 5;

LoopCountingPollServer::LoopCountingPollServer(unsigned int max_fds)
  : PollServer(max_fds) {}

void LoopCountingPollServer::LoopFor(int max_count) {
  for (int i = 0; i < max_count; ++i)
    LoopOnce(kPollTimeout);
}

MockClientSocket::MockClientSocket(int fd, PollServer *pollserver, int port)
    : Socket(fd, pollserver), connected_(false) {
  fcntl(fd_, F_SETFL, O_NONBLOCK);
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port);
  addr_.sin_addr.s_addr = inet_addr(kLocalhostIP);
  memset(&(addr_.sin_zero), '\0', 8);  // zero the rest of the struct
}

MockClientSocket::~MockClientSocket() { if (!connected_) Close(); }

void MockClientSocket::HandleWrite() {
  int success = connect(fd_,
                        (struct sockaddr*) &addr_,
                        sizeof(struct sockaddr));
  if (success != -1) {
    ps_->Unregister(this);
    connected_ = true;
  } else if (errno != EINPROGRESS) {
    LOG(INFO) << "Connect failed ("
              << errno << "=" << strerror(errno) << ")";
  }
}

MockListenerSocket::MockListenerSocket(
    int fd, PollServer *pollserver,
    int port, bool keep_alive) :
    Socket(fd, pollserver), accepted_(0), accepted_fd_(-1),
    keep_alive_(keep_alive) {
  fcntl(fd_, F_SETFL, O_NONBLOCK);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  int result = bind(fd_, (struct sockaddr*)&addr,
                    static_cast<socklen_t>(sizeof(addr)));
  CHECK_NE(result, -1) << "Could not bind to port " << port;

  result = listen(fd_, 3);
  CHECK_NE(result, -1) << "Could not listen";
}

MockListenerSocket::~MockListenerSocket() { Close(); }

void MockListenerSocket::HandleRead() {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  accepted_fd_ = accept(fd_, (struct sockaddr *)&addr, &addrlen);

  if (accepted_fd_ != -1) {
    accepted_++;
    if (keep_alive_)
      fcntl(accepted_fd_, F_SETFL, O_NONBLOCK);
    else
      close(accepted_fd_);
  } else if (errno != EWOULDBLOCK) {
    LOG(INFO) << "Accept failed (" << errno << "=" << strerror(errno) << ").";
  }
}

MockConnectedSocket::MockConnectedSocket(int fd, PollServer *pollserver)
    : ConnectedSocket(fd, pollserver), disconnected_count_(0) {}

void MockConnectedSocket::HandleDisconnected() {
  disconnected_count_++;
}

}  // namespace gtags
