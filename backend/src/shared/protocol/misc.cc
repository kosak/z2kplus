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

#include "z2kplus/backend/shared/protocol/misc.h"

#include <iostream>
#include "kosak/coding/coding.h"
#include "kosak/coding/myjson.h"

using kosak::coding::streamf;

namespace z2kplus::backend::shared::protocol {
std::ostream &operator<<(std::ostream &s, const Estimate &o) {
  return streamf(s, "[%o, %o]", o.count_, o.exact_);
}

DEFINE_TYPICAL_JSON(Estimate, count_, exact_);

Estimates Estimates::create(size_t frontCount, size_t backCount, bool frontIsExact, bool backIsExact) {
  Estimate front(frontCount, frontIsExact);
  Estimate back(backCount, backIsExact);
  return {front, back};
}

Estimates::Estimates(const Estimate &front, const Estimate &back) :
    front_(front), back_(back) {}

bool operator==(const Estimates &lhs, const Estimates &rhs) {
  return lhs.front_ == rhs.front_ && lhs.back_ == rhs.back_;
}

std::ostream &operator<<(std::ostream &s, const Estimates &o) {
  return streamf(s, "[front=%o, back=%o]", o.front_, o.back_);
}

DEFINE_TYPICAL_JSON(Estimates, front_, back_);

Filter::Filter() = default;
Filter::Filter(
    std::optional<std::string> sender, std::optional<std::string> instanceExact,
    std::optional<std::string> instancePrefix, bool strong) :
    sender_(std::move(sender)), instanceExact_(std::move(instanceExact)),
    instancePrefix_(std::move(instancePrefix)), strong_(strong) {}
Filter::Filter(const Filter &other) = default;
Filter::Filter(Filter &&other) noexcept = default;
Filter &Filter::operator=(const Filter &other) = default;
Filter &Filter::operator=(Filter &&other) noexcept = default;
Filter::~Filter() = default;

std::ostream &operator<<(std::ostream &s, const Filter &o) {
  return streamf(s, "Filter(%o,%o,%o,%o)", o.sender_, o.instanceExact_, o.instancePrefix_, o.strong_);
}

DEFINE_TYPICAL_JSON(Filter, sender_, instanceExact_, instancePrefix_, strong_);
}  // namespace z2kplus::backend::shared::protocol
