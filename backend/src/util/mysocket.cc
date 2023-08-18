// Copyright 2023 The Z2K Plus+ Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "z2kplus/backend/util/mysocket.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <unistd.h>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/unix.h"

namespace nsunix = kosak::coding::nsunix;
using kosak::coding::atScopeExit;
using kosak::coding::FailFrame;
using kosak::coding::nsunix::FileCloser;
using kosak::coding::nsunix::tryPipe2;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::util {

bool MySocket::tryConnect(std::string_view server, int port, MySocket *result,
    const FailFrame &ff) {
  auto ss = server.size();
  char serverAsCString[ss + 1];
  memcpy(serverAsCString, server.data(), ss);
  serverAsCString[ss] = 0;

  char portAsString[16];
  snprintf(portAsString, STATIC_ARRAYSIZE(portAsString), "%d", port);
  struct addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  struct addrinfo *addrResult;

  if (getaddrinfo(serverAsCString, portAsString, &hints, &addrResult) != 0) {
    return ff.failf(HERE, "getaddrinfo() failed, errno=%o", errno);
  }

  // Try them all until one works.
  int fd = -1;
  auto ai = addrResult;
  for ( ; ai != nullptr; ai = ai->ai_next) {
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) {
      continue;
    }
    if (connect(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
      ::close(fd);
      continue;
    }
    break;
  }
  freeaddrinfo(addrResult);
  if (ai == nullptr) {
    return ff.failf(HERE, "Couldn't connect to %o:%o", server, port);
  }
  *result = MySocket(fd);
  return true;
}

bool MySocket::tryListen(int requestedPort, int *assignedPort, MySocket *result,
  const FailFrame &ff) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    return ff.failf(HERE, "socket() failed. errno is %o", errno);
  }

  {
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
      return ff.failf(HERE, "setsockopt() failed. errno is %o", errno);
    }
  }

  // Now that we have a valid fd, we grant its ownership to MySocket so it can take care of cleanup
  // even if we return an error.
  *result = MySocket(fd);

  struct sockaddr_in addr = {};  //zero it
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((uint16_t)requestedPort);
  if (bind(fd, kosak::coding::bit_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
    return ff.failf(HERE, "bind() failed. errno is %o", errno);
  }

  socklen_t addrLen = sizeof(addr);
  if (getsockname(fd, kosak::coding::bit_cast<struct sockaddr *>(&addr), &addrLen) != 0) {
    return ff.failf(HERE, "getsockname() failed. errno is %o", errno);
  }
  if (addrLen != sizeof(addr)) {
    return ff.failf(HERE, "addrLen changed on me. From %o to %o", sizeof(addr), addrLen);
  }
  *assignedPort = ntohs(addr.sin_port);

  if (listen(fd, 5) != 0) {
    return ff.failf(HERE, "listen() failed. errno is %o", errno);
  }
  return true;
}

bool MySocket::tryEpollCreate(MySocket *epollSocket, const FailFrame &ff) {
  auto epollFd = epoll_create(1);  // The 1 is ignored, but must not be zero.
  if (epollFd < 0) {
    return ff.failf(HERE, "epoll_create() failed, errno=%o", errno);
  }
  *epollSocket = MySocket(epollFd);
  return true;
}

bool MySocket::tryPipe2(int flags, MySocket *readPipe, MySocket *writePipe, const FailFrame &ff) {
  FileCloser fd0, fd1;
  if (!nsunix::tryPipe2(flags, &fd0, &fd1, ff.nest(HERE))) {
    return false;
  }
  *readPipe = MySocket(fd0.release());
  *writePipe = MySocket(fd1.release());
  return true;
}

bool MySocket::trySocketpair(int domain, int type, int protocol, MySocket *socket0,
    MySocket *socket1, const FailFrame &ff) {
  FileCloser fd0, fd1;
  if (!nsunix::trySocketpair(domain, type, protocol, &fd0, &fd1, ff.nest(HERE))) {
    return false;
  }
  *socket0 = MySocket(fd0.release());
  *socket1 = MySocket(fd1.release());
  return true;
}

MySocket::~MySocket() {
  close();
}

MySocket::MySocket(MySocket &&other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}

MySocket &MySocket::operator=(MySocket &&other) noexcept {
  close();
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}

bool MySocket::tryAccept(MySocket *result, const FailFrame &ff) {
  auto res = accept(fd_, nullptr, nullptr);
  if (res < 0) {
    return ff.failf(HERE, "accept() failed, errno=%o", res);
  }
  *result = MySocket(res);
  return true;
}

void MySocket::close() {
  if (fd_ < 0) {
    return;
  }
  ::close(fd_);
  fd_ = -1;
}

}  //  namespace z2kplus::backend::util
