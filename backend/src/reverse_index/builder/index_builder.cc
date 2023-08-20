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

#include "z2kplus/backend/reverse_index/builder/index_builder.h"

#include <experimental/array>
#include "kosak/coding/memory/buffered_writer.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/sorting/sort_manager.h"
#include "kosak/coding/text/misc.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/reverse_index/builder/canonical_string_processor.h"
#include "z2kplus/backend/reverse_index/builder/log_analyzer.h"
#include "z2kplus/backend/reverse_index/builder/log_splitter.h"
#include "z2kplus/backend/reverse_index/builder/metadata_builder.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/row_iterator.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"

using kosak::coding::bit_cast;
using kosak::coding::dump;
using kosak::coding::FailFrame;
using kosak::coding::Hexer;
using kosak::coding::MyOstringStream;
using kosak::coding::ParseContext;
using kosak::coding::streamf;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::ReusableString32;
using kosak::coding::text::Splitter;
using kosak::coding::nsunix::FileCloser;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::sorting::KeyOptions;
using kosak::coding::sorting::SortOptions;
using z2kplus::backend::files::DateAndPartKey;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::Location;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::queryparsing::WordSplitter;
using z2kplus::backend::reverse_index::builder::CanonicalStringProcessor;
using z2kplus::backend::reverse_index::builder::LogSplitter;
using z2kplus::backend::reverse_index::builder::LogSplitterResult;
using z2kplus::backend::reverse_index::builder::MetadataBuilder;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeLastKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::RowIterator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator;
using z2kplus::backend::reverse_index::builder::ZgramDigestor;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::metadata::FrozenMetadata;
using z2kplus::backend::reverse_index::trie::FrozenTrie;
using z2kplus::backend::reverse_index::trie::FrozenNode;
using z2kplus::backend::shared::magicConstants::plusPlusRegex;
using z2kplus::backend::shared::PlusPlusScanner;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::util::frozen::FrozenStringPool;
using z2kplus::backend::util::frozen::FrozenVector;

#define HERE KOSAK_CODING_HERE

namespace filenames = z2kplus::backend::shared::magicConstants::filenames;
namespace magicConstants = z2kplus::backend::shared::magicConstants;
namespace nsunix = kosak::coding::nsunix;
namespace userMetadata = z2kplus::backend::shared::userMetadata;
namespace zgMetadata = z2kplus::backend::shared::zgMetadata;

