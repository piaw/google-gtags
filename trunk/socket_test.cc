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

#include <errno.h>
#include <sys/socket.h>

#include "gtagsunit.h"
#include "socket.h"

#include "mock_socket.h"
#include "pollserver.h"
#include "socket_util.h"

namespace gtags {

// *** Common *** //

class CallbackCounter {
 public:
  CallbackCounter() : count_(0) {}
  void Reset() { count_ = 0; }
  void Call() { count_++; }
  static void Ignore() {}
  int count_;
};

class MockConnectedCreator {
 public:
  MockConnectedCreator() : created_(0) {
  }
  void Reset() { created_ = 0; }
  ConnectedSocket* CountCreations(int fd, PollServer *pollserver) {
    created_++;
    return CreateNULL(fd, pollserver);
  }
  static ConnectedSocket* CreateNULL(int fd, PollServer *pollserver) {
    int result = close(fd);
    EXPECT_NE(result, -1);
    return NULL;
  }
  int created_;
};

// *** Socket Tests *** //

class MockSocket : public Socket {
 public:
  MockSocket(int fd, PollServer *pollserver) : Socket(fd, pollserver) {}
  virtual ~MockSocket() {}
};

TEST(SocketTest, CloseTest) {
  PollServer pollserver(1);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockSocket sock(fd, &pollserver);
    sock.Close();

    int result = close(fd);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
  }  // guarantee destruction of Pollables before PollServer
}

// *** Listener Tests *** //

TEST(ListenerTest, PermanentCallbackCreateTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  ListenerSocket *listener;

  // Create fails with non-permanent callback
  listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::Create(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener == NULL);

  // Create succeeds with permanent callback
  listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener != NULL);
  EXPECT_NE(listener->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(listener));

  delete listener;
}

TEST(ListenerTest, PortCreateTest) {
  const int port1 = FindAvailablePort();

  PollServer pollserver(2);
  ListenerSocket *listener1;
  ListenerSocket *listener2;

  // Create succeeds with unused port1
  listener1 = ListenerSocket::Create(
      port1, &pollserver, CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener1 != NULL);
  EXPECT_NE(listener1->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(listener1));

  // Create fails with used port1
  listener2 = ListenerSocket::Create(
      port1, &pollserver, CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener2 == NULL);

  // Create succeeds with unused port2
  const int port2 = FindAvailablePort();
  listener2 = ListenerSocket::Create(
      port2, &pollserver, CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener2 != NULL);
  EXPECT_NE(listener2->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(listener2));

  delete listener2;
  delete listener1;
}

TEST(ListenerTest, CloseOnDestructTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  ListenerSocket *listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(listener != NULL);
  EXPECT_NE(listener->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(listener));

  int fd = listener->fd();
  delete listener;

  // fd is already closed
  int result = close(fd);
  EXPECT_EQ(result, -1);
}

