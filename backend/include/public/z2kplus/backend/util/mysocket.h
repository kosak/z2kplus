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

#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/unix.h"

namespace z2kplus::backend::util {

class MySocket {
  typedef kosak::coding::FailFrame FailFrame;
public:
  static bool tryConnect(std::string_view server, int port, MySocket *result,
      const FailFrame &ff);
  static bool tryListen(int requestedPort, int *assignedPort, MySocket *result,
      const FailFrame &ff);

  static bool tryPipe2(int flags, MySocket *readPipe, MySocket *writePipe, const FailFrame &ff);
  static bool tryEpollCreate(MySocket *epollSocket, const FailFrame &ff);
  static bool trySocketpair(int domain, int type, int protocol, MySocket *socket0, MySocket *socket1,
      const FailFrame &ff);

  MySocket() = default;
  DISALLOW_COPY_AND_ASSIGN(MySocket);
  DECLARE_MOVE_COPY_AND_ASSIGN(MySocket);
  ~MySocket();

  bool tryAccept(MySocket *result, const FailFrame &ff);
  [[nodiscard]] int fd() const { return fd_; }
  void close();

private:
  explicit MySocket(int fd) : fd_(fd) {}

  int fd_ = -1;
};

}  // namespace z2kplus::backend::util
