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

#include "z2kplus/backend/reverse_index/builder/metadata_builder.h"

#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/buffered_writer.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/reverse_index/builder/common.h"
#include "z2kplus/backend/reverse_index/builder/inflator.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/accumulator.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/counter.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/prefix_grabber.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/row_iterator.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/running_sum.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/string_freezer.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/true_keeper.h"
#include "z2kplus/backend/reverse_index/builder/schemas.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/frozen/frozen_map.h"
#include "z2kplus/backend/util/frozen/frozen_set.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"
#include "z2kplus/backend/util/frozen/frozen_vector.h"
#include "z2kplus/backend/util/relative.h"

using kosak::coding::FailFrame;
using kosak::coding::stringf;
using kosak::coding::memory::MappedFile;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::nsunix::FileCloser;
using z2kplus::backend::reverse_index::builder::inflator::tryInflate;
using z2kplus::backend::reverse_index::builder::tuple_iterators::Accumulator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeAccumulator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeCounter;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeLastKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makePrefixGrabber;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeStringFreezer;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeTrueKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeRunningSum;
using z2kplus::backend::reverse_index::builder::tuple_iterators::RowIterator;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::util::frozen::frozenStringRef_t;
using z2kplus::backend::util::frozen::FrozenMap;
using z2kplus::backend::util::frozen::FrozenSet;
using z2kplus::backend::util::frozen::FrozenStringPool;
using z2kplus::backend::util::frozen::FrozenVector;
using z2kplus::backend::util::RelativePtr;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;
namespace schemas = z2kplus::backend::reverse_index::builder::schemas;

namespace z2kplus::backend::reverse_index::builder {
namespace {
bool tryMakeReactions(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::reactions_t *result,
    const FailFrame &ff);
bool tryMakeReactionCounts(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::reactionCounts_t *result,
    const FailFrame &ff);
bool tryMakeZgramRevisions(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::zgramRevisions_t *result,
    const FailFrame &ff);
bool tryMakeZgramRefersTos(const std::string &tempName, const std::string &filename,
    SimpleAllocator *alloc, FrozenMetadata::zgramRefersTo_t *result, const FailFrame &ff);
bool tryMakeZmojis(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::zmojis_t *result,
    const FailFrame &ff);
bool tryMakePlusPluses(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::plusPluses_t *result,
    const FailFrame &ff);
bool tryMakeMinusMinuses(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::minusMinuses_t *result,
    const FailFrame &ff);
bool tryMakePlusPlusKeys(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::plusPlusKeys_t *result,
    const FailFrame &ff);
}  // namespace

bool MetadataBuilder::tryMakeMetadata(const LogSplitterResult &lsr, const ZgramDigestorResult &zgdr,
    const std::string &tempFile, const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata *result,
    const FailFrame &ff) {
  FrozenMetadata::reactions_t reactions;
  FrozenMetadata::reactionCounts_t reactionCounts;
  FrozenMetadata::zgramRevisions_t zgramRevisions;
  FrozenMetadata::zgramRefersTo_t zgramRefersTo;
  FrozenMetadata::zmojis_t zmojis;
  FrozenMetadata::plusPluses_t plusPluses;
  FrozenMetadata::minusMinuses_t minusMinuses;
  FrozenMetadata::plusPlusKeys_t plusPlusKeys;
  if (!tryMakeReactions(tempFile, lsr.reactionsByZgramId_, stringPool, alloc, &reactions, ff.nest(HERE)) ||
      !tryMakeReactionCounts(tempFile, lsr.reactionsByReaction_, stringPool, alloc, &reactionCounts, ff.nest(HERE)) ||
      !tryMakeZgramRevisions(tempFile, lsr.zgramRevisions_, stringPool, alloc, &zgramRevisions, ff.nest(HERE)) ||
      !tryMakeZgramRefersTos(tempFile, lsr.zgramRefersTo_, alloc, &zgramRefersTo, ff.nest(HERE)) ||
      !tryMakeZmojis(tempFile, lsr.zmojis_, stringPool, alloc, &zmojis, ff.nest(HERE)) ||
      !tryMakePlusPluses(tempFile, zgdr.plusPlusEntriesName(), stringPool, alloc, &plusPluses,
          ff.nest(HERE))  ||
      !tryMakeMinusMinuses(tempFile, zgdr.minusMinusEntriesName(), stringPool, alloc, &minusMinuses,
          ff.nest(HERE)) ||
      !tryMakePlusPlusKeys(tempFile, zgdr.plusPlusKeysName(), stringPool, alloc, &plusPlusKeys,
          ff.nest(HERE))) {
    return false;
  }
  *result = FrozenMetadata(std::move(reactions), std::move(reactionCounts), std::move(zgramRevisions),
      std::move(zgramRefersTo), std::move(zmojis), std::move(plusPluses), std::move(minusMinuses),
      std::move(plusPlusKeys));
  return true;
}

namespace {
// TODO(kosak): put this somewhere
/**
 * Calculates the "tree depth" aka number of collections in a target data structure. This is used by the inflator
 * to turn a stream of tuples into a data structure.
 * For example, FrozenMap<frozenStringRef_t, FrozenMap<ZgramId, int64_t>> has two collections in it.
 * Whereas FrozenMap<frozenStringRef_t, FrozenMap<ZgramId, FrozenSet<int64_t>>> has three collections in it.
 * You can't get this answer just by looking at the tuple, because both of the above have the same tuple representation,
 * namely tuple<frozenStringRef_t, ZgramId, int64_t>
 */
template<typename T>
struct CalcTreeDepth {
  static constexpr size_t value = 0;
};

template<typename T>
struct CalcTreeDepth<FrozenVector<T>> {
static constexpr size_t value = 1 + CalcTreeDepth<T>::value;
};

template<typename T>
struct CalcTreeDepth<FrozenSet<T>> {
static constexpr size_t value = 1 + CalcTreeDepth<T>::value;
};

template<typename K, typename V>
struct CalcTreeDepth<FrozenMap<K,V>> {
  static constexpr size_t value = 1 + CalcTreeDepth<V>::value;
};

bool tryMakeReactions(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::reactions_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  typedef schemas::ReactionsByZgramId::tuple_t tuple_t;
  constexpr auto keySize = schemas::ReactionsByZgramId::keySize;

  RowIterator<tuple_t> iter(std::move(mf));  // zgramId, reaction, creator, wantAdd
  auto lastKeeper = makeLastKeeper<keySize>(&iter);
  auto trueKeeper = makeTrueKeeper<keySize>(&lastKeeper); // zgramId, reaction, creator, true
  auto reactions = makePrefixGrabber<keySize>(&trueKeeper);  // zgramId, reaction, creator
  auto frozen = makeStringFreezer(&reactions, &stringPool);
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::reactions_t>::value;
  static_assert(treeDepth == 3);
  auto res = tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE));
  return res;
}

