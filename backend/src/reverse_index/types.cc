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

#include "z2kplus/backend/reverse_index/types.h"

#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/comparers.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FailFrame;
using kosak::coding::streamf;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index {

bool ZgramInfo::tryCreate(uint64_t timesecs, const LogLocation &location, wordOff_t startingWordOff,
    ZgramId zgramId, size_t senderWordLength, size_t signatureWordLength,
    size_t instanceWordLength, size_t bodyWordLength, ZgramInfo *result, const FailFrame &ff) {
  *result = ZgramInfo(timesecs, location, startingWordOff, zgramId, senderWordLength,
      signatureWordLength, instanceWordLength, bodyWordLength);
  if (senderWordLength != result->senderWordLength() ||
      signatureWordLength != result->signatureWordLength() ||
      instanceWordLength != result->instanceWordLength() ||
      bodyWordLength != result->bodyWordLength()) {
    return ff.failf(HERE, "One of the fields overflowed: %o, %o, %o, %o",
        senderWordLength, signatureWordLength, instanceWordLength, bodyWordLength);
  }
  return true;
}

ZgramInfo::ZgramInfo() = default;

ZgramInfo::ZgramInfo(uint64 timesecs, const LogLocation &location, wordOff_t startingWordOff,
    ZgramId zgramId, uint16_t senderWordLength, uint16_t signatureWordLength,
    uint16_t instanceWordLength, uint16_t bodyWordLength) :
  timesecs_(timesecs), location_(location), zgramId_(zgramId),
  startingWordOff_(startingWordOff), senderWordLength_(senderWordLength),
  signatureWordLength_(signatureWordLength), instanceWordLength_(instanceWordLength),
  bodyWordLength_(bodyWordLength) {
}

std::ostream &operator<<(std::ostream &s, const ZgramInfo &zg) {
  return streamf(s,
    "(tsecs=%o, location=%o, wo=%o, zgId=%o, sLen=%o, sgLen=%o, iLen=%o, bLen=%o)",
    zg.timesecs_, zg.location_, zg.startingWordOff_, zg.zgramId_, (size_t)zg.senderWordLength_,
    (size_t)zg.signatureWordLength_, (size_t)zg.instanceWordLength_, (size_t)zg.bodyWordLength_);
}

bool WordInfo::tryCreate(zgramOff_t zgramOff, FieldTag fieldTag, WordInfo *result,
    const FailFrame &ff) {
  *result = WordInfo(zgramOff, fieldTag);
  if (zgramOff != result->zgramOff() || fieldTag != result->fieldTag()) {
    return ff.failf(HERE, "Some value was truncated: %o and %o", zgramOff, fieldTag);
  }
  return true;
}

int WordInfo::compare(const WordInfo &other) const {
  // pull out of bitfield
  const auto lzg = zgramOff_;
  const auto rzg = other.zgramOff_;
  int difference = kosak::coding::compare(&lzg, &rzg);
  if (difference != 0) {
    return difference;
  }
  const auto lft = fieldTag_;
  const auto rft = other.fieldTag_;
  return kosak::coding::compare(&lft, &rft);
}

std::ostream &operator<<(std::ostream &s, const WordInfo &zg) {
  return streamf(s, "[zg=%o/%o]", zg.zgramOff(), zg.fieldTag());
}

}  // namespace z2kplus::backend::reverse_index
