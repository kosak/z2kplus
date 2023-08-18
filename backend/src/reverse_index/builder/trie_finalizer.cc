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

#include "z2kplus/backend/reverse_index/builder/trie_finalizer.h"

#include <charconv>
#include <chrono>
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/text/misc.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/reverse_index/builder/common.h"
#include "z2kplus/backend/reverse_index/builder/trie_builder.h"

using kosak::coding::FailFrame;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::ReusableString32;
using kosak::coding::text::Splitter;
using z2kplus::backend::reverse_index::builder::defaultFieldSeparator;
using z2kplus::backend::reverse_index::builder::defaultRecordSeparator;
using z2kplus::backend::reverse_index::builder::SimpleAllocator;
using z2kplus::backend::reverse_index::builder::TrieBuilderNode;
using z2kplus::backend::reverse_index::builder::wordOffSeparator;
using z2kplus::backend::reverse_index::trie::FrozenNode;

namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

void supercow_confirm_sorted_here();

namespace z2kplus::backend::reverse_index::builder {
namespace {
bool tryAppendWordOffs(wordOff_t wordOffBase, std::string_view src, std::vector<wordOff_t> *dest,
    const FailFrame &ff);
bool tryReadUint32(Splitter *splitter, uint32_t *dest, const FailFrame &ff);
}  // namespace

bool TrieFinalizer::tryMakeTrie(const std::string &trieEntries,
    const std::vector<wordOff_t> &wordOffs, SimpleAllocator *alloc, FrozenTrie *result,
    const FailFrame &ff) {
  MappedFile<char> mf;
  if (!mf.tryMap(trieEntries, false, ff.nest(HERE))) {
    return false;
  }
  std::string_view entryText(mf.get(), mf.byteSize());
  auto splitter = Splitter::ofRecords(entryText, defaultRecordSeparator);

  // The observation here is that if you populate a trie in lexicographic order, then every node
  // will always have at most one "active" child (children whose contents are changing), and
  // furthermore, once a node's parent moves on to its next child, that node and its children
  // can be "frozen" / committed, because they will never change again. Put another way, a node
  // can be frozen when its parent is frozen or when its parent moves on to the next child.
  TrieBuilderNode root;
  std::optional<std::string_view> prevKey;
  std::vector<wordOff_t> prevWords;
  ReusableString32 rs32;
  auto flushPrevState = [alloc, &root, &prevKey, &prevWords, &rs32](const FailFrame &ff) {
    return rs32.tryReset(*prevKey, ff.nest(HERE)) &&
        root.tryInsert(rs32.storage(), prevWords.data(), prevWords.size(), alloc,
            ff.nest(HERE));
  };
  auto splittyStart = std::chrono::system_clock::now();
  std::string_view record;
  while (splitter.moveNext(&record)) {
    auto recordSplitter = Splitter::of(record, defaultFieldSeparator);
    std::string_view keyText;
    uint32_t shard;
    std::string_view wordOffsText;
    if (!recordSplitter.tryMoveNext(&keyText, ff.nest(HERE)) ||
        !tryReadUint32(&recordSplitter, &shard, ff.nest(HERE)) ||
        !recordSplitter.tryMoveNext(&wordOffsText, ff.nest(HERE)) ||
        !recordSplitter.tryConfirmEmpty(ff.nest(HERE))) {
      return false;
    }

    if (wordOffsText.empty()) {
      return ff.fail(HERE, "Words field was empty?!");
    }

    if (prevKey.has_value() && keyText != *prevKey) {
      if (!flushPrevState(ff.nest(HERE))) {
        return false;
      }
      prevWords.clear();
    }
    prevKey = keyText;
    if (!tryAppendWordOffs(wordOffs[shard], wordOffsText, &prevWords, ff.nest(HERE))) {
      return false;
    }
  }
  auto splittyStop = std::chrono::system_clock::now();
  auto splittyDur = splittyStop - splittyStart;
  std::cerr << "splitty duration is "
      << std::chrono::duration_cast<std::chrono::seconds>(splittyDur).count()
      << '\n';

  // Flush the final entry if there is one.
  if (prevKey.has_value() && !flushPrevState(ff.nest(HERE))) {
    return false;
  }

  trie::FrozenNode *frozenRoot;
  if (!root.tryFreeze(alloc, &frozenRoot, ff.nest(HERE))) {
    return false;
  }
  *result = FrozenTrie(frozenRoot);
  return true;
}

namespace {
bool tryAppendWordOffs(wordOff_t wordOffBase, std::string_view src, std::vector<wordOff_t> *dest,
    const FailFrame &ff) {
  auto splitter = Splitter::of(src, wordOffSeparator);
  uint32_t size;
  if (!tryReadUint32(&splitter, &size, ff.nest(HERE))) {
    return false;
  }

  for (size_t i = 0; i < size; ++i) {
    uint32_t value;
    if (!tryReadUint32(&splitter, &value, ff.nest(HERE))) {
      return false;
    }
    auto newWordOff = wordOffBase.addRaw(value);
    if (!dest->empty() && newWordOff <= dest->back()) {
      return ff.failf(HERE, "Words out of order: %o then %o", dest->back(), newWordOff);
    }
    dest->emplace_back(newWordOff);
  }
  if (!splitter.empty()) {
    return ff.failf(HERE, R"(Trailing matter "%o")", *splitter.text());
  }
  return true;
}

bool tryReadUint32(Splitter *splitter, uint32_t *dest, const FailFrame &ff) {
  std::string_view text;
  if (!splitter->tryMoveNext(&text, ff.nest(HERE))) {
    return false;
  }

  auto [endp, ec] = std::from_chars(text.begin(), text.end(), *dest);
  // Probably don't need to check both conditions
  if (endp == text.begin() || ec != std::errc()) {
    return ff.failf(HERE, "Couldn't find an unsigned integer starting at %o", text);
  }
  if (endp != text.end()) {
    return ff.failf(HERE, "Trailing matter in %o", text);
  }
  return true;
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::builder