TEST(ListenerTest, WrongPortTest) {
  const int listen_port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);
  ListenerSocket *listener = ListenerSocket::Create(
      listen_port, &pollserver, CallbackFactory::CreatePermanent(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(listener != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    const int connect_port = FindAvailablePort();
    MockClientSocket client(fd, &pollserver, connect_port);
    EXPECT_FALSE(client.connected_);

    pollserver.ShortLoop();
    EXPECT_FALSE(client.connected_);
    EXPECT_EQ(connected_creator.created_, 0);
  }  // guarantee destruction of Pollables before PollServer

  delete listener;
}

TEST(ListenerTest, SingleClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);
  ListenerSocket *listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::CreatePermanent(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(listener != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockClientSocket client(fd, &pollserver, port);
    EXPECT_FALSE(client.connected_);

    pollserver.ShortLoop();
    EXPECT_TRUE(client.connected_);
    EXPECT_EQ(connected_creator.created_, 1);
  }  // guarantee destruction of Pollables before PollServer

  delete listener;
}

TEST(ListenerTest, MultipleSequentialClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);
  ListenerSocket *listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::CreatePermanent(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(listener != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  for (int i = 0; i < 3; ++i) {
    connected_creator.created_ = 0;

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    ASSERT_NE(fd, -1);

    MockClientSocket client(fd, &pollserver, port);
    EXPECT_FALSE(client.connected_);

    pollserver.ShortLoop();
    EXPECT_TRUE(client.connected_);
    EXPECT_EQ(connected_creator.created_, 1);
  }  // guarantee destruction of Pollables before PollServer

  delete listener;
}

TEST(ListenerTest, MultipleSimultaneousClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(4);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);
  ListenerSocket *listener = ListenerSocket::Create(
      port, &pollserver, CallbackFactory::CreatePermanent(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(listener != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd1 = socket(PF_INET, SOCK_STREAM, 0);
  int fd2 = socket(PF_INET, SOCK_STREAM, 0);
  int fd3 = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd1, -1);
  ASSERT_NE(fd2, -1);
  ASSERT_NE(fd3, -1);

  {
    MockClientSocket client1(fd1, &pollserver, port);
    MockClientSocket client2(fd2, &pollserver, port);
    MockClientSocket client3(fd3, &pollserver, port);
    EXPECT_FALSE(client1.connected_);
    EXPECT_FALSE(client2.connected_);
    EXPECT_FALSE(client3.connected_);
    EXPECT_EQ(connected_creator.created_, 0);

    pollserver.ShortLoop();
    EXPECT_TRUE(client1.connected_);
    EXPECT_TRUE(client2.connected_);
    EXPECT_TRUE(client3.connected_);
    EXPECT_EQ(connected_creator.created_, 3);
  }  // guarantee destruction of Pollables before PollServer

  delete listener;
}

// *** ClientSocket Tests *** //

TEST(ClientTest, NonPermanentCallbackCreateTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  ClientSocket *client;

  // Create fails with permanent callback
  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::CreatePermanent(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(client == NULL);

  // Create succeeds with non-permanent callback
  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(
          &MockConnectedCreator::CreateNULL));
  EXPECT_TRUE(client != NULL);
  EXPECT_NE(client->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(client));

  delete client;

  // Create fails with permanent callback
  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::CreatePermanent(&CallbackCounter::Ignore));
  EXPECT_TRUE(client == NULL);

  // Create succeeds with non-permanent callback
  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::Create(&CallbackCounter::Ignore));
  EXPECT_TRUE(client != NULL);
  EXPECT_NE(client->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(client));

  delete client;
}

TEST(ClientTest, CallbackDeletedTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  CallbackCounter counter;
  EXPECT_EQ(counter.count_, 0);

  ClientSocket *client;

  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::Create(&counter, &CallbackCounter::Call));
  EXPECT_TRUE(client != NULL);
  EXPECT_EQ(counter.count_, 0);

  delete client;

  counter.Reset();
  EXPECT_EQ(counter.count_, 0);

  client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::Create(&counter, &CallbackCounter::Call));
  EXPECT_TRUE(client != NULL);
  EXPECT_EQ(counter.count_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockListenerSocket mls(fd, &pollserver, port);
    EXPECT_EQ(mls.accepted_, 0);

    pollserver.ShortLoop();
    EXPECT_EQ(mls.accepted_, 1);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ClientTest, ErrorCallbackTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  CallbackCounter counter;
  EXPECT_EQ(counter.count_, 0);

  ClientSocket *client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::Create(&counter, &CallbackCounter::Call));
  EXPECT_TRUE(client != NULL);
  EXPECT_EQ(counter.count_, 0);

  delete client;
  EXPECT_EQ(counter.count_, 1);
}

TEST(ClientTest, BadAddressTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  CallbackCounter counter;
  EXPECT_EQ(counter.count_, 0);

  ClientSocket *client = ClientSocket::Create(
      kBadAddress, port, &pollserver,
      CallbackFactory::Create(&MockConnectedCreator::CreateNULL),
      CallbackFactory::Create(&counter, &CallbackCounter::Call));
  EXPECT_TRUE(client == NULL);
  EXPECT_EQ(counter.count_, 1);
}

