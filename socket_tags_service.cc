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
// The SocketTagsServiceUser performs an RPC to the GTags server and then waits
// for a result.  The waiting is done by spawning an RPCWaiter thread to contain
// the PollServer loop.
//
// Communication with the GTags server is done by forward the unmodified string.
//
// For consistency with the existing GTags server, commands are sent with a
// trailing '\n' as the GTags server will not process the command until the '\n'
// is received.
//
// The structure of the RPCs required each SocketTagsServiceUser to use it's own
// thread and PollServer for waiting.  This could be improved by sharing a
// PollServer amongst all SocketTagsServiceUsers and keeping one extra thread to
// contain the PollServer loop.
// However, this means that there will always be another thread around, though
// it will mostly be blocked on the poll call.  In contrast, the current
// approach creates more threads, though they only exist for the duration of the
// RPC.  This approach was chosen for its resemblance to the internal version.

#include "socket_tags_service.h"

#include "gtagsmixer.h"
#include "pollserver.h"
#include "socket.h"
#include "thread.h"

namespace gtags {

const char* TAGS_SERVICE_ERROR = "Tags Service was unable to complete RPC";

void DoneGetTags(PollServer *ps, ResultHolder *holder,
                 const string& response) {
  LOG(INFO) << "Tags Service RPC received: " << response;
  holder->set_result(response);
  ps->ForceLoopExit();
}

void RPCError(PollServer* ps, ResultHolder *holder) {
  LOG(INFO) << "Tags Service RPC failed";
  holder->set_failure(TAGS_SERVICE_ERROR);
  LOG(INFO) << "Set failure";
  ps->ForceLoopExit();
}

class RPCWaiter : public Thread {
 public:
  RPCWaiter(PollServer *ps) : Thread(), ps_(ps) {}
  virtual ~RPCWaiter() {}

 protected:
  virtual void Run() {
    ps_->Loop();
    delete ps_;
    LOG(INFO) << "Finished RPC";
    delete this;
  }

  PollServer *ps_;
};

void SocketTagsServiceUser::GetTags(
    const string &request, ResultHolder *holder) {
  string command = request + '\n';

  PollServer *ps = new PollServer(1);

  LOG(INFO) << "Sending to " << address_ << ':' << port_ << ": " << request;

  if (RPCSocket::PerformRPC(
          address_.c_str(), port_, ps, command,
          CallbackFactory::Create(&DoneGetTags, ps, holder),
          CallbackFactory::Create(&RPCError, ps, holder))) {
    Thread *rpc_waiter = new RPCWaiter(ps);
    rpc_waiter->SetJoinable(true);
    rpc_waiter->Start();
  } else {
    delete ps;
  }

  LOG(INFO) << "Finished RPC";
}

}  // namespace gtags
