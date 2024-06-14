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

#include "z2kplus/backend/reverse_index/builder/zgram_digestor.h"

#include <charconv>
#include <thread>

#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/sorting/sort_manager.h"
#include "kosak/coding/memory/buffered_writer.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/reverse_index/builder/schemas.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/iterator_base.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/last_keeper.h"
#include "z2kplus/backend/reverse_index/builder/tuple_iterators/row_iterator.h"
#include "z2kplus/backend/reverse_index/builder/trie_finalizer.h"
#include "z2kplus/backend/reverse_index/trie/frozen_trie.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::makeReservedVector;
using kosak::coding::memory::BufferedWriter;
using kosak::coding::memory::MappedFile;
using kosak::coding::nsunix::FileCloser;
using kosak::coding::sorting::KeyOptions;
using kosak::coding::sorting::SortManager;
using kosak::coding::sorting::SortOptions;
using kosak::coding::streamf;
using kosak::coding::stringf;
using kosak::coding::toString;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::queryparsing::WordSplitter;
using z2kplus::backend::reverse_index::builder::tuple_iterators::makeLastKeeper;
using z2kplus::backend::reverse_index::builder::tuple_iterators::RowIterator;
using z2kplus::backend::reverse_index::builder::tuple_iterators::TupleIterator;
using z2kplus::backend::reverse_index::trie::FrozenTrie;
using z2kplus::backend::reverse_index::builder::TrieFinalizer;
using z2kplus::backend::shared::PlusPlusScanner;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::util::frozen::FrozenVector;

