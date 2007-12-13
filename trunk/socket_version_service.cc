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
// The SocketVersionServiceProvider starts a PollServer and a ListenerSocket and
// then waits to receive connection attempts.  When an attempt is received, it
// accepts it and spawns a VersionSocket to handle the request.
//
// Communication is done via the VersionServiceCommand character codes.
//
// Messages from emacs end with a '\n', so for consistency, commands are sent
// with a trailing '\n' and commands are not processed until the '\n' is
// received.
//
// Callback functions are wrapped in anonymous namespaces so that they don't
// conflict with similarly-named functions in other .cc files.  That scenario
// produces linker warning with our external compiler.

#include "socket_version_service.h"

#include "pollserver.h"
#include "socket.h"
#include "strutil.h"

namespace gtags {

namespace {

const char* kLocalhost = "127.0.0.1";

enum VersionServiceCommand {
  GET_VERSION = 'v',
  SHUT_DOWN = '!'
};

class VersionSocket : public ConnectedSocket {
 public:
  VersionSocket(int socket_fd, PollServer *ps, int version) :
      ConnectedSocket(socket_fd, ps), version_(version) {}

 protected:
  virtual bool HandleReceived() {
    // Wait for a \n terminated string
    if (inbuf_.length() < 1 || inbuf_[inbuf_.length()-1] != '\n')
      return false;

    LOG(INFO) << "Processing Version Service command: " << inbuf_;

    switch (inbuf_[0]) {
      case GET_VERSION:
        outbuf_ += FastItoa(version_);
        break;
      case SHUT_DOWN:
        // Close first so the other side knows the RPC reached
        Close();
        exit(0);
    }

    return true;
  }

  virtual void HandleSent() {
    Close();
    delete this;
  }

  int version_;
};

ConnectedSocket* CreateVersionSocket(int version,
                                     int socket_fd, PollServer *ps) {
  return new VersionSocket(socket_fd, ps, version);
}

}  // namespace

void SocketVersionServiceProvider::Run() {
  PollServer ps(2);
  ListenerSocket *listener = ListenerSocket::Create(
      port_, &ps, CallbackFactory::CreatePermanent(
          &CreateVersionSocket, version_));

  CHECK(listener != NULL) << "Unable to start listener for Version Service";

  servicing_ = true;
  ps.Loop();

  delete listener;
}

namespace {

void DoneGetVersion(PollServer *ps, bool *success, int *version,
                    const string& response) {
  LOG(INFO) << "Version Service RPC received: " << response;
  *version = atoi(response.c_str());
  *success = (version != 0);
  ps->ForceLoopExit();
}

void DoneShutDown(PollServer *ps, bool *success,
                    const string& response) {
  LOG(INFO) << "Version Service RPC received: " << response;
  *success = true;
  ps->ForceLoopExit();
}

void RPCError(PollServer* ps) {
  LOG(INFO) << "Version Service RPC failed";
  ps->ForceLoopExit();
}

}  // namespace

bool SocketVersionServiceUser::GetVersion(int *version) {
  PollServer ps(1);
  string command;
  command += GET_VERSION;
  command += '\n';

  LOG(INFO) << "Sending Version Service command on port "
            << port_ << ": " << command;

  bool success = false;

  if (RPCSocket::PerformRPC(
          kLocalhost, port_, &ps, command,
          CallbackFactory::Create(&DoneGetVersion, &ps, &success, version),
          CallbackFactory::Create(&RPCError, &ps))) {
    ps.Loop();
  }

  return success;
}

bool SocketVersionServiceUser::ShutDown() {
  PollServer ps(1);
  string command;
  command += SHUT_DOWN;
  command += '\n';

  bool success = false;

  if (RPCSocket::PerformRPC(
          kLocalhost, port_, &ps, command,
          CallbackFactory::Create(&DoneShutDown, &ps, &success),
          CallbackFactory::Create(&RPCError, &ps))) {
    ps.Loop();
  }

  return success;
}

}  // namespace gtags