TEST(ClientTest, WrongPortTest) {
  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    const int listen_port = FindAvailablePort();
    MockListenerSocket listener(fd, &pollserver, listen_port);
    EXPECT_EQ(listener.accepted_, 0);

    const int connect_port = FindAvailablePort();
    ClientSocket *client = ClientSocket::Create(
        kLocalhostIP, connect_port, &pollserver, CallbackFactory::Create(
            &connected_creator, &MockConnectedCreator::CountCreations));
    EXPECT_TRUE(client != NULL);
    EXPECT_EQ(connected_creator.created_, 0);

    pollserver.ShortLoop();
    EXPECT_EQ(listener.accepted_, 0);
    EXPECT_EQ(connected_creator.created_, 0);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ClientTest, SingleClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);

  ClientSocket *client = ClientSocket::Create(
      kLocalhostIP, port, &pollserver, CallbackFactory::Create(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(client != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockListenerSocket listener(fd, &pollserver, port);
    EXPECT_EQ(listener.accepted_, 0);

    pollserver.ShortLoop();
    EXPECT_EQ(listener.accepted_, 1);
    EXPECT_EQ(connected_creator.created_, 1);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ClientTest, SingleClientToNamedHostTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);

  ClientSocket *client = ClientSocket::Create(
      kLocalhostName, port, &pollserver, CallbackFactory::Create(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(client != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockListenerSocket listener(fd, &pollserver, port);
    EXPECT_EQ(listener.accepted_, 0);

    pollserver.ShortLoop();
    EXPECT_EQ(listener.accepted_, 1);
    EXPECT_EQ(connected_creator.created_, 1);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ClientTest, MultipleSequentialClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(2);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockListenerSocket listener(fd, &pollserver, port);
    EXPECT_EQ(listener.accepted_, 0);

    for (int i = 0; i < 3; ++i) {
      listener.accepted_ = 0;
      connected_creator.created_ = 0;

      ClientSocket *client = ClientSocket::Create(
          kLocalhostIP, port, &pollserver, CallbackFactory::Create(
              &connected_creator, &MockConnectedCreator::CountCreations));
      EXPECT_TRUE(client != NULL);
      EXPECT_EQ(connected_creator.created_, 0);

      pollserver.ShortLoop();
      EXPECT_EQ(listener.accepted_, 1);
      EXPECT_EQ(connected_creator.created_, 1);
    }
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ClientTest, MultipleSimultaneousClientTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(4);
  MockConnectedCreator connected_creator;
  EXPECT_EQ(connected_creator.created_, 0);

  ClientSocket *client1 = ClientSocket::Create(
      kLocalhostIP, port, &pollserver, CallbackFactory::Create(
          &connected_creator, &MockConnectedCreator::CountCreations));
  ClientSocket *client2 = ClientSocket::Create(
      kLocalhostIP, port, &pollserver, CallbackFactory::Create(
          &connected_creator, &MockConnectedCreator::CountCreations));
  ClientSocket *client3 = ClientSocket::Create(
      kLocalhostIP, port, &pollserver, CallbackFactory::Create(
          &connected_creator, &MockConnectedCreator::CountCreations));
  EXPECT_TRUE(client1 != NULL);
  EXPECT_TRUE(client2 != NULL);
  EXPECT_TRUE(client3 != NULL);
  EXPECT_EQ(connected_creator.created_, 0);

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(fd, -1);

  {
    MockListenerSocket listener(fd, &pollserver, port);
    EXPECT_EQ(listener.accepted_, 0);

    pollserver.ShortLoop();
    EXPECT_EQ(listener.accepted_, 3);
    EXPECT_EQ(connected_creator.created_, 3);
  }  // guarantee destruction of Pollables before PollServer
}

// *** ConnectedSocket Tests *** //

TEST(ConnectedTest, RegistrationTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(3);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    ConnectedSocket connected1(listener.accepted_fd_, &pollserver);
    EXPECT_TRUE(pollserver.IsRegistered(&connected1));
    ConnectedSocket connected2(connect_fd, &pollserver);
    EXPECT_TRUE(pollserver.IsRegistered(&connected2));
    EXPECT_FALSE(pollserver.IsRegistered(&client));
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, CloseOnDestructTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(3);
  MockConnectedCreator connected_creator;

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    {
      ConnectedSocket connected1(listener.accepted_fd_, &pollserver);
      ConnectedSocket connected2(connect_fd, &pollserver);
    }  // destruct connected sockets

    int result;

    result = close(listener.accepted_fd_);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);

    result = close(connect_fd);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, EBADF);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, DisconnectTest) {
  const int port = FindAvailablePort();
  const string MSG = "a\n";

  LoopCountingPollServer pollserver(2);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    MockConnectedSocket connected(connect_fd, &pollserver);
    EXPECT_EQ(connected.disconnected_count_, 0);

    int result = close(listener.accepted_fd_);
    EXPECT_NE(result, -1);

    pollserver.ShortLoop();
    EXPECT_EQ(connected.disconnected_count_, 1);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, ShortSendTest) {
  const int port = FindAvailablePort();
  const string MSG = "a\n";
  const int BUFFER_LENGTH = 5;

  LoopCountingPollServer pollserver(2);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    MockConnectedSocket connected(connect_fd, &pollserver);
    connected.outbuf_ += MSG;
    EXPECT_STREQ(connected.outbuf_.c_str(), MSG.c_str());

    pollserver.ShortLoop();

    char buf[BUFFER_LENGTH];
    int result = recv(listener.accepted_fd_, &buf, BUFFER_LENGTH-1, 0);
    EXPECT_EQ(result, MSG.length());
    buf[result] = '\0';
    EXPECT_STREQ(buf, MSG.c_str());

    close(listener.accepted_fd_);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, LongSendTest) {
  const int port = FindAvailablePort();
  const string MSG =
      "'Ah, felicitations, Mr. Daniel!'\n'De rien, de rien.'\n";
  const int BUFFER_LENGTH = 60;

  LoopCountingPollServer pollserver(2);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    MockConnectedSocket connected(connect_fd, &pollserver);
    connected.outbuf_ += MSG;
    EXPECT_STREQ(connected.outbuf_.c_str(), MSG.c_str());

    pollserver.ShortLoop();

    char buf[BUFFER_LENGTH];
    int result = recv(listener.accepted_fd_, &buf, BUFFER_LENGTH-1, 0);
    EXPECT_EQ(result, MSG.length());
    buf[result] = '\0';
    EXPECT_STREQ(buf, MSG.c_str());

    close(listener.accepted_fd_);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, ShortReceiveTest) {
  const int port = FindAvailablePort();
  const string MSG = "a\n";

  LoopCountingPollServer pollserver(2);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    int result = send(listener.accepted_fd_, MSG.c_str(), MSG.length(), 0);
    EXPECT_EQ(result, MSG.length());

    MockConnectedSocket connected(connect_fd, &pollserver);
    EXPECT_STREQ(connected.inbuf_.c_str(), "");

    pollserver.ShortLoop();

    EXPECT_STREQ(connected.inbuf_.c_str(), MSG.c_str());

    close(listener.accepted_fd_);
  }  // guarantee destruction of Pollables before PollServer
}

TEST(ConnectedTest, LongReceiveTest) {
  const int port = FindAvailablePort();
  const string MSG =
      "'Ah, felicitations, Mr. Daniel!'\n'De rien, de rien.'\n";

  LoopCountingPollServer pollserver(2);

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    int result = send(listener.accepted_fd_, MSG.c_str(), MSG.length(), 0);
    EXPECT_EQ(result, MSG.length());

    MockConnectedSocket connected(connect_fd, &pollserver);
    EXPECT_STREQ(connected.inbuf_.c_str(), "");

    pollserver.ShortLoop();

    EXPECT_STREQ(connected.inbuf_.c_str(), MSG.c_str());

    close(listener.accepted_fd_);
  }  // guarantee destruction of Pollables before PollServer
}

