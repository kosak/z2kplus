// Copyright 2023 Corey Kosak
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

#include "kosak/coding/myjson.h"

#include <cuchar>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

using kosak::coding::atScopeExit;
using kosak::coding::FailFrame;
using kosak::coding::Hexer;
using kosak::coding::Unit;
using kosak::coding::humanReadableTypeName;

#define HERE KOSAK_CODING_HERE

namespace kosak::coding {
bool KeyMaster::tryGetIndexFromKey(std::string_view key, size_t *index, const FailFrame &ff) const {
  for (size_t i = 0; i < numKeys_; ++i) {
    if (keys_[i] == key) {
      *index = i;
      return true;
    }
  }
  return ff.failf(HERE, "Key %o not known", key);
}

bool KeyMaster::tryGetKeyFromIndex(size_t index, std::string_view *key, const FailFrame &ff) const {
  if (index >= numKeys_) {
    return ff.failf(HERE, "Index %o >= size %o", index, numKeys_);
  }
  *key = keys_[index];
  return true;
}

ParseContext::ParseContext(std::string_view sr) : begin_(sr.begin()), end_(sr.end()),
  current_(sr.begin()) {}
ParseContext::~ParseContext() = default;

bool ParseContext::tryConsumeChar(char ch, const FailFrame &ff) {
  if (!maybeConsumeChar(ch)) {
    return noteContext(HERE, ff).failf(HERE, "Expected the character '%o'", ch);
  }
  return true;
}

bool ParseContext::maybeConsumeChar(char ch) {
  consumeWhitespace();
  if (current_ != end_ && *current_ == ch) {
    ++current_;
    return true;
  }
  return false;
}

bool ParseContext::tryFinishConsuming(char alreadyConsumed, std::string_view suffix,
    const FailFrame &ff) {
  auto buffer = asStringView().substr(0, suffix.size());
  if (buffer != suffix) {
    return noteContext(HERE, ff).failf(HERE, "Expected %o%o", alreadyConsumed, suffix);
  }
  current_ += suffix.size();
  return true;
}

void ParseContext::consumeWhitespace() {
  while (current_ != end_ && isspace(*current_)) {
    ++current_;
  }
}

internal::BorrowedStringHolder ParseContext::borrowBufferString() {
  std::string str;
  if (!recycleBin_.empty()) {
    str = std::move(recycleBin_.back());
    recycleBin_.pop_back();
  }
  return internal::BorrowedStringHolder(&recycleBin_, std::move(str));
}

internal::ContextFrame
ParseContext::noteContext(const char *func, const char *file, int line, const FailFrame &ff) const {
  const auto *pos = current_;
  const size_t margin = 100;
  passert(pos >= begin_ && pos <= end_);
  auto prefixSize = std::min<size_t>(pos - begin_, margin);
  auto suffixSize = std::min<size_t>(end_ - pos, margin);
  std::string_view surroundingContext(pos - prefixSize, prefixSize + suffixSize);
  return {ff.root(), &ff, func, file, line, surroundingContext, prefixSize};
}

namespace internal {
BorrowedStringHolder::BorrowedStringHolder(std::vector<std::string> *owner, std::string str) :
  owner_(owner), str_(std::move(str)) {
}

BorrowedStringHolder::~BorrowedStringHolder() {
  str_.clear();
  owner_->push_back(std::move(str_));
}

void ContextFrame::streamFrameTo(std::ostream &s) const {
  streamToHelper(s);
  std::string spaces(prefixSize_, ' ');
  streamf(s, " Context:\n%o\n%o^", context_, spaces);
}
}  // namespace internal

// JSON support for char
bool tryAppendJson(char c, std::string *result, const FailFrame &ff) {
  // Forward to the string routines, so they can handle it if 'c' is a special character or
  // not 7-bit ASCII.
  std::string_view temp(&c, 1);
  return tryAppendJson(temp, result, ff.nest(HERE));
}

bool tryParseJson(ParseContext *ctx, char *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);
  auto buffer = ctx->borrowBufferString();
  auto &str = buffer.str();
  if (!tryParseJson(ctx, &str, ff.nestWithType<char>(HERE))) {
    return false;
  }
  if (str.size() != 1) {
    return ff.failf(HERE, "String of length %o cannot fit in a char", str.size());
  }
  *result = str[0];
  return true;
}

// JSON support for bool

bool tryAppendJson(bool b, std::string *result, const FailFrame &/*ff*/) {
  const char *text = b ? "true" : "false";
  result->append(text);
  return true;
}

