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

#include "z2kplus/backend/reverse_index/builder/canonical_string_processor.h"

#include <tuple>

#include "kosak/coding/text/misc.h"
#include "kosak/coding/sorting/sort_manager.h"
#include "z2kplus/backend/reverse_index/builder/schemas.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/prefix_grabber.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/row_iterator.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/true_keeper.h"
#include "z2kplus/backend/shared/magic_constants.h"

using kosak::coding::Delegate;
using kosak::coding::FailFrame;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::memory::MappedFile;
using kosak::coding::nsunix::FileCloser;
using kosak::coding::text::Splitter;
using kosak::coding::sorting::KeyOptions;
using kosak::coding::sorting::SortManager;
using kosak::coding::sorting::SortOptions;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::util::frozen::FrozenStringPool;
using z2kplus::backend::util::frozen::FrozenVector;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeLastKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makePrefixGrabber;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeTrueKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::RowIterator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator;

#define HERE KOSAK_CODING_HERE

namespace nsunix = kosak::coding::nsunix;
namespace filenames = z2kplus::backend::shared::magicConstants::filenames;

namespace z2kplus::backend::reverse_index::builder {
namespace {
bool tryScanAllStrings(const PathMaster &pm, const LogSplitterResult &lsr,
    const ZgramDigestorResult &zgdr, std::string *canonicalStringsResult, const FailFrame &ff);
}  // namespace

bool CanonicalStringProcessor::tryMakeCanonicalStringPool(const PathMaster &pm, const LogSplitterResult &lsr,
    const ZgramDigestorResult &zgdr, SimpleAllocator *alloc, FrozenStringPool *stringPool,
    const FailFrame &ff) {
  std::string canonicalStringFile;
  if (!tryScanAllStrings(pm, lsr, zgdr, &canonicalStringFile, ff.nest(HERE))) {
    return false;
  }
  MappedFile<char> mf;
  if (!mf.tryMap(canonicalStringFile, false, ff.nest(HERE))) {
    return false;
  }
  std::string_view stringText(mf.get(), mf.byteSize());
  size_t numStrings = 0;
  size_t numChars = 0;
  {
    auto splitter = Splitter::ofRecords(stringText, defaultRecordSeparator);
    std::string_view temp;
    while (splitter.moveNext(&temp)) {
      ++numStrings;
      numChars += temp.size();
    }
  }
  uint32_t *endArrayStart;
  char *textStart;
  if (!alloc->tryAllocate(numStrings, &endArrayStart, ff.nest(HERE)) ||
      !alloc->tryAllocate(numChars, &textStart, ff.nest(HERE))) {
    return false;
  }
  FrozenVector<uint32_t> endOffsets(endArrayStart, 0);
  auto splitter = Splitter::ofRecords(stringText, defaultRecordSeparator);
  std::string_view temp;
  auto *textCurrent = textStart;
  while (splitter.moveNext(&temp)) {
    std::memcpy(textCurrent, temp.data(), temp.size());
    textCurrent += temp.size();
    endOffsets.push_back(textCurrent - textStart);
  }
  *stringPool = FrozenStringPool(textStart, std::move(endOffsets));
  return true;
}

namespace {
template<typename Tuple>
bool tryScanHelper(BufferedWriter *writer, TupleIterator<Tuple> *iter, const FailFrame &ff);

bool tryScanAllStrings(const PathMaster &pm, const LogSplitterResult &lsr,
    const ZgramDigestorResult &zgdr, std::string *canonicalStringsResult, const FailFrame &ff) {
  auto canonicalStrings = pm.getScratchPathFor(filenames::canonicalStrings);
  auto canonicalStringsBeforeSorting = canonicalStrings + filenames::beforeSortingSuffix;
  FileCloser fc;
  if (!nsunix::tryOpen(canonicalStringsBeforeSorting, O_CREAT | O_WRONLY | O_TRUNC, 0644, &fc,
      ff.nest(HERE))) {
    return false;
  }
  BufferedWriter writer(std::move(fc));

  MappedFile<char> reactionsFile, zgRevsFile, zmojisFile, plusPlusKeysFile;
  if (!reactionsFile.tryMap(lsr.reactionsByZgramId_, false, ff.nest(HERE)) ||
      !zgRevsFile.tryMap(lsr.zgramRevisions_, false, ff.nest(HERE)) ||
      !zmojisFile.tryMap(lsr.zmojis_, false, ff.nest(HERE)) ||
      !plusPlusKeysFile.tryMap(zgdr.plusPlusKeysName(), false, ff.nest(HERE))) {
    return false;
  }

  RowIterator<schemas::ReactionsByZgramId::tuple_t> reactionsAllIter(std::move(reactionsFile));
  // zgramId, reaction, creator, wantAdd
  auto reactionsLastIter = makeLastKeeper<schemas::ReactionsByZgramId::keySize>(&reactionsAllIter);
  // zgramId, reaction, creator, true
  auto rIter = makeTrueKeeper<schemas::ReactionsByZgramId::keySize>(&reactionsLastIter);

  RowIterator<schemas::ZgramRevisions::tuple_t> zgAllIter(std::move(zgRevsFile));

  RowIterator<schemas::ZmojisRevisions::tuple_t> zmojiAllIter(std::move(zmojisFile));
  auto zIter = makeLastKeeper<schemas::ZmojisRevisions::keySize>(&zmojiAllIter);

  RowIterator<schemas::PlusPlusKeys::tuple_t> pkIter(std::move(plusPlusKeysFile));

  if (!tryScanHelper(&writer, &rIter, ff.nest(HERE)) ||
      !tryScanHelper(&writer, &zgAllIter, ff.nest(HERE)) ||
      !tryScanHelper(&writer, &zIter, ff.nest(HERE)) ||
      !tryScanHelper(&writer, &pkIter, ff.nest(HERE)) ||
      !writer.tryClose(ff.nest(HERE))) {
    return false;
  }

  SortManager sm;
  SortOptions sortOptions(false, true, defaultFieldSeparator, true);
  std::vector<KeyOptions> keyOptions{{1, false}};
  if (!SortManager::tryCreate(sortOptions, keyOptions, {canonicalStringsBeforeSorting},
      canonicalStrings, &sm, ff.nest(HERE)) ||
      !sm.tryFinish(ff.nest(HERE))) {
    return false;
  }
  *canonicalStringsResult = std::move(canonicalStrings);
  return true;
}

template<size_t Index, typename Tuple>
bool tryScanItemRecurse(BufferedWriter *writer, const Tuple &tuple, const FailFrame &ff) {
  if constexpr (Index != std::tuple_size_v<Tuple>) {
    typedef std::tuple_element_t<Index, Tuple> element_t;
    typedef std::remove_cv_t<std::remove_reference_t<element_t>> stripped_t;
    if constexpr (std::is_same_v<std::string_view, stripped_t>) {
      const auto &item = std::get<Index>(tuple);
      auto *buffer = writer->getBuffer();
      buffer->append(item.data(), item.size());
      buffer->push_back(defaultRecordSeparator);
    }
    return tryScanItemRecurse<Index + 1>(writer, tuple, ff.nest(HERE));
  }
  return true;
}

template<typename Tuple>
bool tryScanHelper(BufferedWriter *writer, TupleIterator<Tuple> *iter, const FailFrame &ff) {
  std::optional<Tuple> item;
  while (true) {
    if (!iter->tryGetNext(&item, ff.nest(HERE))) {
      return false;
    }
    if (!item.has_value()) {
      return true;
    }
    if (!tryScanItemRecurse<0>(writer, *item, ff.nest(HERE)) ||
        !writer->tryMaybeFlush(false, ff.nest(HERE))) {
      return false;
    }
  }
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::builder
