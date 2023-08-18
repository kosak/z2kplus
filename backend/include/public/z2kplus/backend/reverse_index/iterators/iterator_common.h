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

#include "kosak/coding/coding.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/metadata/frozen_metadata.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::reverse_index::iterators {
namespace internal {
struct ZgramRelTag {
  static constexpr const char text[] = "ZgramRel";
};
struct WordRelTag {
  static constexpr const char text[] = "WordRel";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint32, internal::ZgramRelTag> zgramRel_t;
typedef kosak::coding::StrongInt<uint32, internal::WordRelTag> wordRel_t;

class IteratorUtils {
public:
  static bool MaskContains(FieldMask mask, FieldTag tag) {
    return ((size_t)mask & (size_t(1) << (size_t(tag)))) != 0;
  }
};

class IteratorContext {
protected:
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;

public:
  IteratorContext(const ConsolidatedIndex &ci, bool forward) : ci_(ci), forward_(forward) {}
  DISALLOW_COPY_AND_ASSIGN(IteratorContext);
  DISALLOW_MOVE_COPY_AND_ASSIGN(IteratorContext);
  ~IteratorContext() = default;

  static_assert(std::is_same_v<zgramOff_t::value_t, uint32_t>);
  static_assert(std::is_same_v<zgramRel_t::value_t, uint32_t>);

  zgramRel_t offToRel(zgramOff_t v) const {
    return zgramRel_t(maybeFlip(v.raw()));
  }

  wordRel_t offToRel(wordOff_t v) const {
    return wordRel_t(maybeFlip(v.raw()));
  }

  zgramOff_t relToOff(zgramRel_t v) const {
    return zgramOff_t(maybeFlip(v.raw()));
  }

  wordOff_t relToOff(wordRel_t v) const {
    return wordOff_t(maybeFlip(v.raw()));
  }

  std::pair<zgramRel_t, zgramRel_t> getIndexZgBoundsRel() const {
    return maybeFlipPair<zgramRel_t>(0, ci_.zgramInfoSize());
  }

  std::pair<wordRel_t, wordRel_t> getIndexWordBoundsRel() const {
    return maybeFlipPair<wordRel_t >(0, ci_.wordInfoSize());
  }

  std::pair<wordRel_t, wordRel_t> getWordBoundsRel(const ZgramInfo &zgInfo) const {
    auto begin = zgInfo.startingWordOff().raw();
    return maybeFlipPair<wordRel_t>(begin, begin + zgInfo.totalWordLength());
  }

  std::pair<wordRel_t, wordRel_t> getFieldBoundsRel(const ZgramInfo &zgInfo,
      FieldTag fieldTag) const;

  const ConsolidatedIndex &ci() const { return ci_; }
  bool forward() const { return forward_; }

protected:
  const ConsolidatedIndex &ci_;
  bool forward_ = false;

  uint32_t maybeFlip(uint32_t raw) const {
    return forward_ ? raw : std::numeric_limits<uint32_t>::max() - 1 - raw;
  }

  template<typename T>
  std::pair<T, T> maybeFlipPair(uint32_t begin, uint32_t end) const {
    static_assert(sizeof(T) == 4);
    if (!forward_) {
      auto temp = begin;
      begin = maybeFlip(end) + 1;
      end = maybeFlip(temp) + 1;
    }
    return std::make_pair(T(begin), T(end));
  }
};

class ZgramIteratorState {
public:
  ZgramIteratorState() = default;
  virtual ~ZgramIteratorState() = default;
  bool updateNextStart(const IteratorContext &ctx, zgramRel_t lowerBound, size_t capacity);

  zgramRel_t nextStart() const { return nextStart_; }

protected:
  zgramRel_t nextStart_;
};

class WordIteratorState {
public:
  WordIteratorState() = default;
  virtual ~WordIteratorState() = default;
  bool updateNextStart(const IteratorContext &ctx, wordRel_t lowerBound, size_t capacity);

