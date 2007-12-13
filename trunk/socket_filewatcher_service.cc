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
// The SocketFileWatchedServiceProvider starts a PollServer and a ListenerSocket
// and then waits to receive connection attempts.  When an attempt is received,
// it accepts it and spawns a FileWatcherSocket to handle the request.
//
// Communication is done via s-expressions.  The format for adding/removing
// watched directories is
//   (<action> (dirs ...) (excludes ...))
// where action is either 'add' or 'remove'.  The dirs and excludes parameter
// should each be a non-empty list of directories.  Each directory should be
// written as an absolute path and is enclosed in double quotes.
// In addition, the excludes parameter is optional.
//
//   Examples:
//     (add (dirs "/home/nigdsouza/gtags"))
//     (add (dirs "/usr/include") (excludes "/usr/include/sys"))
//     (remove (dirs "/home/nigdsouza" "/usr/include"))
//
// For consistency with other communications, commands are sent with a trailing
// '\n' and are not processed until the '\n' is received.
//
// Callback functions are wrapped in anonymous namespaces so that they don't
// conflict with similarly-named functions in other .cc files.  That scenario
// produces linker warning with our external compiler.

#include "socket_filewatcher_service.h"

#include "filewatcherrequesthandler.h"
#include "pollserver.h"
#include "sexpression.h"
#include "sexpression_util.h"
#include "sexpression_util-inl.h"
#include "socket.h"
#include "strutil.h"

namespace gtags {

namespace {

const char* kLocalhost = "127.0.0.1";

const char* kCommandAdd = "add";
const char* kCommandRemove = "remove";

class FileWatcherSocket : public ConnectedSocket {
 public:
  FileWatcherSocket(int socket_fd, PollServer *ps,
                    FileWatcherRequestHandler *handler) :
      ConnectedSocket(socket_fd, ps), handler_(handler) {}

 protected:
  virtual bool HandleReceived() {
    // Wait for a \n terminated string
    if (inbuf_.length() < 1 || inbuf_[inbuf_.length()-1] != '\n')
      return false;

    LOG(INFO) << "Processing File Watcher Service command: " << inbuf_;

    SExpression* sexpr = SExpression::Parse(inbuf_);

    if (!sexpr) {
      LOG(INFO) << "Could not process malformed sexpression: " << inbuf_;
    } else {
      // Parse action
      SExpression::const_iterator iter = sexpr->Begin();
      if (iter != sexpr->End() && iter->IsAtom()) {
        string action = iter->Repr();
        ++iter;

        list<string> dirs;
        list<string> excludes;

        // Parse attributes (dirs, excludes)
        for (; iter != sexpr->End(); ++iter) {
          if (!iter->IsList()) {
            LOG(INFO) << "Skipping: " << iter->Repr();
            continue;
          }

          SExpression::const_iterator attribute_iter = iter->Begin();

          if (attribute_iter != iter->End() && attribute_iter->IsAtom()) {
            string attribute = attribute_iter->Repr();
            ++attribute_iter;

            if (attribute == "dirs") {
              ReadList(attribute_iter, iter->End(), &dirs);
            } else if (attribute == "excludes") {
              ReadList(attribute_iter, iter->End(), &excludes);
            } else {
              LOG(INFO) << "Skipping: " << iter->Repr();
            }
          }
        }

        // Process the action & attributes
        if (dirs.size() > 0) {
          if (action == kCommandAdd) {
            handler_->Add(dirs, excludes);
          } else if (action == kCommandRemove) {
            handler_->Remove(dirs, excludes);
          } else {
            LOG(INFO) << "Skipping action: " << action;
          }
        }
      }
    }

    delete sexpr;

    Close();
    delete this;

    return false;
  }

  FileWatcherRequestHandler *handler_;
};

ConnectedSocket* CreateFileWatcherSocket(
    FileWatcherRequestHandler *handler, int socket_fd, PollServer *ps) {
  return new FileWatcherSocket(socket_fd, ps, handler);
}

}  // namespace

void SocketFileWatcherServiceProvider::Run() {
  PollServer ps(2);
  ListenerSocket *listener = ListenerSocket::Create(
      port_, &ps, CallbackFactory::CreatePermanent(
          &CreateFileWatcherSocket, handler_));

  CHECK(listener != NULL)
      << "Unable to start listener for File Watcher Service";

  servicing_ = true;
  ps.Loop();

  delete listener;
}

namespace {

void DoneRPC(PollServer *ps, bool *success, const string& response) {
  LOG(INFO) << "File Watcher Service RPC received: " << response;
  *success = true;
  ps->ForceLoopExit();
}

void RPCError(PollServer* ps) {
  LOG(INFO) << "Version Service RPC failed";
  ps->ForceLoopExit();
}

void ConvertToString(const list<string> &dirs,
                     const list<string> &excludes,
                     string *command) {
  command->append(" (dirs");
  for (list<string>::const_iterator iter = dirs.begin();
       iter != dirs.end(); ++iter) {
    command->append(" \"");
    command->append(*iter);
    command->append("\"");
  }
  command->append(")");

  command->append(" (excludes");
  for (list<string>::const_iterator iter = excludes.begin();
       iter != excludes.end(); ++iter) {
    command->append(" \"");
    command->append(*iter);
    command->append("\"");
  }
  command->append(")");
}

}  // namespace

bool SocketFileWatcherServiceUser::Add(const list<string> &dirs,
                                       const list<string> &excludes) {
  PollServer ps(1);
  string command;
  command += "(";
  command += kCommandAdd;
  ConvertToString(dirs, excludes, &command);
  command += ")\n";

  LOG(INFO) << "Sending File Watcher Service command on port "
            << port_ << ": " << command;

  bool success = false;

  if (RPCSocket::PerformRPC(
          kLocalhost, port_, &ps, command,
          CallbackFactory::Create(&DoneRPC, &ps, &success),
          CallbackFactory::Create(&RPCError, &ps))) {
    ps.Loop();
  }

  return success;
}

bool SocketFileWatcherServiceUser::Remove(const list<string> &dirs,
                                          const list<string> &excludes) {
  PollServer ps(1);
  string command;
  command += "(";
  command += kCommandRemove;
  ConvertToString(dirs, excludes, &command);
  command += ")\n";

  LOG(INFO) << "Sending File Watcher Service command on port "
            << port_ << ": " << command;

  bool success = false;

  if (RPCSocket::PerformRPC(
          kLocalhost, port_, &ps, command,
          CallbackFactory::Create(&DoneRPC, &ps, &success),
          CallbackFactory::Create(&RPCError, &ps))) {
    ps.Loop();
  }

  return success;
}

}  // namespace gtags
