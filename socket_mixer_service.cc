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

#include "socket_mixer_service.h"

#include "pollserver.h"
#include "socket.h"
#include "mixerrequesthandler.h"

namespace gtags {

class MixerSocket : public ConnectedSocket {
 public:
  MixerSocket(int socket_fd, PollServer *ps, MixerRequestHandler *handler) :
      ConnectedSocket(socket_fd, ps), handler_(handler) {}

 protected:
  virtual bool HandleReceived() {
    // Wait for a \n terminated string
    if (inbuf_.length() < 1 || inbuf_[inbuf_.length()-1] != '\n')
      return false;

    LOG(INFO) << "Processing Mixer Service command: " << inbuf_;

    // Chop off the '\n'
    inbuf_.erase(inbuf_.length() - 1);
    handler_->Execute(inbuf_.c_str(), gtags::CallbackFactory::Create(
                          this, &MixerSocket::HandleMixerResponse));

    return false;
  }

  virtual void HandleSent() {
    Close();
    delete this;
  }

  void HandleMixerResponse(const string &response) {
    LOG(INFO) << "Mixer Service response: " << response;
    outbuf_ += response;
  }

  MixerRequestHandler *handler_;
};

ConnectedSocket* CreateMixerSocket(MixerRequestHandler *handler,
                                   int socket_fd, PollServer *ps) {
  return new MixerSocket(socket_fd, ps, handler);
}

void SocketMixerServiceProvider::Start(MixerRequestHandler *handler) {
  PollServer ps(2);
  ListenerSocket *listener = ListenerSocket::Create(
      port_, &ps, CallbackFactory::CreatePermanent(
          &CreateMixerSocket, handler));

  pollserver_ = &ps;
  servicing_ = true;

  CHECK(listener != NULL) << "Unable to start listener for Mixer Service";

  ps.Loop();

  servicing_ = false;
  pollserver_ = NULL;
  delete listener;
}

void SocketMixerServiceProvider::Stop() {
  if (pollserver_)
    pollserver_->ForceLoopExit();
}

}  // namespace gtags