  wordRel_t nextStart() const { return nextStart_; }

protected:
  wordRel_t nextStart_;
};

class ZgramIterator {
protected:
  typedef z2kplus::backend::reverse_index::metadata::FrozenMetadata FrozenMetadata;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;

public:
  ZgramIterator() = default;
  DISALLOW_COPY_AND_ASSIGN(ZgramIterator);
  DISALLOW_MOVE_COPY_AND_ASSIGN(ZgramIterator);
  virtual ~ZgramIterator();

  virtual std::unique_ptr<ZgramIteratorState> createState(const IteratorContext &ctx) const = 0;

  virtual size_t getMore(const IteratorContext &ctx, ZgramIteratorState *state,
      zgramRel_t lowerBound, zgramRel_t *result, size_t capacity) const = 0;

  // Methods that enable certain optimizations
  virtual bool tryReleaseAndChildren(std::vector<std::unique_ptr<ZgramIterator>> *result) {
    return false;
  }

  virtual bool tryReleaseOrChildren(std::vector<std::unique_ptr<ZgramIterator>> *result) {
    return false;
  }

  virtual bool tryNegate(std::unique_ptr<ZgramIterator> *result) {
    return false;
  }

  virtual bool matchesEverything() const {
    return false;
  }

  virtual bool matchesNothing() const {
    return false;
  }

protected:
  virtual void dump(std::ostream &s) const = 0;

  friend std::ostream &operator<<(std::ostream &s, const ZgramIterator &o) {
    o.dump(s);
    return s;
  }
};

class WordIterator {
protected:
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;

public:
  WordIterator() = default;
  DISALLOW_COPY_AND_ASSIGN(WordIterator);
  DISALLOW_MOVE_COPY_AND_ASSIGN(WordIterator);
  virtual ~WordIterator() = default;

  virtual std::unique_ptr<WordIteratorState> createState(const IteratorContext &ctx) const = 0;

  virtual size_t getMore(const IteratorContext &ctx, WordIteratorState *state, wordRel_t lowerBound,
      wordRel_t *result, size_t capacity) const = 0;

  // Methods that enable certain optimizations
  virtual bool matchesAnyWord(FieldMask *fieldMask) const { return false; }
  virtual bool tryGetAnchorChild(std::unique_ptr<WordIterator> *child, bool *anchoredLeft,
    bool *anchoredRight) { return false; }

protected:
  virtual void dump(std::ostream &s) const {}

  friend std::ostream &operator<<(std::ostream &s, const WordIterator &o) {
    o.dump(s);
    return s;
  }
};

class ZgramStreamer {
protected:
  static constexpr size_t bufferCapacity = 128;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;

public:
  ZgramStreamer();
  ZgramStreamer(const ZgramIterator *child, std::unique_ptr<ZgramIteratorState> &&childState);
  DISALLOW_COPY_AND_ASSIGN(ZgramStreamer);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramStreamer);
  ~ZgramStreamer();

  bool tryGetOrAdvance(const IteratorContext &ctx, zgramRel_t lowerBound, zgramRel_t *result);

private:
  const ZgramIterator *child_ = nullptr;
  std::unique_ptr<ZgramIteratorState> childState_;
  std::unique_ptr<zgramRel_t[]> data_;
  zgramRel_t *current_ = nullptr;
  zgramRel_t *end_ = nullptr;
};

class WordStreamer {
protected:
  static constexpr size_t bufferCapacity = 128;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;

public:
  WordStreamer();
  WordStreamer(const WordIterator *child, std::unique_ptr<WordIteratorState> &&childState);
  DISALLOW_COPY_AND_ASSIGN(WordStreamer);
  DECLARE_MOVE_COPY_AND_ASSIGN(WordStreamer);
  ~WordStreamer();

  bool tryGetOrAdvance(const IteratorContext &ctx, wordRel_t lowerBound, wordRel_t *result);

private:
  // Does not own.
  const WordIterator *child_ = nullptr;
  std::unique_ptr<WordIteratorState> childState_;
  std::unique_ptr<wordRel_t[]> data_;
  wordRel_t *current_ = nullptr;
  wordRel_t *end_ = nullptr;
};
}  // namespace z2kplus::backend::reverse_index::iterators