const int NUM_MSGS = 5;
static string MSGS[NUM_MSGS] = {
  "Hello?",
  "Hi?",
  "Hey!  How's it going?",
  "Pretty good, I guess.  You?",
  "Ditto for me!"
};

class ConversationConnectedSocket : public ConnectedSocket {
 public:
  ConversationConnectedSocket(
      int fd, PollServer *pollserver, bool should_receive) :
      ConnectedSocket(fd, pollserver),
      should_receive_(should_receive), next_(0) {
    if (!should_receive_)
      outbuf_ += MSGS[next_];
  }
  virtual ~ConversationConnectedSocket() {}

  virtual bool HandleReceived() {
    EXPECT_TRUE(should_receive_);
    // once we've received the expected message
    if (inbuf_ == MSGS[next_]) {
      next_++;
      // send the next one
      if (next_ < NUM_MSGS)
        outbuf_ += MSGS[next_];
      should_receive_ = false;
      return true;
    }
    return false;
  }
  virtual void HandleSent() {
    EXPECT_FALSE(should_receive_);
    next_++;
    should_receive_ = (next_ < NUM_MSGS);
  }

  bool should_receive_;
  int next_;
};

TEST(ConnectedTest, ConversationTest) {
  const int port = FindAvailablePort();

  LoopCountingPollServer pollserver(3);
  MockConnectedCreator connected_creator;

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  int connect_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);
  ASSERT_NE(connect_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    MockClientSocket client(connect_fd, &pollserver, port);

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);
    ASSERT_TRUE(client.connected_);

    ConversationConnectedSocket connected1(
        listener.accepted_fd_, &pollserver, false);
    ConversationConnectedSocket connected2(
        connect_fd, &pollserver, true);
    EXPECT_EQ(connected1.next_, 0);
    EXPECT_EQ(connected2.next_, 0);

    pollserver.LoopFor(NUM_MSGS * 2);
    EXPECT_EQ(connected1.next_, NUM_MSGS);
    EXPECT_EQ(connected2.next_, NUM_MSGS);

  }  // guarantee destruction of Pollables before PollServer
}

