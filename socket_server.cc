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

#include "socket_server.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

#include "tagsprofiler.h"
#include "tagsoptionparser.h"
#include "tagsrequesthandler.h"

DEFINE_INT32(tags_port, 2222, "port to tags server");

// Maximum length of a request
const int kMaxTagLen = 512;

class SocketIO : public IOInterface {
 public:
  SocketIO(int listen_socket) {
    // a_addr_ is not actually a sockaddr, so we need to measure its size
    addrlen_ = sizeof(a_addr_);
    connected_socket_ = accept(listen_socket,
                              (struct sockaddr *)&a_addr_,
                              &addrlen_);
    source_ = inet_ntoa(a_addr_.sin_addr);
  }

  virtual ~SocketIO() {
    if (connected_socket_ != -1) {
      close(connected_socket_);
    }
  }

  virtual bool Input(char** input) {
    if (connected_socket_ == -1) {
      LOG(INFO) << "Connection error" << "\n";
      return false;
    }

    int bytesread = recv(connected_socket_, buf_, kMaxTagLen, 0);

    if ( bytesread == -1) {
      LOG(INFO) << "Error receiving" << "\n";
      return false;
    }

    LOG(INFO) << "bytes read: " << bytesread << "\n";

    // strip off \n\r and null terminate the string
    if (bytesread >= 2 && buf_[bytesread - 1] == '\n'
        && buf_[bytesread - 2] == '\r') {
      buf_[bytesread - 2] = '\0';
    }

    LOG(INFO) << buf_ << "\n";
    *input = buf_;

    return false;
  }

  virtual bool Output(const char * output) {
    const char* outbuf = output;
    int towrite = strlen(output);

    while (towrite > 0) {
      int wrote = write(connected_socket_, outbuf, towrite);
      if (wrote <= 0 ) {
        break;
      }
      outbuf += wrote;
      towrite -= wrote;
    }
    return false;
  }

  virtual void Close() {
    close(connected_socket_);
    connected_socket_ = -1;
  }

  virtual const char* Source() const {
    return source_.c_str();
  }

 private:
  struct sockaddr_in a_addr_;
  socklen_t addrlen_;
  int connected_socket_;
  char buf_[kMaxTagLen];
  string source_;
};

void SocketServer::Loop() {
  int listen_socket;
  struct sockaddr_in addr;

  listen_socket = socket(PF_INET, SOCK_STREAM, 0);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(GET_FLAG(tags_port));
  addr.sin_addr.s_addr = INADDR_ANY;

  CHECK(bind(listen_socket, (struct sockaddr *)&addr,
                   static_cast<socklen_t>(sizeof(addr))) != -1);

  int server_socket = listen(listen_socket, 3);
  CHECK(server_socket != -1);

  LOG(INFO) << "Tags server listening on port " << GET_FLAG(tags_port) << "\n";

  while(true) {
    SocketIO socket_io(listen_socket);
    TagsIOProfiler profiler(&socket_io, tags_request_handler_);
    profiler.Execute();
  }
}