namespace z2kplus::backend::reverse_index::builder {

// 1. Make the "next" directory

// 2. Retire the expired zgrams. By policy we don't touch anything older than N days (say 30), so
//    A. For the files in the "current" directory older than N days, hard link them to next.
//    B. For the remainder, parse them once to obtain the set of expirations, and parse them again to
//       filter out expired items and references to expired items. This is kind of annoyingly
//       inefficient but, for the sake of the rest of the algorithm, it's easier to treat them like
//       they were never there.
// 3. Our next task is to feed the entire plaintext log history into the sorters. We have a
//    different sorter for each type, as well as the derived metadata types. We will (later)
//    have another sorter for the string pool.
// 4. Feed the plaintext into the various sorters.
//
// 3. Our first task is to build zgramInfo, wordInfo, and the canonical string pool.
//    We need to know what the "net" versions of the zgrams are, after modifications are applied
//    to them. We also need to collect all the strings appearing in all metadata.
// 4. Create all the sorters: byZgramId, by
// 4. Create a zgramSorter and a unique word sorter.
// 5. Feed zgrams and zgram-revising metadata to the zgramSorter
// 6. Feed all metadata to the unique word sorter.
//
//
//
// We describe these things in logical order, but actually we may do them in a different order.
// 3. Make a Zgram sorter. This will process Zephyrgrams and the various zephyrgram-modifying
//    metadatas: instanceRevisions, bodyRevisions, renderStyleRevisions, expirationRevisions,
//    bodyRevisionPurgeRequests.
// 4. Pull stuff out of the Zgram sorter in order to build your ZgramInfos and WordInfos
//    temporary file. When you are done. You can move these to the final result.
// 5. You are done with the Zgram sorter.
// 6. Make the various metadata sorters. You need one for each metadata dictionary:
//    reactions, reactionCounts, etc.
// 7. Make a stringPool sorter
// 8. Feed the corpus to all these sorters.
// 9. Read back from the sorters a first time in order to
//    A. Get a total count for the outermost key
//    B. Feed the string pool sorter
// 10. Read the string pool sorter in order to finalize the string pool to the result.
// 11. Now you are ready to commit the string pool to the result.
// 12. You are done with the string pool sorter.
// 13. Read each sorter a second time. At this point you have enough information to read a sorter,
//     and write its info to the result file.
bool IndexBuilder::tryClearScratchDirectory(const PathMaster &pm, const FailFrame &ff) {
  // Because I'm weirdly obsessed about heap allocation/deallocation.
  std::string storage;
  auto cb = [&storage](std::string_view fullPath, bool isDir, const FailFrame &ff) {
    storage = fullPath;
    return isDir || nsunix::tryUnlink(storage, ff.nest(HERE));
  };
  return nsunix::tryEnumerateFilesAndDirsRecursively(pm.scratchRoot(), &cb, ff.nest(HERE));
}

// Steps:
// 1. Concatenate all the plaintexts and sort by key to make plaintext.sorted
// 2. Scan this to make canonicalStringPool.unsorted
// 3. Sort to make
bool IndexBuilder::tryBuild(const PathMaster &pm,
    std::optional<FileKey> loggedBeginKey, std::optional<FileKey> loggedEndKey,
    std::optional<FileKey> unloggedBeginKey, std::optional<FileKey> unloggedEndKey,
    const FailFrame &ff) {
  LogAnalyzer lazr;
  LogSplitterResult lsr;
  if (!LogAnalyzer::tryAnalyze(pm, loggedBeginKey, loggedEndKey, unloggedBeginKey, unloggedEndKey,
      &lazr, ff.nest(HERE)) ||
      !LogSplitter::split(pm, lazr.includedKeys(), magicConstants::numIndexBuilderShards,
      &lsr, ff.nest(HERE))) {
    return false;
  }

  // The strategy here is to make a "very large" sparse file and mmap it.
  auto outputFileName = pm.getScratchIndexPath();
  MappedFile<char> outputFile;
  if (!nsunix::tryMakeFileOfSize(outputFileName, 0644, outputFileMaxSize, ff.nest(HERE)) ||
      !outputFile.tryMap(outputFileName, true, ff.nest(HERE))) {
    return false;
  }

  SimpleAllocator alloc(outputFile.get(), outputFile.byteSize(), 8);
  FrozenIndex *start;
  if (!alloc.tryAllocate(1, &start, ff.nest(HERE))) {
    return false;
  }
  auto tempFile = pm.getScratchPathFor(filenames::tempFileForTupleCounts);
  ZgramDigestorResult zgdr;
  FrozenStringPool stringPool;
  FrozenMetadata metadata;
  if (!ZgramDigestor::tryDigest(pm, lsr, &alloc, &zgdr, ff.nest(HERE)) ||
      !CanonicalStringProcessor::tryMakeCanonicalStringPool(pm, lsr, zgdr, &alloc, &stringPool,
          ff.nest(HERE)) ||
      !MetadataBuilder::tryMakeMetadata(lsr, zgdr, tempFile, stringPool, &alloc, &metadata,
          ff.nest(HERE))) {
    return false;
  }

  // Default to minimum key.
  DateAndPartKey endKeyToUse;
  if (!lazr.includedKeys().empty()) {
    auto dpkey = lazr.includedKeys().back().asDateAndPartKey();
    if (!dpkey.tryBump(&endKeyToUse, ff.nest(HERE))) {
      return false;
    }
  }

  new((void*)start) FrozenIndex(endKeyToUse,
      std::move(zgdr.zgramInfos()), std::move(zgdr.wordInfos()), std::move(zgdr.trie()),
      std::move(stringPool), std::move(metadata));
  auto outputSize = alloc.allocatedSize();
  if (!outputFile.tryUnmap(ff.nest(HERE)) ||
      !nsunix::tryTruncate(outputFileName, outputSize, ff.nest(HERE))) {
    return false;
  }
  return true;
}
}  // namespace z2kplus::backend::reverse_index::builder