// *** RPCSocket Tests *** //

class RPCCallee {
 public:
  static void DoNothing(const string &response) {}
  void StoreResponse(const string &response) { response_ = response; }
  string response_;
};

TEST(RPCSocket, NonPermanentCallbackCreateTest) {
  const int port = FindAvailablePort();

  PollServer pollserver(1);
  ClientSocket *client;

  // Create fails with permanent callback
  client = RPCSocket::PerformRPC(
      kLocalhostIP, port, &pollserver, "",
      CallbackFactory::CreatePermanent(&RPCCallee::DoNothing));
  EXPECT_TRUE(client == NULL);

  // Create succeeds with non-permanent callback
  client = RPCSocket::PerformRPC(
      kLocalhostIP, port, &pollserver, "",
      CallbackFactory::Create(&RPCCallee::DoNothing));
  EXPECT_TRUE(client != NULL);
  EXPECT_NE(client->fd(), -1);
  EXPECT_TRUE(pollserver.IsRegistered(client));

  delete client;
}

TEST(RPCTest, ResponseTest) {
  const int port = FindAvailablePort();
  const string QUESTION = "Gambit's real name?";
  const string ANSWER = "Remy LeBeau";
  const int BUFFER_LENGTH = 25;

  LoopCountingPollServer pollserver(3);
  MockConnectedCreator connected_creator;

  int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
  ASSERT_NE(listen_fd, -1);

  {
    MockListenerSocket listener(listen_fd, &pollserver, port, true);
    RPCCallee rpc_callee;
    ClientSocket *client = RPCSocket::PerformRPC(
        kLocalhostIP, port, &pollserver, QUESTION,
        CallbackFactory::Create(&rpc_callee, &RPCCallee::StoreResponse));
    EXPECT_TRUE(client != NULL);
    EXPECT_NE(client->fd(), -1);
    EXPECT_TRUE(pollserver.IsRegistered(client));
    EXPECT_STREQ(rpc_callee.response_.c_str(), "");

    pollserver.ShortLoop();
    ASSERT_EQ(listener.accepted_, 1);
    ASSERT_NE(listener.accepted_fd_, -1);

    pollserver.ShortLoop();

    int result;

    // receive the question
    char buf[BUFFER_LENGTH];
    result = recv(listener.accepted_fd_, &buf, BUFFER_LENGTH-1, 0);
    EXPECT_EQ(result, QUESTION.length());
    buf[result] = '\0';
    EXPECT_STREQ(buf, QUESTION.c_str());

    // send the answer
    result = send(listener.accepted_fd_, ANSWER.c_str(), ANSWER.length(), 0);
    EXPECT_EQ(result, ANSWER.length());

    // finish the rpc
    result = close(listener.accepted_fd_);
    EXPECT_NE(result, -1);

    // retrieve the response
    pollserver.ShortLoop();

    EXPECT_STREQ(rpc_callee.response_.c_str(), ANSWER.c_str());
  }  // guarantee destruction of Pollables before PollServer
}

}  // namespace gtags
