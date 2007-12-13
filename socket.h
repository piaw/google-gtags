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
// A basic socket library that allows for RPC communication.
//
// A ListenerSocket is used to listen for connection attempts on a specified
// port.  When it hears a connection attempt, it accepts it, and then calls
// the ConnectedCallback, passing it the new socket fd.
//
// A ClientSocket is used to attempt a connection to specified target ip/port
// pair.  When it establishes a connection, it calls the ConnectedCallback,
// passing it its own socket fd; i.e. the ClientSocket manages the socket fd
// first, and once a connection is established, that same socket fd moves on
// to be managed by the ConnectedSocket.  If a connection could not be
// established, it calls the ErrorCallback.
//
// The ConnectedSocket is used to perform all communications through an
// established connection.  Users should subclass ConnectedSocket and override
// the HandleReceived, HandleSent, and HandleDisconnected methods to receive
// notifications for the corresponding events.
//
// An RPCSocket is used to perform a one-time RPC.  To perform an RPC, call
// RPCSocket::PerformRPC(..) with a target ip/port pair and a command.  If a
// connection could be made, the RPC is said to be complete when the other side
// disconnects.  At this point, RPCSocket invokes the DoneCallback, passing it
// the entire response received until the disconnection.

#ifndef TOOLS_TAGS_SOCKET_H__
#define TOOLS_TAGS_SOCKET_H__

#include <netinet/in.h>

#include "callback.h"
#include "pollable.h"
#include "strutil.h"

namespace gtags {

class ConnectedSocket;

// Provides a Close method for all Sockets.
// May provide more if necessary.
class Socket : public Pollable {
 public:
  typedef Callback2<ConnectedSocket*, int, PollServer*> ConnectedCallback;
  typedef Callback0<void> ErrorCallback;

  virtual ~Socket() {}
  virtual void Close();

 protected:
  Socket(int socket_fd, PollServer *pollserver) :
      Pollable(socket_fd, pollserver) {}

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Socket);
};

class ListenerSocket : public Socket {
 public:
  virtual ~ListenerSocket() {
    Close();
    delete connected_callback_;
  }

  // Creates a new ListenerSocket for the specified port and managed by the
  // specified PollServer.  The ConnectedCallback will be called whenever a
  // connection is accepted.  The ConnectedCallback will be deleted
  // automatically by the ListenerSocket.
  // If connected_callback is not repeatable or the socket could not be bound
  // to the specified port, deletes the callback and returns NULL.
  // Otherwise, returns the new ListenerSocket.
  static ListenerSocket* Create(
      int port, PollServer *pollserver,
      ConnectedCallback *connected_callback);

 protected:
  ListenerSocket(int socket_fd, PollServer *pollserver,
                 ConnectedCallback *connected_callback) :
      Socket(socket_fd, pollserver), connected_callback_(connected_callback) {}

  virtual void HandleRead();

  ConnectedCallback *connected_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ListenerSocket);
};

class ClientSocket : public Socket {
 public:
  virtual ~ClientSocket();

  // Creates a new ClientSocket for the specified address and port and managed
  // by the specified PollServer.  The ConnectedCallback will be called when the
  // connection is completed.  The ConnectedCallback will be deleted
  // automatically by the ClientSocket.
  // If connected callback is repeatable or error callback is specified and is
  // repeatable, deletes both callbacks and returns NULL.
  // Otherwise, if the address is named and can't be resolved, calls the error
  // callback (if it is set), deletes the connected callback, and returns NULL.
  // Otherwise, returns the new ClientSocket.
  static ClientSocket* Create(
      const char* address, int port, PollServer *pollserver,
      ConnectedCallback *connected_callback,
      ErrorCallback *error_callback = NULL);

 protected:
  ClientSocket(int socket_fd, PollServer *pollserver,
               const struct sockaddr_in &addr,
               ConnectedCallback *connected_callback,
               ErrorCallback *error_callback) :
      Socket(socket_fd, pollserver), addr_(addr), connected_(false),
      connected_callback_(connected_callback),
      error_callback_(error_callback) {}

  virtual void HandleWrite();

  struct sockaddr_in addr_;
  bool connected_;
  ConnectedCallback *connected_callback_;
  ErrorCallback *error_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ClientSocket);
};

class ConnectedSocket : public Socket {
 public:
  ConnectedSocket(int socket_fd, PollServer *pollserver)
      : Socket(socket_fd, pollserver) {}
  virtual ~ConnectedSocket() { Close(); }

 protected:
  virtual void HandleRead();
  virtual void HandleWrite();

  // Subclasses should override these methods to get notifications
  virtual bool HandleReceived() { return false; }
  virtual void HandleSent() {}
  virtual void HandleDisconnected() {}

  // Subclasses should use these buffers to send/receive data
  string inbuf_;
  string outbuf_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ConnectedSocket);
};

class RPCSocket : public ConnectedSocket {
  typedef Callback1<void, const string&> DoneCallback;

 public:
  virtual ~RPCSocket() { Close(); }

  // Performs a (non-blocking) RPC to the specified address:port.
  // Upon establishing a connection it sends the specified command and waits for
  // the call to complete (connection closed), at which point it runs the
  // DoneCallback with the response.  The DoneCallback will be deleted
  // automatically by the RPCSocket.
  // If done_callback is repeatable or error_callback is specified and is
  // repeatable, deletes both callbacks and returns NULL.
  // Otherwise, returns the new ClientSocket that will start the connection.
  // The returned ClientSocket can be ignored, it is just returned to simplify
  // testing.
  static ClientSocket* PerformRPC(
      const char* address, int port, PollServer *pollserver,
      const string &command, DoneCallback *done_callback,
      ErrorCallback *error_callback = NULL);

 protected:
  RPCSocket(
      int socket_fd, PollServer *pollserver,
      const string &command, DoneCallback *done_callback) :
      ConnectedSocket(socket_fd, pollserver), done_callback_(done_callback) {
    outbuf_.append(command);
  }

  virtual void HandleDisconnected();

  static ConnectedSocket* Create(
      string command, DoneCallback *done_callback,
      ErrorCallback *error_callback,
      int fd, PollServer *pollserver);

  static void HandleError(
      DoneCallback *done_callback, ErrorCallback *error_callback);

  DoneCallback *done_callback_;
  ErrorCallback *error_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(RPCSocket);
};

}  // namespace gtags

#endif  // TOOLS_TAGS_SOCKET_H__
