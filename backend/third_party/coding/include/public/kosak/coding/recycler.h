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

#pragma once

#include <vector>
#include "kosak/coding/coding.h"

namespace kosak::coding {

template<typename T>
class Recycler {
public:
  T borrow();
  void returnItem(T &&item);

private:
  std::vector<T> items_;
};

template<typename T>
T Recycler<T>::borrow() {
  if (items_.empty()) {
    return T();
  }

  auto result = std::move(items_.back());
  items_.pop_back();
  return result;
}

template<typename T>
void Recycler<T>::returnItem(T &&item) {
  items_.push_back(std::move(item));
}

}  // namespace kosak::coding