bool tryMakeReactionCounts(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::reactionCounts_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::ReactionsByReaction::tuple_t> allIter(std::move(mf));  // reaction, zgramId, creator, wantAdd
  auto lastIter = makeLastKeeper<schemas::ReactionsByReaction::keySize>(&allIter);  // reaction, zgramId, creator, wantAdd
  auto trueIter = makeTrueKeeper<schemas::ReactionsByReaction::keySize>(&lastIter);  // reaction, zgramId, creator, true
  auto reactions = makePrefixGrabber<2>(&trueIter);  // reaction, zgramId
  auto counter = makeCounter<2>(&reactions);  // reaction, zgramId, count

  while (true) {
    std::optional<std::tuple<std::string_view, ZgramId, size_t>> item;
    if (!counter.tryGetNext(&item, ff.nest(HERE))) {
      return false;
    }
    if (!item.has_value()) {
      break;
    }
  }
  counter.reset();
  auto frozen = makeStringFreezer(&counter, &stringPool);
  while (true) {
    std::optional<std::tuple<frozenStringRef_t, ZgramId, size_t>> item;
    if (!frozen.tryGetNext(&item, ff.nest(HERE))) {
      return false;
    }
    if (!item.has_value()) {
      break;
    }
  }
  frozen.reset();
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::reactionCounts_t>::value;
  static_assert(treeDepth == 2);
  if (!tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE))) {
    return false;
  }
  return true;
}

bool tryMakeZgramRevisions(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::zgramRevisions_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::ZgramRevisions::tuple_t> iter(std::move(mf));
  auto frozen = makeStringFreezer(&iter, &stringPool);
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::zgramRevisions_t>::value;
  static_assert(treeDepth == 2);
  return tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE));
}

bool tryMakeZgramRefersTos(const std::string &tempName, const std::string &filename,
    SimpleAllocator *alloc, FrozenMetadata::zgramRefersTo_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::ZgramRefersTos::tuple_t> iter(std::move(mf));  // zgramId, refersTo, value
  constexpr auto keySize = schemas::ZgramRefersTos::keySize;
  auto lastKeeper = makeLastKeeper<keySize>(&iter);  // zgramId, refersTo, value
  auto trueKeeper = makeTrueKeeper<keySize>(&lastKeeper); // zgramId, refersTo, true
  auto refersTo = makePrefixGrabber<keySize>(&trueKeeper);  // zgramId, reaction

  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::zgramRefersTo_t>::value;
  static_assert(treeDepth == 2);
  return tryInflate(tempName, &refersTo, treeDepth, result, alloc, ff.nest(HERE));
}

bool tryMakeZmojis(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::zmojis_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::ZmojisRevisions::tuple_t> iter(std::move(mf));  // userid -> emojis
  auto lastKeeper = makeLastKeeper<schemas::ZmojisRevisions::keySize>(&iter);
  auto frozen = makeStringFreezer(&lastKeeper, &stringPool);
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::zmojis_t>::value;
  static_assert(treeDepth == 1);
  return tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE));
}

bool tryMakePlusPluses(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::plusPluses_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::PlusPluses::tuple_t> iter(std::move(mf));  // userid -> vector<ZgramId>
  auto frozen = makeStringFreezer(&iter, &stringPool);
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::plusPluses_t>::value;
  static_assert(treeDepth == 2);
  return tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE));
}

bool tryMakeMinusMinuses(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::minusMinuses_t *result,
    const FailFrame &ff) {
  return tryMakePlusPluses(tempName, filename, stringPool, alloc, result, ff.nest(HERE));
}

bool tryMakePlusPlusKeys(const std::string &tempName, const std::string &filename,
    const FrozenStringPool &stringPool, SimpleAllocator *alloc, FrozenMetadata::plusPlusKeys_t *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(filename, false, ff.nest(HERE))) {
    return false;
  }
  RowIterator<schemas::PlusPlusKeys::tuple_t> iter(std::move(mf));  // userid -> vector<ZgramId>
  auto frozen = makeStringFreezer(&iter, &stringPool);
  constexpr auto treeDepth = CalcTreeDepth<FrozenMetadata::plusPluses_t>::value;
  static_assert(treeDepth == 2);
  return tryInflate(tempName, &frozen, treeDepth, result, alloc, ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::builder
