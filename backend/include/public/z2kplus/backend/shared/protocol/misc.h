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

#include <iostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/strongint.h"

namespace z2kplus::backend::shared::protocol {

class Estimate {
public:
  Estimate() = default;
  Estimate(size_t count, bool exact) : count_(count), exact_(exact) {}
  DEFINE_COPY_AND_ASSIGN(Estimate);
  DEFINE_MOVE_COPY_AND_ASSIGN(Estimate);
  ~Estimate() = default;

  bool count() const { return count_; }
  bool exact() const { return exact_; }

private:
  size_t count_ = 0;
  bool exact_ = false;

  friend bool operator==(const Estimate &lhs, const Estimate &rhs) {
    return lhs.count_ == rhs.count_ && lhs.exact_ == rhs.exact_;
  }

  friend bool operator!=(const Estimate &lhs, const Estimate &rhs) {
    return !(lhs == rhs);
  }
  friend std::ostream &operator<<(std::ostream &s, const Estimate &o);
  DECLARE_TYPICAL_JSON(Estimate);
};

class Estimates {
public:
  static Estimates create(uint64_t frontSize, uint64_t backSize, bool frontIsExact, bool backIsExact);

  Estimates() = default;
  Estimates(const Estimate &front, const Estimate &back);
  DEFINE_COPY_AND_ASSIGN(Estimates);
  DEFINE_MOVE_COPY_AND_ASSIGN(Estimates);
  ~Estimates() = default;

  const Estimate &front() const { return front_; }
  const Estimate &back() const { return back_; }

private:
  Estimate front_;
  Estimate back_;

  friend bool operator==(const Estimates &lhs, const Estimates &rhs);
  friend bool operator!=(const Estimates &lhs, const Estimates &rhs) {
    return !(lhs == rhs);
  }
  friend std::ostream &operator<<(std::ostream &s, const Estimates &o);
  DECLARE_TYPICAL_JSON(Estimates);
};
}  // namespace z2kplus::backend::shared::protocol