bool tryParseJson(ParseContext *ctx, bool *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);

  if (ctx->maybeConsumeChar('t')) {
    if (!ctx->tryFinishConsuming('t', "rue", ff.nest(HERE))) {
      return false;
    }
    *result = true;
    return true;
  }
  if (ctx->maybeConsumeChar('f')) {
    if (!ctx->tryFinishConsuming('f', "alse", ff.nest(HERE))) {
      return false;
    }
    *result = false;
    return true;
  }
  return ff.failf(HERE, R"(Expected "true" or "false" here)");
}

// JSON support for various integer types

namespace {

bool tryParseNumberHelper(ParseContext *ctx, unsigned long long min, unsigned long long max,
  unsigned long long *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);
  char *end;
  errno = 0;
  *result = strtoull(ctx->current(), &end, 10);
  if (errno != 0) {
    return ff.failf(HERE, "Error queryparsing number, errno = %o", errno);
  }
  if (ctx->current() == end) {
    return ff.fail(HERE, "Expected a number");
  }
  ctx->setCurrent(end);
  if (min != 0) {
    auto smin = bit_cast<long long>(min);
    auto smax = bit_cast<long long>(max);
    auto svalue = bit_cast<long long>(*result);
    if (svalue < smin || svalue > smax) {
      return ff.failf(KOSAK_CODING_HERE, "Signed value %o is out of range [%o..%o]", svalue, smin,
          smax);
    }
    return true;
  }
  if (*result > max) {
    return ff.failf(HERE, "Unsigned value %o is out of range [0..%o]", *result, max);
  }
  return true;
}

template<typename T>
bool tryParseInteger(ParseContext *ctx, T *result, const FailFrame &ff) {
  static_assert(std::is_integral<T>::value);
  auto minValue = std::numeric_limits<T>::min();
  auto maxValue = std::numeric_limits<T>::max();
  unsigned long long uvalue;
  if (!tryParseNumberHelper(ctx, minValue, maxValue, &uvalue, ff.nest(HERE))) {
    return false;
  }
  if (std::is_signed<T>::value) {
    auto svalue = bit_cast<long long>(uvalue);
    *result = (T)svalue;
    return true;
  }
  *result = (T)uvalue;
  return true;
}

}  // namespace

#define MAKE_JSON_SUPPORT(Type) \
bool tryParseJson(ParseContext *ctx, Type *result, const FailFrame &ff) { \
  return tryParseInteger(ctx, result, ff.nest(KOSAK_CODING_HERE)); \
}
MAKE_JSON_SUPPORT(signed char) MAKE_JSON_SUPPORT(unsigned char)
MAKE_JSON_SUPPORT(int16) MAKE_JSON_SUPPORT(uint16)
MAKE_JSON_SUPPORT(int32) MAKE_JSON_SUPPORT(uint32)
MAKE_JSON_SUPPORT(int64) MAKE_JSON_SUPPORT(uint64)
#undef MAKE_JSON_SUPPORT


// JSON support for double

namespace {
bool tryParseDoubleHelper(ParseContext *ctx, double *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);
  char *end;
  errno = 0;
  *result = strtod(ctx->current(), &end);
  if (errno != 0) {
    return ff.failf(HERE, "Error queryparsing double, errno = %o", errno);
  }
  if (ctx->current() == end) {
    return ff.failf(HERE, "Expected a double");
  }
  ctx->setCurrent(end);
  return true;
}
}  // namespace

bool tryParseJson(ParseContext *ctx, double *result, const FailFrame &ff) {
  return tryParseDoubleHelper(ctx, result, ff.nestWithType<double>(HERE));
}

namespace internal::enumHelpers {
namespace {
bool findIndexOfValue(const size_t *values, size_t numElements, size_t value, size_t *result,
    const FailFrame &ff) {
  auto ip = std::find(values, values + numElements, value);
  if (ip == values + numElements) {
    return ff.failf(HERE, "Invalid enum value: %o", value);
  }
  *result = ip - values;
  return true;
}
}  // namespace

bool tryAppendJson(const size_t *values, const char * const *tags, size_t numElements,
    size_t value, std::string *result, const FailFrame &ff) {
  size_t index;
  if (!findIndexOfValue(values, numElements, value, &index, ff.nest(HERE))) {
    return false;
  }
  std::string_view sv(tags[index]);
  return tryAppendJson(sv, result, ff.nest(HERE));
}

bool tryParseJson(const size_t *values, const char * const *tags, size_t numElements,
    ParseContext *ctx, size_t *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);
  auto bs = ctx->borrowBufferString();
  auto &str = bs.str();
  if (!tryParseJson(ctx, &str, ff.nest(HERE))) {
    return false;
  }
  auto ip = std::find(tags, tags + numElements, str);
  if (ip == tags + numElements) {
    return ff.failf(HERE, "Invalid enum tag: %o", str);
  }
  auto index = ip - tags;
  *result = values[index];
  return true;
}

