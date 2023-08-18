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

#include "z2kplus/backend/communicator/session.h"

#include <chrono>
#include <mutex>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"

using kosak::coding::stringf;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::communicator {
namespace {
// It's ok to have one mutex for everybody because there isn't much contention for it.
std::mutex swapMutex_;
}  // namespace

std::shared_ptr<Session> Session::create(std::shared_ptr<Profile> profile, std::shared_ptr<Channel> channel) {
  static uint64_t sessionBase =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  static std::atomic<uint64_t> nextFreeSessionId;
  sessionId_t id(nextFreeSessionId++);
  // Not really a GUID qua GUID but good enough for now.
  auto guid = stringf("%o:%o", sessionBase, nextFreeSessionId);

  return std::make_shared<Session>(Private(), id, std::move(guid), std::move(profile), std::move(channel));
}

Session::Session(Private, sessionId_t id, std::string &&guid, std::shared_ptr<Profile> &&profile,
    std::shared_ptr<Channel> &&channel) : id_(id), guid_(std::move(guid)), profile_(std::move(profile)),
    channel_(std::move(channel)) {}
Session::~Session() = default;

std::shared_ptr<Channel> Session::swapChannel(std::shared_ptr<Channel> newChannel) {
  std::unique_lock guard(swapMutex_);
  channel_.swap(newChannel);
  return newChannel;  // no longer "newChannel"; after the swap, it's the old channel.
}
}  // namespace z2kplus::backend::communicator
