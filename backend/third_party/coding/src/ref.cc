// Copyright 2023 Corey Kosak
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

#include "kosak/coding/ref.h"

namespace kosak::coding {
namespace internal {

RefAccounting::RefAccounting() = default;

RefAccounting::~RefAccounting() {
  std::unique_lock lock(mutex_);
  condVar_.wait(lock, [this] { return references_ == 0; });
}

void RefAccounting::ref() {
  std::unique_lock lock(mutex_);
  ++references_;
}

void RefAccounting::unref() {
  mutex_.lock();
  auto shouldNotify = --references_ == 0;
  mutex_.unlock();
  if (shouldNotify) {
    condVar_.notify_all();
  }
}

bool RefAccounting::isUnique() {
  std::unique_lock lock(mutex_);
  return references_ == 0;
}
}  // namespace internal
}  // namespace kosak::coding
