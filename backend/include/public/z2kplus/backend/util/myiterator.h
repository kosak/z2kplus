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

#if 0
#pragma once
#include "kosak/coding/coding.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace z2kplus::backend::util {

// This is a common base type which allows me to pass iterators of somewhat diverse types (yet of
// course all yielding the same ItemType) to the Merger.
template<typename ItemType>
class MyIteratorBase {
public:
  virtual ~MyIteratorBase() = default;
  typedef ItemType item_type;
  virtual bool tryGetNext(ItemType *result) = 0;
};

// This wrapper class allows for value-style copying of MyIterators.
template<typename ItemType>
class MyIteratorWrapper final : public MyIteratorBase<ItemType> {
public:
  explicit MyIteratorWrapper(std::unique_ptr<MyIteratorBase<ItemType>> impl) :
    impl_(std::move(impl)) {}
  bool tryGetNext(ItemType *result) final {
    return impl_->tryGetNext(result);
  }

private:
  std::unique_ptr<MyIteratorBase<ItemType>> impl_;
};

template<typename MyIterator>
MyIteratorWrapper<typename MyIterator::item_type> wrapIterator(MyIterator it) {
  auto unique = std::make_unique<MyIterator>(std::move(it));
  auto result = MyIteratorWrapper<typename MyIterator::item_type>(std::move(unique));
  return std::move(result);
}

}  // namespace z2kplus::backend::util
#endif
