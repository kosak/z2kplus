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
#include <cstddef>
#include <initializer_list>
#include "kosak/coding/coding.h"

namespace z2kplus::backend::util {

namespace internal {
template<bool Done, size_t Index, size_t Size>
struct PopulateTupleHelper {
  template<typename Tuple>
  static void doit(void **starts, Tuple *dest) {
    typedef typename std::tuple_element<Index, Tuple>::type elementType_t;
    std::get<Index>(*dest) = (elementType_t)starts[Index];
    PopulateTupleHelper<Index + 1 == Size, Index + 1, Size>::doit(starts, dest);
  }
};

template<size_t Index, size_t Size>
struct PopulateTupleHelper<true, Index, Size> {
  template<typename Tuple>
  static void doit(void **starts, Tuple *dest) {}
};

template<typename T>
struct GimmeSize_t {
  typedef size_t type;
};
};  // namespace internal

class SimpleAllocator {
public:
  explicit SimpleAllocator(size_t worstCaseAlignment) : worstCaseAlignment_(worstCaseAlignment) {}

  template<typename TStruct>
  TStruct *allocate(size_t numElements) {
    auto result = allocateMulti<TStruct>(numElements);
    return std::get<0>(result);
  }

  template<typename ...Types>
  std::tuple<Types*...> allocateMulti(typename internal::GimmeSize_t<Types>::type ...counts);

  void allocateMulti(const size_t *aggregateSizes, const size_t *alignments, size_t numItems,
    void **result);

protected:
  virtual void allocateImpl(const size_t *aggregateSizes, const size_t *alignments, size_t count,
    void **results) = 0;

  size_t worstCaseAlignment_ = 0;
};

template<typename ...Types>
std::tuple<Types*...>  SimpleAllocator::allocateMulti(
  typename internal::GimmeSize_t<Types>::type ...counts) {
  constexpr size_t numItems = sizeof...(Types);
  static size_t sizes[] = {sizeof(Types)...};
  static size_t alignments[] = {alignof(Types)...};
  size_t countsArray[] = { counts... };

  size_t aggregateSizes[numItems];
  for (size_t i = 0; i < numItems; ++i) {
    aggregateSizes[i] = sizes[i] * countsArray[i];
  }
  void *starts[numItems];
  allocateMulti(aggregateSizes, alignments, numItems, starts);
  std::tuple<Types*...> result;
  internal::PopulateTupleHelper<0 == numItems, 0, numItems>::doit(starts, &result);
  return result;
}

class BufferPopulator : public SimpleAllocator {
public:
  // Use buffer == nullptr and capacity == 0 if you are just measuring.
  BufferPopulator(size_t worstCaseAlignment, void *buffer, size_t capacity);

  const size_t size() const { return offset_; }

protected:
  void allocateImpl(const size_t *sizes, const size_t *alignments, size_t count,
    void **results) final;

private:
  void *buffer_;
  size_t offset_;
  size_t capacity_;
};

}  // namespace z2kplus::backend::util
#endif