std::ostream &stream(const size_t *values, const char *const *tags, size_t numElements,
    std::ostream &s, size_t item) {
  size_t index;
  FailRoot fr(true);
  if (!findIndexOfValue(values, numElements, item, &index, fr.nest(KOSAK_CODING_HERE))) {
    return streamf(s, "(invalid enum value %o)", item);
  }
  return s << tags[index];
}
}  // namespace internal::enumHelpers

// JSON support for string_view and string
namespace {
// Returns a value such that the half-open interval [startp, return_value) does not contain any
// control characters [0-31] or quote [34] or backslash [whatever] or DEL [127]. All other
// characters (including UTF-8) ok.
const char *nextContiguousPrintableSegment(const char *startp, const char *endp) {
  while (startp != endp) {
    auto ch = (unsigned char)*startp;
    if (ch < 32 || ch == '"' || ch == '\\' || ch == 127) {
      break;
    }
    ++startp;
  }
  return startp;
}

// Returns a value such that the half-open interval [startp, return_value) does not contain a \ or "
const char *nextUnterminatedUnescapedSegment(const char *startp, const char *endp) {
  while (startp != endp) {
    auto ch = *startp;
    if (ch == '\\' || ch == '"') {
      break;
    }
    ++startp;
  }
  return startp;
}

bool handleStringEscape(ParseContext *ctx, std::string *result, const FailFrame &ff_in) {
  auto ff = ctx->noteContext(HERE, ff_in);
  auto cp = ctx->current();
  passert(cp != ctx->end());
  ctx->setCurrent(cp + 1);
  switch (*cp) {
    case 'u': {
      cp = ctx->current();
      auto remaining = ctx->end() - cp;
      if (remaining < 4) {
        return ff.fail(HERE, "Not enough characters for Unicode escape");
      }
      char terminatedBuffer[5];
      memcpy(terminatedBuffer, cp, 4);
      terminatedBuffer[4] = 0;
      char *hexEndp;
      auto hexValue = strtoul(terminatedBuffer, &hexEndp, 16);
      if (hexEndp - terminatedBuffer != 4) {
        return ff.fail(HERE, "Unicode escape didn't have 4 valid hex characters");
      }
      char utf8Buffer[MB_CUR_MAX];
      std::mbstate_t state{};
      auto rc = (ssize_t)std::c32rtomb(utf8Buffer, (char32_t)hexValue, &state);
      if (rc < 0) {
        return ff.failf(HERE, "Error %o when converting %o to UTF-8", errno, Hexer(hexValue));
      }
      result->append(utf8Buffer, (std::string::size_type)rc);
      ctx->setCurrent(cp + 4);
      return true;
    };
    case 'b': { result->push_back('\b'); break; }
    case 'f': { result->push_back('\f'); break; }
    case 'n': { result->push_back('\n'); break; }
    case 'r': { result->push_back('\r'); break; }
    case 't': { result->push_back('\t'); break; }
    // default includes / " and "\"
    default: { result->push_back(*cp); break; }
  }
  return true;
}

}  // namespace

// JSON support for const char *
bool tryAppendJson(const char *value, std::string *result, const FailFrame &ff) {
  return tryAppendJson(std::string_view(value), result, ff.nest(HERE));
}

// JSON support for string_view
bool tryAppendJson(const std::string_view &value, std::string *result, const FailFrame &/*ff*/) {
  // Append non-"special" segments forever. When a non-"special" character or end of string is
  // encountered, either end or append the non-special character and continue.
  result->push_back('"');
  const char *current = &value[0];
  const char *end = current + value.size();
  while (true) {
    auto segmentEnd = nextContiguousPrintableSegment(current, end);
    result->append(current, segmentEnd);
    if (segmentEnd == end) {
      result->push_back('"');
      return true;
    }
    char ch = *segmentEnd;
    // 'ch' is a control character or quote or backslash or DEL.
    switch (ch) {
      case '\b': { result->append("\\b"); break; }
      case '\f': { result->append("\\f"); break; }
      case '\n': { result->append("\\n"); break; }
      case '\r': { result->append("\\r"); break; }
      case '\t': { result->append("\\t"); break; }
      case '"': { result->append("\\\""); break; }
      case '\\': { result->append("\\\\"); break; }
      default: {
        appendf(result, "\\u%o", Hexer((unsigned char)ch, 4));
        break;
      }
    }
    current = segmentEnd + 1;
  }
}