namespace filenames = z2kplus::backend::shared::magicConstants::filenames;
namespace schemas = z2kplus::backend::reverse_index::builder::schemas;
namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::builder {
namespace {
struct NameAndWriter {
  NameAndWriter();
  NameAndWriter(std::string outputName, BufferedWriter writer);
  DISALLOW_COPY_AND_ASSIGN(NameAndWriter);
  DECLARE_MOVE_COPY_AND_ASSIGN(NameAndWriter);
  ~NameAndWriter();

  std::string outputName_;
  BufferedWriter writer_;
};

struct TrieEntriesWriter {
  static constexpr const size_t flushThreshold = 16384;

  TrieEntriesWriter(uint32_t shard, NameAndWriter entries);
  DISALLOW_COPY_AND_ASSIGN(TrieEntriesWriter);
  DECLARE_MOVE_COPY_AND_ASSIGN(TrieEntriesWriter);
  ~TrieEntriesWriter();

  bool tryAdd(std::string_view key, wordOff_t wordOff, const FailFrame &ff);

  bool tryClose(const FailFrame &ff);

  bool tryFlush(const FailFrame &ff);

  uint32_t shard_ = 0;
  NameAndWriter entries_;
  // Collected Word Offsets that haven't been written out yet.
  std::map<std::string_view, std::vector<wordOff_t>> wordMap_;
  // Total number of words in the wordMap_ (used to periodically flush).
  size_t numWords_ = 0;
};

class DigesterThread {
public:
  static bool tryCreate(size_t shard, const PathMaster &pm, const LogSplitterResult &lsr,
      std::shared_ptr<DigesterThread> *result, const FailFrame &ff);

  DigesterThread(size_t shard, MappedFile<char> logged, MappedFile<char> unlogged,
      MappedFile<char> zgRevs, NameAndWriter zgInfos, NameAndWriter wordInfos,
      NameAndWriter plusPlusEntries, NameAndWriter minusMinusEntries,
      NameAndWriter plusPlusKeys, TrieEntriesWriter trieEntriesWriter);
  ~DigesterThread();
  bool tryFinish(const FailFrame &ff);

  static void run(std::shared_ptr<DigesterThread> self);
  bool tryRunHelper(const FailFrame &ff);

  static bool checkOrAdvance(const schemas::Zephyrgram &zgView,
      TupleIterator <schemas::ZgramRevisions::tuple_t> *iter,
      std::optional<schemas::ZgramRevisions::tuple_t> *item,
      std::string_view *instance, std::string_view *body, const FailFrame &ff);

  bool addZgramRow(const schemas::Zephyrgram &zgv, std::string_view instanceToUse,
      std::string_view bodyToUse, const FailFrame &ff);
  bool addPlusPlusesAndMinusMinuses(ZgramId zgramId, std::string_view bodyToUse, const FailFrame &ff);

  size_t shard_ = 0;

  MappedFile<char> logged_;
  MappedFile<char> unlogged_;
  MappedFile<char> zgRevs_;

  NameAndWriter zgInfos_;
  NameAndWriter wordInfos_;
  NameAndWriter plusPlusEntries_;
  NameAndWriter minusMinusEntries_;
  NameAndWriter plusPlusKeys_;
  TrieEntriesWriter trieEntriesWriter_;

  // Current zgram offset
  zgramOff_t zgramOff_;
  // Current word offset
  wordOff_t wordOff_;

  PlusPlusScanner plusPlusScanner_;

  std::optional<std::string> error_;
  std::thread thread_;
};

void appendUint32(std::string *dest, uint32_t value);
bool tryGatherZgramInfos(const std::vector<std::string> &zgInfoNames, SimpleAllocator *alloc,
    FrozenVector<ZgramInfo> *result, const FailFrame &ff);
bool tryGatherWordInfos(const std::vector<std::string> &wordInfoNames,
    const std::vector<size_t> &numZgramsPerShard, SimpleAllocator *alloc,
    FrozenVector<WordInfo> *result, std::vector<size_t> *numWordsPerShard, const FailFrame &ff);
bool tryGatherPlusPluses(const std::vector<std::string> &plusPlusEntriesNames,
    const std::string &plusPlusEntriesSorted, const FailFrame &ff);
bool tryGatherMinusMinuses(const std::vector<std::string> &plusPlusEntriesNames,
    const std::string &plusPlusEntriesSorted, const FailFrame &ff);
bool tryGatherPlusPlusKeys(const std::vector<std::string> &plusPlusKeysNames,
    const std::string &plusPlusKeysSorted, const FailFrame &ff);
bool tryGatherTrieEntries(const std::vector<std::string> &trieEntriesNames,
    const std::string &trieEntriesSorted, const FailFrame &ff);
}  // namespace

bool ZgramDigestor::tryDigest(const PathMaster &pm, const LogSplitterResult &lsr,
    SimpleAllocator *alloc, ZgramDigestorResult *result, const FailFrame &ff) {
  auto numShards = lsr.loggedZgrams_.size();
  passert(numShards == lsr.unloggedZgrams_.size());

  auto plusPlusEntriesName = pm.getScratchPathFor(filenames::plusPlusEntries);
  auto minusMinusEntriesName = pm.getScratchPathFor(filenames::minusMinusEntries);
  auto plusPlusKeysName = pm.getScratchPathFor(filenames::plusPlusKeys);
  auto trieEntries = pm.getScratchPathFor(filenames::trieEntries);

  std::vector<std::shared_ptr<DigesterThread>> digesters(numShards);
  for (size_t i = 0; i != numShards; ++i) {
    if (!DigesterThread::tryCreate(i, pm, lsr, &digesters[i], ff.nest(HERE))) {
      return false;
    }
  }

  for (size_t i = 0; i != numShards; ++i) {
    (void)digesters[i]->tryFinish(ff.nest(HERE));
  }
  if (!ff.ok()) {
    return false;
  }

  auto numZgramsPerShard = makeReservedVector<size_t>(digesters.size());
  for (const auto &digester : digesters) {
    numZgramsPerShard.push_back(digester->zgramOff_.raw());
  }

  auto zgInfoNames = makeReservedVector<std::string>(digesters.size());
  auto wordInfoNames = makeReservedVector<std::string>(digesters.size());
  auto plusPlusEntriesNames = makeReservedVector<std::string>(digesters.size());
  auto minusMinusEntriesNames = makeReservedVector<std::string>(digesters.size());
  auto plusPlusKeysNames = makeReservedVector<std::string>(digesters.size());
  auto trieEntriesNames = makeReservedVector<std::string>(digesters.size());
  for (const auto &digester : digesters) {
    zgInfoNames.push_back(digester->zgInfos_.outputName_);
    wordInfoNames.push_back(digester->wordInfos_.outputName_);
    plusPlusEntriesNames.push_back(digester->plusPlusEntries_.outputName_);
    minusMinusEntriesNames.push_back(digester->minusMinusEntries_.outputName_);
    plusPlusKeysNames.push_back(digester->plusPlusKeys_.outputName_);
    trieEntriesNames.push_back(digester->trieEntriesWriter_.entries_.outputName_);
  }

  FrozenVector<ZgramInfo> zgramInfos;
  FrozenVector<WordInfo> wordInfos;
  FrozenTrie trie;
  std::vector<size_t> numWordsPerShard;
  if (!tryGatherZgramInfos(zgInfoNames, alloc, &zgramInfos, ff.nest(HERE)) ||
      !tryGatherWordInfos(wordInfoNames, numZgramsPerShard, alloc, &wordInfos, &numWordsPerShard, ff.nest(HERE)) ||
      !tryGatherPlusPluses(plusPlusEntriesNames, plusPlusEntriesName, ff.nest(HERE)) ||
      !tryGatherMinusMinuses(minusMinusEntriesNames, minusMinusEntriesName, ff.nest(HERE)) ||
      !tryGatherPlusPlusKeys(plusPlusKeysNames, plusPlusKeysName, ff.nest(HERE)) ||
      !tryGatherTrieEntries(trieEntriesNames, trieEntries, ff.nest(HERE))) {
    return false;
  }
  auto wordOffs = makeReservedVector<wordOff_t>(numShards);
  wordOff_t nextWordOff(0);
  for (auto nw : numWordsPerShard) {
    wordOffs.push_back(nextWordOff);
    nextWordOff = nextWordOff.addRaw(nw);
  }
  if (!TrieFinalizer::tryMakeTrie(trieEntries, wordOffs, alloc, &trie, ff.nest(HERE))) {
    return false;
  }

  *result = ZgramDigestorResult(std::move(zgramInfos), std::move(wordInfos), std::move(trie),
      std::move(plusPlusEntriesName), std::move(minusMinusEntriesName), std::move(plusPlusKeysName));
  return true;
}

ZgramDigestorResult::ZgramDigestorResult() = default;
ZgramDigestorResult::ZgramDigestorResult(FrozenVector<ZgramInfo> zgramInfos,
    FrozenVector<WordInfo> wordInfos, FrozenTrie trie, std::string plusPlusEntriesName,
    std::string minusMinusEntriesName, std::string plusPlusKeysName) :
    zgramInfos_(std::move(zgramInfos)), wordInfos_(std::move(wordInfos)),
    trie_(std::move(trie)),plusPlusEntriesName_(std::move(plusPlusEntriesName)),
    minusMinusEntriesName_(std::move(minusMinusEntriesName)),
    plusPlusKeysName_(std::move(plusPlusKeysName)) {}
ZgramDigestorResult::ZgramDigestorResult(ZgramDigestorResult &&) noexcept = default;
ZgramDigestorResult &ZgramDigestorResult::operator=(ZgramDigestorResult &&) noexcept = default;
ZgramDigestorResult::~ZgramDigestorResult() = default;

namespace {
bool DigesterThread::tryCreate(size_t shard, const PathMaster &pm,
    const LogSplitterResult &lsr, std::shared_ptr<DigesterThread> *result, const FailFrame &ff) {
  // The logged zgrams for this shard
  MappedFile<char> logged;
  // The unlogged zgrams for this shard
  MappedFile<char> unlogged;
  // All the zgram revisions (we will skip over the ones we don't want)
  MappedFile<char> zgRevs;
  if (!logged.tryMap(lsr.loggedZgrams_[shard], false, ff.nest(HERE)) ||
      !unlogged.tryMap(lsr.unloggedZgrams_[shard], false, ff.nest(HERE)) ||
      !zgRevs.tryMap(lsr.zgramRevisions_, false, ff.nest(HERE))) {
    return false;
  }

  auto tryMakeNameAndWriter = [shard, &pm](const char *filename, NameAndWriter *result,
      const FailFrame &ff2) {
    auto name = stringf("%o%o.%o", pm.getScratchPathFor(filename), filenames::beforeSortingSuffix,
        shard);
    FileCloser fc;
    if (!nsunix::tryOpen(name, filenames::standardFlags, filenames::standardMode, &fc,
        ff2.nest(HERE))) {
      return false;
    }
    BufferedWriter bw(std::move(fc));
    *result = NameAndWriter(std::move(name), std::move(bw));
    return true;
  };

  NameAndWriter zgInfos;
  NameAndWriter wordInfos;
  NameAndWriter plusPlusEntries;
  NameAndWriter minusMinusEntries;
  NameAndWriter plusPlusKeys;
  NameAndWriter trieEntries;
  if (!tryMakeNameAndWriter(filenames::zgramInfos, &zgInfos, ff.nest(HERE)) ||
      !tryMakeNameAndWriter(filenames::wordInfos, &wordInfos, ff.nest(HERE)) ||
      !tryMakeNameAndWriter(filenames::plusPlusEntries, &plusPlusEntries, ff.nest(HERE)) ||
      !tryMakeNameAndWriter(filenames::minusMinusEntries, &minusMinusEntries, ff.nest(HERE)) ||
      !tryMakeNameAndWriter(filenames::plusPlusKeys, &plusPlusKeys, ff.nest(HERE)) ||
      !tryMakeNameAndWriter(filenames::trieEntries, &trieEntries, ff.nest(HERE))) {
    return false;
  }
  TrieEntriesWriter trieEntriesWriter(shard, std::move(trieEntries));

  auto res = std::make_shared<DigesterThread>(shard, std::move(logged), std::move(unlogged),
      std::move(zgRevs), std::move(zgInfos), std::move(wordInfos), std::move(plusPlusEntries),
      std::move(minusMinusEntries), std::move(plusPlusKeys), std::move(trieEntriesWriter));
  res->thread_ = std::thread(&run, res);
  *result = std::move(res);
  return true;
}

DigesterThread::DigesterThread(size_t shard, MappedFile<char> logged, MappedFile<char> unlogged,
    MappedFile<char> zgRevs, NameAndWriter zgInfos, NameAndWriter wordInfos,
    NameAndWriter plusPlusEntries, NameAndWriter minusMinusEntries, NameAndWriter plusPlusKeys,
    TrieEntriesWriter trieEntriesWriter) : shard_(shard),
    logged_(std::move(logged)), unlogged_(std::move(unlogged)),
    zgRevs_(std::move(zgRevs)), zgInfos_(std::move(zgInfos)), wordInfos_(std::move(wordInfos)),
    plusPlusEntries_(std::move(plusPlusEntries)), minusMinusEntries_(std::move(minusMinusEntries)),
    plusPlusKeys_(std::move(plusPlusKeys)), trieEntriesWriter_(std::move(trieEntriesWriter)) {}
DigesterThread::~DigesterThread() = default;

bool DigesterThread::tryFinish(const FailFrame &ff) {
  thread_.join();
  if (error_.has_value()) {
    return ff.failf(HERE, "%o", *error_);
  }
  return true;
}

void DigesterThread::run(std::shared_ptr<DigesterThread> self) {
  auto id = stringf("Digester-%o", self->shard_);
  streamf(std::cerr, "%o: waking up\n", id);

  FailRoot fr;
  if (!self->tryRunHelper(fr.nest(HERE))) {
    self->error_ = toString(fr);
    streamf(std::cerr, "%o: got an error: %o", id, *self->error_);
  }
  streamf(std::cerr, "%o: shutting down\n", id);
}

bool DigesterThread::tryRunHelper(const FailFrame &ff) {
  RowIterator<schemas::Zephyrgram::tuple_t> loggedIter(std::move(logged_));
  RowIterator<schemas::Zephyrgram::tuple_t> unloggedIter(std::move(unlogged_));
  RowIterator<schemas::ZgramRevisions::tuple_t> zgAllRevsIter(std::move(zgRevs_));

  auto zgIter = makeLastKeeper<schemas::ZgramRevisions::keySize>(&zgAllRevsIter);

  std::optional<schemas::Zephyrgram::tuple_t> thisLogged;
  std::optional<schemas::Zephyrgram::tuple_t> thisUnlogged;
  std::optional<schemas::ZgramRevisions::tuple_t> thisZgRev;

  if (!loggedIter.tryGetNext(&thisLogged, ff.nest(HERE)) ||
      !unloggedIter.tryGetNext(&thisUnlogged, ff.nest(HERE)) ||
      !zgIter.tryGetNext(&thisZgRev, ff.nest(HERE))) {
    return false;
  }

  // auto start = std::chrono::system_clock::now();
  while (thisLogged.has_value() || thisUnlogged.has_value()) {
    bool useLogged;
    if (!thisLogged.has_value()) {
      useLogged = false;
    } else if (!thisUnlogged.has_value()) {
      useLogged = true;
    } else {
      const auto &loggedZgramId = std::get<0>(*thisLogged);
      const auto &unloggedZgram = std::get<0>(*thisUnlogged);
      auto compare = loggedZgramId.compare(unloggedZgram);
      if (compare == 0) {
        return ff.failf(HERE, "Logged and unlogged have the same zgramId %o", loggedZgramId);
      }
      useLogged = compare < 0;
    }
    auto *whichIter = useLogged ? &loggedIter : &unloggedIter;
    auto &whichOptional = useLogged ? thisLogged : thisUnlogged;
    schemas::Zephyrgram whichView(*whichOptional);

    std::string_view instanceToUse;
    std::string_view bodyToUse;
    if (!checkOrAdvance(whichView, &zgIter, &thisZgRev, &instanceToUse, &bodyToUse,
        ff.nest(HERE)) ||
        !addZgramRow(whichView, instanceToUse, bodyToUse, ff.nest(HERE)) ||
        !addPlusPlusesAndMinusMinuses(whichView.zgramId(), bodyToUse, ff.nest(HERE)) ||
        !whichIter->tryGetNext(&whichOptional, ff.nest(HERE))) {
      return false;
    }
  }

  return zgInfos_.writer_.tryClose(ff.nest(HERE)) &&
      wordInfos_.writer_.tryClose(ff.nest(HERE)) &&
      plusPlusEntries_.writer_.tryClose(ff.nest(HERE)) &&
      minusMinusEntries_.writer_.tryClose(ff.nest(HERE)) &&
      plusPlusKeys_.writer_.tryClose(ff.nest(HERE)) &&
      trieEntriesWriter_.tryClose(ff.nest(HERE));
}

bool DigesterThread::checkOrAdvance(const schemas::Zephyrgram &zgView,
    TupleIterator <schemas::ZgramRevisions::tuple_t> *iter,
    std::optional<schemas::ZgramRevisions::tuple_t> *item,
    std::string_view *instance, std::string_view *body, const FailFrame &ff) {
  *instance = zgView.instance();
  *body = zgView.body();
  while (true) {
    if (!item->has_value()) {
      return true;
    }
    const auto &[itemId, itemInstance, itemBody, itemRenderStyle] = **item;
    auto cmp = itemId.compare(zgView.zgramId());
    if (cmp < 0) {
      if (!iter->tryGetNext(item, ff.nest(HERE))) {
        return false;
      }
      continue;
    }
    if (cmp > 0) {
      return true;
    }
    *instance = itemInstance;
    *body = itemBody;
    return iter->tryGetNext(item, ff.nest(HERE));
  }
}

bool DigesterThread::addZgramRow(const schemas::Zephyrgram &zgv, std::string_view instanceToUse,
    std::string_view bodyToUse, const FailFrame &ff) {
  static std::array<FieldTag, 4> fieldTags = {
      FieldTag::sender, FieldTag::signature, FieldTag::instance, FieldTag::body
  };
  std::array<std::string_view, 4> sources = {zgv.sender(), zgv.signature(),
      instanceToUse, bodyToUse};
  std::array<size_t, 4> numTokens = {};

  static_assert(fieldTags.size() == sources.size());
  static_assert(fieldTags.size() == numTokens.size());

  auto originalWordOff = wordOff_;
  // Collected WordInfos
  std::vector<WordInfo> wordInfos;
  // Reusable token buffer
  std::vector<std::string_view> tokens;
  // std::vector<std::string_view> tokens2;
  for (size_t i = 0; i < fieldTags.size(); ++i) {
    tokens.clear();
    // tokens2.clear();
    // wordSplitter_.split(sources[i], &tokens);
    WordSplitter::split(sources[i], &tokens);
//    if (tokens != tokens2) {
//      std::cerr << sources[i] << '\n';
//      std::cerr << "exp " << tokens << '\n';
//      std::cerr << "act " << tokens2 << '\n';
//      tokens2.clear();
//      WordSplitter2::split(sources[i], &tokens2);
//    }
    numTokens[i] = tokens.size();
    for (auto token : tokens) {
      WordInfo wordInfo;
      if (!WordInfo::tryCreate(zgramOff_, fieldTags[i], &wordInfo, ff.nest(HERE))) {
        return false;
      }
      wordInfos.emplace_back(wordInfo);
      if (!trieEntriesWriter_.tryAdd(token, wordOff_, ff.nest(HERE))) {
        return false;
      }
      ++wordOff_;
    }
  }

  if (!wordInfos_.writer_.tryWritePOD(wordInfos.data(), wordInfos.size(), ff.nest(HERE))) {
    return false;
  }

  ++zgramOff_;

  LogLocation location(zgv.fileKey(), zgv.offset(), zgv.size(), "approved");
  ZgramInfo zgInfo;
  return ZgramInfo::tryCreate(zgv.timesecs(), location, originalWordOff, zgv.zgramId(),
      numTokens[0], numTokens[1], numTokens[2], numTokens[3], &zgInfo, ff.nest(HERE)) &&
      zgInfos_.writer_.tryWritePOD(&zgInfo, 1, ff.nest(HERE));
}

bool DigesterThread::addPlusPlusesAndMinusMinuses(ZgramId zgramId, std::string_view bodyToUse,
    const FailFrame &ff) {
  PlusPlusScanner::ppDeltas_t netPlusPlusCounts;
  plusPlusScanner_.scan(bodyToUse, 1, &netPlusPlusCounts);

  std::string reusableBuffer;
  char digits[16];  // 16 is plenty
  auto [ptr, ec] = std::to_chars(digits, digits + STATIC_ARRAYSIZE(digits), zgramId.raw());
  if (ec != std::errc()) {
    return ff.failf(HERE, "std::to_chars failed");
  }
  std::string_view zgramIdText(digits, ptr - digits);

  auto addEntries = [zgramIdText, &reusableBuffer](BufferedWriter *which, const std::string &key,
      size_t count, const FailFrame &ff2) {
    // Repeat the entry 'count' times
    reusableBuffer.clear();
    for (size_t i = 0; i != count; ++i) {
      reusableBuffer.append(key);
      reusableBuffer.push_back(defaultFieldSeparator);
      reusableBuffer.append(zgramIdText);
      reusableBuffer.push_back(defaultRecordSeparator);
    }
    return which->tryWriteBytes(reusableBuffer.data(), reusableBuffer.size(), ff2.nest(HERE));
  };

  for (const auto &[key, delta] : netPlusPlusCounts) {
    if (delta > 0) {
      addEntries(&plusPlusEntries_.writer_, key, delta, ff.nest(HERE));
    } else if (delta < 0) {
      addEntries(&minusMinusEntries_.writer_, key, -delta, ff.nest(HERE));
    } else {
      // This is a bit of a hack. If delta == 0 we still want to to dependency tracking.
      // So we add a balanced entry to both sides. Technically we would only need to do
      // this if both plusPluses and minusMinuses are empty, but we don't bother with that
      // optimization
      addEntries(&plusPlusEntries_.writer_, key, 1, ff.nest(HERE));
      addEntries(&minusMinusEntries_.writer_, key, 1, ff.nest(HERE));
    }

    // Add to keys (even if count is 0)
    reusableBuffer.clear();
    reusableBuffer.append(zgramIdText);
    reusableBuffer.push_back(defaultFieldSeparator);
    reusableBuffer.append(key);
    reusableBuffer.push_back(defaultRecordSeparator);
    if (!plusPlusKeys_.writer_.tryWriteBytes(reusableBuffer.data(), reusableBuffer.size(), ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

TrieEntriesWriter::TrieEntriesWriter(uint32_t shard, NameAndWriter entries) :
    shard_(shard), entries_(std::move(entries)) {}
TrieEntriesWriter::TrieEntriesWriter(TrieEntriesWriter &&) noexcept = default;
// TrieEntriesWriter &TrieEntriesWriter::operator=(TrieEntriesWriter &&) noexcept = default;
TrieEntriesWriter::~TrieEntriesWriter() = default;

bool TrieEntriesWriter::tryAdd(std::string_view key, wordOff_t wordOff, const FailFrame &ff) {
  wordMap_[key].push_back(wordOff);
  ++numWords_;
  return numWords_ < flushThreshold ||
      tryFlush(ff.nest(HERE));
}

bool TrieEntriesWriter::tryClose(const FailFrame &ff) {
  return tryFlush(ff.nest(HERE)) &&
      entries_.writer_.tryClose(ff.nest(HERE));
}

bool TrieEntriesWriter::tryFlush(const FailFrame &ff) {
  std::string buffer;
  for (const auto &[key, wordOffs] : wordMap_) {
    // key fieldseparator shard fieldseparator wordoffs.size (';' wordoff)...
    buffer.append(key);
    buffer.push_back(defaultFieldSeparator);
    appendUint32(&buffer, shard_);
    buffer.push_back(defaultFieldSeparator);
    appendUint32(&buffer, wordOffs.size());
    for (auto wordOff : wordOffs) {
      buffer.push_back(wordOffSeparator);
      appendUint32(&buffer, wordOff.raw());
    }
    buffer.push_back(defaultRecordSeparator);
  }
  wordMap_.clear();
  numWords_ = 0;
  return entries_.writer_.tryWriteBytes(buffer.data(), buffer.size(), ff.nest(HERE));
}

NameAndWriter::NameAndWriter() = default;
NameAndWriter::NameAndWriter(std::string outputName, BufferedWriter writer) :
  outputName_(std::move(outputName)), writer_(std::move(writer)) {}
NameAndWriter::NameAndWriter(NameAndWriter &&) noexcept = default;
NameAndWriter &NameAndWriter::operator=(NameAndWriter &&) noexcept = default;
NameAndWriter::~NameAndWriter() = default;

void appendUint32(std::string *dest, uint32_t value) {
  // 16 is more than enough for uint32_t which needs at most 10.
  std::array<char, 16> buffer;
  auto [endp, ec] = std::to_chars(buffer.begin(), buffer.end(), value);
  dest->append(buffer.begin(), endp);
}

/**
 * Jam all the ZgramInfos together. Convert the internal wordOffs they refer to from relative
 * to absolute.
 */
bool tryGatherZgramInfos(const std::vector<std::string> &zgInfoNames, SimpleAllocator *alloc,
    FrozenVector<ZgramInfo> *result, const FailFrame &ff) {
  auto numShards = zgInfoNames.size();
  std::vector<MappedFile<ZgramInfo>> mfs(numShards);
  std::vector<size_t> numElements(numShards);
  size_t totalNumElements = 0;
  for (size_t shard = 0; shard != numShards; ++shard) {
    auto &mf = mfs[shard];
    if (!mf.tryMap(zgInfoNames[shard], false, ff.nest(HERE))) {
      return false;
    }
    auto thisNumElements = mf.byteSize() / sizeof(ZgramInfo);
    numElements[shard] = thisNumElements;
    totalNumElements += thisNumElements;
  }

  ZgramInfo *start;
  if (!alloc->tryAllocate(totalNumElements, &start, ff.nest(HERE))) {
    return false;
  }
  auto *dest = start;
  wordOff_t wordOff(0);
  for (size_t shard = 0; shard != numShards; ++shard) {
    const auto *src = mfs[shard].get();
    auto thisNumElements = numElements[shard];
    for (size_t i = 0; i != thisNumElements; ++i) {
      if (!ZgramInfo::tryCreate(src->timesecs(), src->location(), wordOff,
          src->zgramId(), src->senderWordLength(), src->signatureWordLength(), src->instanceWordLength(),
          src->bodyWordLength(), dest, ff.nest(HERE))) {
        return false;
      }
      wordOff = wordOff.addRaw(src->senderWordLength() + src->signatureWordLength() +
          src->instanceWordLength() + src->bodyWordLength());
      ++src;
      ++dest;
    }
  }

  for (size_t i = 1; i < totalNumElements; ++i) {
    const auto &prev = start[i - 1];
    const auto &self = start[i];
    if (self.zgramId() <= prev.zgramId()) {
      return ff.failf(HERE, "%o is out of order with respect to %o", self.zgramId(), prev.zgramId());
    }
  }
  *result = FrozenVector<ZgramInfo>(start, totalNumElements);
  return true;
}

bool tryGatherWordInfos(const std::vector<std::string> &wordInfoNames,
    const std::vector<size_t> &numZgramsPerShard, SimpleAllocator *alloc,
    FrozenVector<WordInfo> *result, std::vector<size_t> *numWordsPerShard,
    const FailFrame &ff) {
  auto numShards = wordInfoNames.size();
  std::vector<MappedFile<WordInfo>> mfs(numShards);
  std::vector<size_t> numElements(numShards);
  size_t totalNumElements = 0;
  for (size_t shard = 0; shard != numShards; ++shard) {
    auto &mf = mfs[shard];
    if (!mf.tryMap(wordInfoNames[shard], false, ff.nest(HERE))) {
      return false;
    }
    auto thisNumElements = mf.byteSize() / sizeof(WordInfo);
    numElements[shard] = thisNumElements;
    totalNumElements += thisNumElements;
  }

  WordInfo *start;
  if (!alloc->tryAllocate(totalNumElements, &start, ff.nest(HERE))) {
    return false;
  }
  auto *dest = start;
  zgramOff_t zgramOffset(0);

  // Jam all the WordInfos together. For this we need to know the number of zgrams in each shard.
  for (size_t shard = 0; shard != wordInfoNames.size(); ++shard) {
    const auto *src = mfs[shard].get();
    auto thisNumElements = numElements[shard];
    for (size_t i = 0; i != thisNumElements; ++i) {
      zgramOff_t newZgramOff(zgramOffset.raw() + src->zgramOff().raw());
      if (!WordInfo::tryCreate(newZgramOff, src->fieldTag(), dest, ff.nest(HERE))) {
        return false;
      }
      ++src;
      ++dest;
    }
    zgramOffset = zgramOffset.addRaw(numZgramsPerShard[shard]);
  }
  *result = FrozenVector<WordInfo>(start, totalNumElements);
  *numWordsPerShard = std::move(numElements);
  return true;
}

bool tryGatherPlusPluses(const std::vector<std::string> &plusPlusEntriesNames,
    const std::string &plusPlusEntriesSorted, const FailFrame &ff) {
  SortOptions sortOptions(false, false, defaultFieldSeparator, true);
  std::vector<KeyOptions> keyOptions{{1, false}, {2, true}};
  return SortManager::trySort(sortOptions, keyOptions, plusPlusEntriesNames, plusPlusEntriesSorted,
      ff.nest(HERE));
}

bool tryGatherMinusMinuses(const std::vector<std::string> &plusPlusEntriesNames,
    const std::string &plusPlusEntriesSorted, const FailFrame &ff) {
  return tryGatherPlusPluses(plusPlusEntriesNames, plusPlusEntriesSorted, ff.nest(HERE));
}

bool tryGatherPlusPlusKeys(const std::vector<std::string> &plusPlusKeysNames,
    const std::string &plusPlusKeysSorted, const FailFrame &ff) {
  SortOptions sortOptions(false, false, defaultFieldSeparator, true);
  std::vector<KeyOptions> keyOptions{{1, true}, {2, false}};
  return SortManager::trySort(sortOptions, keyOptions, plusPlusKeysNames, plusPlusKeysSorted,
      ff.nest(HERE));
}

bool tryGatherTrieEntries(const std::vector<std::string> &trieEntriesNames,
    const std::string &trieEntriesSorted, const FailFrame &ff) {
  SortOptions sortOptions(true, false, defaultFieldSeparator, true);
  std::vector<KeyOptions> keyOptions{{1, false}, {2, true}};
  return SortManager::trySort(sortOptions, keyOptions, trieEntriesNames, trieEntriesSorted,
      ff.nest(HERE));
}
}  // namespace
}  // namespace z2kplus::backend::reverse_index::builder
