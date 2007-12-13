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

#include "socket.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "pollserver.h"

namespace gtags {

const int kReadBufSize = 64;

#define ERROR_INFO "(" << errno << "=" << strerror(errno) << ")"

void Socket::Close() {
  if (fd_ != -1) {
    if (close(fd_) == -1)
      LOG(WARNING) << "Socket " << fd_ <<
          " failed to close " << ERROR_INFO;
    else
      LOG(INFO) << "Socket " << fd_ << " closed successfully.";

    // If the socket closed successfully, then this Socket is no longer attached
    // to an fd.
    // If the socket did not close successfully, then according to the close man
    // pages, it is because of:
    //   EBADF => a bad fd => socket already closed
    //   EINTR => close was interrupted => fd is in an undefined state
    //   ENOLINK => severed link => socket no longer connected
    // Either way, by this point the fd is guaranteed to be unusable so we
    // unregister ourselves and reset our fd.
    ps_->Unregister(this);
    fd_ = -1;
  }
}

ListenerSocket* ListenerSocket::Create(
    int port, PollServer *pollserver,
    ConnectedCallback *connected_callback) {
  // Check that it's a permanent callback.
  if (!connected_callback->IsRepeatable()) {
    delete connected_callback;
    return NULL;
  }

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  CHECK_NE(fd, -1) << "Could not acquire socket " << ERROR_INFO;

  fcntl(fd, F_SETFL, O_NONBLOCK);

  int one = 1;
  int result = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  if (result == -1) {
    LOG(WARNING) << "Unable to set socket option for address reuse "
		 << ERROR_INFO;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  int success = bind(fd, (struct sockaddr*)&addr,
                     static_cast<socklen_t>(sizeof(addr)));
  if (success == -1) {
    LOG(WARNING)  << "Bind on port " << port << " failed " << ERROR_INFO;
    close(fd);
    delete connected_callback;
    return NULL;
  }

  success = listen(fd, 3);
  CHECK_NE(success, -1) << "Listen failed " << ERROR_INFO;

  return new ListenerSocket(fd, pollserver, connected_callback);
}

void ListenerSocket::HandleRead() {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  int accepted_fd = accept(fd_, (struct sockaddr *)&addr, &addrlen);

  if (accepted_fd == -1 && errno != EWOULDBLOCK) {
    LOG(INFO) << "Unable to accept connection " << ERROR_INFO;
    return;
  }

  LOG(INFO) << "Connection accepted from " << inet_ntoa(addr.sin_addr);

  // Make sure the created socket is nonblocking.
  fcntl(accepted_fd, F_SETFL, O_NONBLOCK);

  connected_callback_->Run(accepted_fd, ps_);
}

ClientSocket::~ClientSocket() {
  if (connected_) {
    connected_callback_->Run(fd_, ps_);
    delete error_callback_;
  } else {
    Close();
    delete connected_callback_;
    if (error_callback_)
      error_callback_->Run();
  }
}

ClientSocket* ClientSocket::Create(
    const char* address, int port, PollServer *pollserver,
    ConnectedCallback *connected_callback, ErrorCallback *error_callback) {
  // The connected callback must be repeatable, and if set, the error callback
  // must be as well.
  if (connected_callback->IsRepeatable() ||
      (error_callback && error_callback->IsRepeatable())) {
    delete connected_callback;
    delete error_callback;
    return NULL;
  }

  // Resolve the address
  struct addrinfo *resolved_address;
  int error = getaddrinfo(address, NULL, NULL, &resolved_address);
  if (error) {
    LOG(INFO) << "Could not understand address: " << address;
    if (error_callback)
      error_callback->Run();
    delete connected_callback;
    // Do not free addrinfo here, it hasn't been allocated.
    return NULL;
  }

  LOG(INFO) << "Resolved address: " << address;

  struct sockaddr_in *addr = (struct sockaddr_in*)resolved_address->ai_addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  memset(&(addr->sin_zero), '\0', 8);  // zero the rest of the struct

  int fd = socket(PF_INET, SOCK_STREAM, 0);
  CHECK_NE(fd, -1);

  fcntl(fd, F_SETFL, O_NONBLOCK);

  ClientSocket *client = new ClientSocket(
      fd, pollserver, *addr, connected_callback, error_callback);

  freeaddrinfo(resolved_address);

  return client;
}

void ClientSocket::HandleWrite() {
  int success = connect(fd_, (struct sockaddr*)&addr_, sizeof(struct sockaddr));
  // Self-destruct if we know the final connection state.
  if (success == 0) {
    connected_ = true;
    delete this;
    LOG(INFO) << "Connection established; self-destructed!";
  } else if (errno != EINPROGRESS && errno != EALREADY) {
    delete this;
    LOG(INFO) << "Connect failed " << ERROR_INFO << "; self-destructed!";
  }
}

void ConnectedSocket::HandleRead() {
  char buf[kReadBufSize];

  int total_read = 0;
  int read = recv(fd_, &buf, kReadBufSize, 0);

  // Receive more data if there is any.
  while (read > 0) {
    total_read += read;
    inbuf_.append(buf, read);
    read = recv(fd_, &buf, kReadBufSize, 0);
  }

  // Print the received text.
  LOG(INFO) << total_read << " bytes read: "
            << inbuf_.substr(inbuf_.length() - total_read);

  // Check the value upon termination.
  if (read == 0) {
    LOG(INFO) << "Detected closed socket";
    ps_->Unregister(this);
    HandleDisconnected();
    return;
  } else if (read == -1 && errno != EWOULDBLOCK) {
    LOG(INFO) << "Error receiving " << ERROR_INFO;
    return;
  }

  // Clear the input buffer if we've finished with the current input.
  if (HandleReceived())
    inbuf_.clear();
}

void ConnectedSocket::HandleWrite() {
  int towrite = outbuf_.length();

  // Check if there's anything to send.
  if (towrite  == 0)
      return;

  int totalwrote = 0;
  const char* outbuf = outbuf_.c_str();
  while (towrite > 0) {
    int wrote = write(fd_, outbuf, towrite);
    if (wrote <= 0) {
      LOG(WARNING) << "Error sending " << ERROR_INFO;
      break;
    }
    LOG(INFO) << "Sent first " << wrote << " bytes of: " << outbuf_;
    outbuf += wrote;
    totalwrote += wrote;
    towrite -= wrote;
  }

  outbuf_.erase(0, totalwrote);

  if (towrite == 0)
    HandleSent();
}

ClientSocket* RPCSocket::PerformRPC(
    const char* address, int port, PollServer *pollserver,
    const string &command, DoneCallback *done_callback,
    ErrorCallback *error_callback) {
  if (done_callback->IsRepeatable()) {
    delete done_callback;
    return NULL;
  }
  return ClientSocket::Create(
      address, port, pollserver,
      CallbackFactory::Create(
          &Create, string(command), done_callback, error_callback),
      CallbackFactory::Create(
          &HandleError, done_callback, error_callback));
}

void RPCSocket::HandleDisconnected() {
  LOG(INFO) << "RPC completed with " << inbuf_;
  done_callback_->Run(inbuf_);
  delete this;
}

ConnectedSocket* RPCSocket::Create(
    string command, DoneCallback *done_callback,
    ErrorCallback *error_callback,
    int fd, PollServer *pollserver) {
  // delete the error_callback since it won't be needed
  delete error_callback;
  return new RPCSocket(fd, pollserver, command, done_callback);
}

void RPCSocket::HandleError(
    DoneCallback *done_callback, ErrorCallback *error_callback) {
  // delete the done_callback since it won't be needed
  delete done_callback;
  if (error_callback)
    error_callback->Run();
}

}  // namespace gtags