// JSON support for string
bool tryAppendJson(const std::string &value, std::string *result, const FailFrame &ff) {
  return tryAppendJson(std::string_view(value), result, ff.nest(HERE));
}

bool tryParseJson(ParseContext *ctx, std::string *result, const FailFrame &ff_in) {
  if (!ctx->tryConsumeChar('"', ff_in.nest(HERE))) {
    return false;
  }
  // Append non-special segments forever
  while (true) {
    auto ff = ctx->noteContext(HERE, ff_in);
    // Find next segment (up to \ or end of string), append it, and then deal with the \ or end of
    // string.
    auto endp = nextUnterminatedUnescapedSegment(ctx->current(), ctx->end());
    if (endp == ctx->end()) {
      return ff.fail(HERE, "Expected '\"', got end of string");
    }
    result->append(ctx->current(), endp);
    ctx->setCurrent(endp + 1);
    char ch = *endp;
    if (ch == '"') {
      return true;
    }
    passert(ch == '\\');
    if (!handleStringEscape(ctx, result, ff.nest(HERE))) {
      return false;
    }
  }
}

// JSON support for coding::Unit
bool tryAppendJson(const coding::Unit &/*value*/, std::string *result, const FailFrame &ff) {
  auto temp = std::tie();
  return tryAppendJson(temp, result, ff.nest(HERE));
}

bool tryParseJson(ParseContext *ctx, Unit */*result*/, const FailFrame &ff) {
  auto temp = std::tie();
  return tryParseJson(ctx, &temp, ff.nestWithType<Unit>(HERE));
}

bool tryParseSequence(ParseContext *ctx, bool useBraces, bool sizeIsDynamic,
    parseItemFunc_t parseItem, void *data, const FailFrame &ff_in) {
  auto ff_outer = ctx->noteContext(HERE, ff_in);
  char openChar = useBraces ? '{' : '[';
  char closeChar = useBraces ? '}' : ']';
  if (!ctx->tryConsumeChar(openChar, ff_in.nest(HERE))) {
    return false;
  }

  size_t nextIndex = 0;
  while (true) {
    auto cb = [nextIndex](std::ostream &s) {
      streamf(s, "Sequence element %o", nextIndex);
    };
    auto ffDel = ff_outer.nestWithDelegate(HERE, &cb);
    auto ff = ctx->noteContext(HERE, ffDel);

    if (ctx->maybeConsumeChar(closeChar)) {
      if (parseItem != nullptr && !sizeIsDynamic) {
        return ff.failf(HERE, "Sequence ended early: expected more than %o elements", nextIndex);
      }
      return true;
    }
    if (parseItem == nullptr) {
      return ff.failf(HERE, "Sequence too long: expected end, but there are more than %o", nextIndex);
    }

    if (nextIndex != 0 && !ctx->tryConsumeChar(',', ff.nest(HERE))) {
      return false;
    }
    ParseItemFuncHolder nextParser;
    if (!parseItem(ctx, data, &nextParser, ff.nest(HERE))) {
      return false;
    }
    parseItem = nextParser.value_;
    ++nextIndex;
  }
}

namespace internal {
namespace variantHelpers {
bool tryParseJsonVariantTag(ParseContext *ctx, const char * const *variantTagsBegin,
  const char * const *variantTagsEnd, size_t parsersSize, size_t *index, const FailFrame &ff) {
  if (!ctx->tryConsumeChar('[', ff.nest(HERE))) {
    return false;
  }
  auto borrowedString = ctx->borrowBufferString();
  auto &str = borrowedString.str();
  if (!tryParseJson(ctx, &str, ff.nest(HERE)) ||
    !ctx->tryConsumeChar(',', ff.nest(HERE))) {
    return false;
  }
  auto iter = std::find(variantTagsBegin, variantTagsEnd, str);
  if (iter == variantTagsEnd) {
    return ff.failf(HERE, "Unknown tag %o", str);
  }
  *index = iter - variantTagsBegin;
  if (*index >= parsersSize) {
    return ff.failf(HERE, "index %o >= parsers size %o", index, parsersSize);
  }
  return true;
}
}  // namespace variantHelpers
}  // namespace internal

}  // namespace kosak::coding
