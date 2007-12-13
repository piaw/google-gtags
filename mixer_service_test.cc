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

#include "mixer_service_test.h"

#include <sys/socket.h>

#include "mixer_service.h"
#include "mixerrequesthandler.h"
#include "mock_socket.h"
#include "thread.h"

namespace gtags {

class MixerProviderThread : public Thread {
 public:
  MixerProviderThread(MixerServiceProvider *mixer_provider,
                      MixerRequestHandler *handler)
      : mixer_provider_(mixer_provider), handler_(handler) {}

 protected:
  virtual void Run() {
    mixer_provider_->Start(handler_);
  }

  MixerServiceProvider *mixer_provider_;
  MixerRequestHandler *handler_;
};

class MockMixerRequestHandler : public MixerRequestHandler {
 public:
  MockMixerRequestHandler(const string &question, const string &answer)
      : MixerRequestHandler(NULL), question_(question), answer_(answer) {}

  virtual void Execute(const char* command,
                       ResponseCallback *response_callback) const {
    EXPECT_STREQ(command, question_.c_str());
    response_callback->Run(answer_);
    executed_ = true;
  }

  const string &question_;
  const string &answer_;

  static bool executed_;
};

// Must have external declaration for static class member.
bool MockMixerRequestHandler::executed_;

void RunServiceTest(MixerServiceProvider *mixer_provider, int port) {
  const string QUESTION = "Gambit's real name?";
  const string ANSWER = "Remy LeBeau";

  // Start the service.
  MockMixerRequestHandler handler(QUESTION, ANSWER);
  MixerProviderThread mixer_provider_thread(mixer_provider, &handler);
  mixer_provider_thread.SetJoinable(true);

  MockMixerRequestHandler::executed_ = false;
  mixer_provider_thread.Start();

  // Busy wait until the provider has started servicing
  while (!mixer_provider->servicing()) {}

  LoopCountingPollServer pollserver(1);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    // Connect a client.
    MockClientSocket client(fd, &pollserver, port);
    pollserver.ShortLoop();
    ASSERT_TRUE(client.connected_);

    MockConnectedSocket connected(fd, &pollserver);
    connected.outbuf_ += QUESTION;
    connected.outbuf_ += '\n';

    // Send the question
    pollserver.ShortLoop();

    // Busy wait until the handler has executed
    while (!MockMixerRequestHandler::executed_) {}

    // Retrieve the answer
    pollserver.MediumLoop();

    EXPECT_STREQ(connected.inbuf_.c_str(), ANSWER.c_str());
  }  // guarantee destruction of Pollables before PollServer

  // Stop the service.
  mixer_provider->Stop();
  mixer_provider_thread.Join();
}

}  // namespace gtags
