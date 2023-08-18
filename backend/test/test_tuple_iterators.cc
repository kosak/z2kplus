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

#include <string>
#include "catch/catch.hpp"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/test/util/test_util.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/accumulator.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/prefix_grabber.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/running_sum.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/true_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/tuple_serializer.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::test {

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeAccumulator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeLastKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makePrefixGrabber;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeRunningSum;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeTrueKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer::tryAppendTuple;
using z2kplus::backend::reverse_index::builder::tuple_iterators::tupleSerializer::tryParseTuple;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::test::util::TestUtil;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;

namespace {
typedef std::tuple<uint32_t, std::string_view, uint32_t> myTuple_t;

template<typename Tuple>
class ListIterator final : public TupleIterator<Tuple> {
public:
  ListIterator(const Tuple *begin, const Tuple *end) : begin_(begin), current_(begin), end_(end) {}

  bool tryGetNext(std::optional<Tuple> *result, const FailFrame &ff) final {
    if (current_ == end_) {
      result->reset();
      return true;
    }
    *result = *current_++;
    return true;
  }

  void reset() final {
    current_ = begin_;
  }

private:
  const Tuple *begin_ = nullptr;
  const Tuple *current_ = nullptr;
  const Tuple *end_ = nullptr;
};

template<typename T>
ListIterator<T> makeListIterator(const T *begin, const T *end) {
  return {begin, end};
}
}  // namespace

namespace {
template<typename Accumulator, typename Tuple>
void expectOutput(Accumulator *accumulator, const Tuple *current, const Tuple *end) {
  FailRoot fr;
  while (true) {
    std::optional<Tuple> next;
    if (!accumulator->tryGetNext(&next, fr.nest(HERE))) {
      INFO(fr);
      REQUIRE(false);
    }

    if (!next.has_value()) {
      if (current == end) {
        return;
      }
      INFO("Accumulator is at end, but expected has at least one more value: " << *current);
      REQUIRE(false);
    }

    if (current == end) {
      INFO("Accumulator has at least one more value, but expected is exhausted: " << *next);
      REQUIRE(false);
    }

    CHECK(*next == *current);
    current++;
  }
}
}  // namespace

TEST_CASE("tuples: lastKeeper", "[tuples]") {
  myTuple_t data[] = {
      {1, "hello", 12},
      {1, "hello", 85},
      {7, "hello", 3},
      {7, "kosak", 3},
      {7, "kosak", 4},
      {9, "kosh",  104}
  };

  auto tuples = makeListIterator(data, data + STATIC_ARRAYSIZE(data));
  auto lk = makeLastKeeper<2>(&tuples);

  myTuple_t expected[] = {
      data[1], data[2], data[4], data[5]
  };
  expectOutput(&lk, expected, expected + STATIC_ARRAYSIZE(expected));
}

TEST_CASE("tuples: accumulate", "[tuples]") {
  myTuple_t data[] = {
      {1, "hello", 12},
      {1, "hello", 85},
      {1, "kosak", 3},
      {1, "kosak", 4},
      {7, "kosak", 5}
  };

  myTuple_t expected[] = {
      {1, "hello", 97},
      {1, "kosak", 7},
      {7, "kosak", 5}
  };

  auto tuples = makeListIterator(data, data + STATIC_ARRAYSIZE(data));
  auto acc = makeAccumulator<2>(&tuples);

  expectOutput(&acc, expected, expected + STATIC_ARRAYSIZE(expected));
}

TEST_CASE("tuples: prefixGrabber", "[tuples]") {
  myTuple_t data[] = {
      {1, "hello", 12},
      {1, "hello", 85},
      {1, "kosak", 3},
      {1, "kosak", 4},
      {7, "kosak", 5}
  };

  typedef std::tuple<uint32_t, std::string_view> myResult_t;

  myResult_t expected[] = {
      {1, "hello"},
      {1, "hello"},
      {1, "kosak"},
      {1, "kosak"},
      {7, "kosak"}
  };

  auto tuples = makeListIterator(data, data + STATIC_ARRAYSIZE(data));
  auto acc = makePrefixGrabber<2>(&tuples);

  expectOutput(&acc, expected, expected + STATIC_ARRAYSIZE(expected));
}

TEST_CASE("tuples: runningSum", "[tuples]") {
  myTuple_t data[] = {
      {1, "hello", 12},
      {1, "hello", 85},
      {1, "kosak", 3},
      {1, "kosak", 4},
      {1, "kosak", 5}
  };

  myTuple_t expected[] = {
      {1, "hello", 12},
      {1, "hello", 97},
      {1, "kosak", 3},
      {1, "kosak", 7},
      {1, "kosak", 12}
  };

  auto tuples = makeListIterator(data, data + STATIC_ARRAYSIZE(data));
  auto rs = makeRunningSum<2>(&tuples);

  expectOutput(&rs, expected, expected + STATIC_ARRAYSIZE(expected));
}

TEST_CASE("tuples: trueKeeper", "[tuples]") {
  typedef std::tuple<uint32_t, std::string_view, bool> boolTuple_t;

  boolTuple_t data[] = {
      {1, "hello", false},
      {1, "hello", true},
      {3, "kosak", true},
      {3, "kosak", false},
      {3, "kosak", false}
  };

  boolTuple_t expected[] = {
      {1, "hello", true},
      {3, "kosak", true}
  };

  auto tuples = makeListIterator(data, data + STATIC_ARRAYSIZE(data));
  auto rs = makeTrueKeeper<2>(&tuples);

  expectOutput(&rs, expected, expected + STATIC_ARRAYSIZE(expected));
}

TEST_CASE("tuples: serializer", "[tuples]") {
  FailRoot fr;
  typedef std::tuple<bool, uint32_t, uint64_t, std::string_view, ZgramId, FileKey> everything_t;
  ZgramId zgId(1234);
  FileKey fk;
  if (!FileKey::tryCreate(1999, 3, 1, 3, true, &fk, fr.nest(HERE))) {
    INFO(fr);
    REQUIRE(false);
  }
  everything_t src(true, 87, 1'234'567'890'123ULL, "kosak", zgId, fk);

  std::string text;
  if (!tryAppendTuple(src, '\t', &text, fr.nest(HERE))) {
    INFO(fr);
    REQUIRE(false);
  }

  const char *expected = "T\t87\t1234567890123\tkosak\t1234\t21700007";
  REQUIRE(expected == text);

  everything_t dest;
  if (!tryParseTuple(text, '\t', &dest, fr.nest(HERE))) {
    INFO(fr);
    REQUIRE(false);
  }

  REQUIRE(src == dest);
}
}  // namespace z2kplus::backend::test
