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

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/strongint.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/shared/zephyrgram.h"

namespace z2kplus::backend::reverse_index {
namespace internal {
struct ZgramOffTag {
  static constexpr const char text[] = "ZgramOff";
};
struct WordOffTag {
  static constexpr const char text[] = "WordOff";
};
}  // namespace internal
typedef kosak::coding::StrongInt<uint32_t, internal::ZgramOffTag> zgramOff_t;
typedef kosak::coding::StrongInt<uint32_t, internal::WordOffTag> wordOff_t;

// This class is blittable.
class ZgramInfo {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::LogLocation LogLocation;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
public:
  static bool tryCreate(uint64 timesecs, const LogLocation &location, wordOff_t startingWordOff,
      ZgramId zgramId, size_t senderWordLength, size_t signatureWordLength,
      size_t instanceWordLength, size_t bodyWordLength, ZgramInfo *result, const FailFrame &ff);

  ZgramInfo();

  size_t totalWordLength() const {
    return senderWordLength_ + signatureWordLength_ + instanceWordLength_ + bodyWordLength_;
  }

  uint64_t timesecs() const { return timesecs_; }
  const LogLocation &location() const { return location_; }
  wordOff_t startingWordOff() const { return startingWordOff_; }
  ZgramId zgramId() const { return zgramId_; }
  uint8_t senderWordLength() const { return senderWordLength_; }
  uint8_t signatureWordLength() const { return signatureWordLength_; }
  uint16_t instanceWordLength() const { return instanceWordLength_; }
  uint16_t bodyWordLength() const { return bodyWordLength_; }

  DEFINE_ALL_COMPARISON_OPERATORS(ZgramInfo);
  int compare(const ZgramInfo &other) const {
    return zgramId_.compare(other.zgramId_);
  }

private:
  ZgramInfo(uint64 timesecs, const LogLocation &location, wordOff_t startingWordOff, ZgramId zgramId,
      uint16_t senderWordLength, uint16_t signatureWordLength, uint16_t instanceWordLength,
      uint16_t bodyWordLength);

  // The timesecs field of this zgram
  uint64 timesecs_ = 0;
  // The location of the zgram
  LogLocation location_;
  // The ID.
  ZgramId zgramId_;

  // Starting wordIndex of this zgram. See explanation below.
  wordOff_t startingWordOff_;
  uint32_t padding_ = 0;
  // Length of sender field in words, where "word" is defined as in our documentation
  // (see dynamic_index.h)
  uint16_t senderWordLength_ = 0;
  // Length of signature field in words. For example "Corey Kosak" is two words.
  uint16_t signatureWordLength_ = 0;
  // Length of instance field in words.
  uint16_t instanceWordLength_ = 0;
  // Length of body field in words.
  uint16_t bodyWordLength_ = 0;

  friend std::ostream &operator<<(std::ostream &s, const ZgramInfo &zg);
};
static_assert(std::is_trivially_copyable_v<ZgramInfo> &&
  std::has_unique_object_representations_v<ZgramInfo>);


// This class is POD. This structure forms the entries of the "word index"---the reverse index of
// word numbers to zephyrgram numbers.
class WordInfo {
  typedef kosak::coding::FailFrame FailFrame;

public:
  static bool tryCreate(zgramOff_t zgramOff, FieldTag fieldTag, WordInfo *result, const FailFrame &ff);

  zgramOff_t zgramOff() const { return zgramOff_t(zgramOff_); }

  FieldTag fieldTag() const { return (FieldTag)fieldTag_; }

  int compare(const WordInfo &other) const;
  DEFINE_ALL_COMPARISON_OPERATORS(WordInfo);

private:
  constexpr WordInfo(zgramOff_t zgramOff, FieldTag fieldTag) :
      zgramOff_(zgramOff.raw()), fieldTag_(static_cast<uint32_t>(fieldTag)) {}

  static_assert((int)FieldTag::numTags <= 8);
  static_assert(sizeof(zgramOff_t) == sizeof(uint32_t));

  // static constexpr size_t maxZgramOff = ((size_t)1 << 29U) - 1;

  // The zgram offset
  uint32_t zgramOff_: 29;
  // ...and the field of that zgram that the word appears in.
  uint32_t fieldTag_: 3;

  friend std::ostream &operator<<(std::ostream &s, const WordInfo &zg);
};
static_assert(std::is_trivially_copyable_v<WordInfo> &&
    std::has_unique_object_representations_v<WordInfo>);

}  // namespace z2kplus::backend::reverse_index
