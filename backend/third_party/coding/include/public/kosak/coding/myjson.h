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

#pragma once

#include <algorithm>
#include <array>
#include <experimental/array>
#include <cuchar>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/dumping.h"
#include "kosak/coding/failures.h"

#define DECLARE_TYPICAL_JSON(TYPE) \
bool tryAppendJsonHelper(std::string *result, const kosak::coding::FailFrame &ff) const; \
bool tryParseJsonHelper(kosak::coding::ParseContext *ctx, const kosak::coding::FailFrame &ff); \
friend bool tryAppendJson(const TYPE &o, std::string *result, const kosak::coding::FailFrame &ff) { \
  return o.tryAppendJsonHelper(result, ff.nest(KOSAK_CODING_HERE)); \
} \
friend bool tryParseJson(kosak::coding::ParseContext *ctx, TYPE *result, const kosak::coding::FailFrame &ff) { \
  return result->tryParseJsonHelper(ctx, ff.nest(KOSAK_CODING_HERE)); \
} \
static_assert(true) /* force our user to provide a final semicolon */

#define DEFINE_TYPICAL_JSON(TYPE, ...) \
bool TYPE::tryAppendJsonHelper(std::string *result, const kosak::coding::FailFrame &ff) const { \
  using kosak::coding::tryAppendJson; \
  auto temp = std::tie(__VA_ARGS__); \
  return tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE)); \
} \
bool TYPE::tryParseJsonHelper(kosak::coding::ParseContext *ctx, const kosak::coding::FailFrame &ff) { \
  using kosak::coding::tryParseJson; \
  auto temp = std::tie(__VA_ARGS__); \
  return tryParseJson(ctx, &temp, ff.nest(KOSAK_CODING_HERE)); \
} \
static_assert(true) /* force our user to provide a final semicolon */

#define LEGACY_DEFINE_DELEGATED_JSON(TYPE, ITEM) \
friend bool tryAppendJson(const TYPE &o, std::string *result, const kosak::coding::FailFrame &ff) { \
  using kosak::coding::tryAppendJson; \
  return tryAppendJson(o.ITEM, result, ff.nest(KOSAK_CODING_HERE)); \
} \
friend bool tryParseJson(kosak::coding::ParseContext *ctx, TYPE *result, \
    const kosak::coding::FailFrame &ff) { \
  using kosak::coding::tryParseJson; \
  return tryParseJson(ctx, &result->ITEM, ff.nest(KOSAK_CODING_HERE)); \
} \
static_assert(true) /* force our user to provide a final semicolon */

namespace kosak::coding {

class ParseContext;

// JSON support for char
bool tryAppendJson(char c, std::string *result, const kosak::coding::FailFrame &ff);
bool tryParseJson(ParseContext *ctx, char *result, const kosak::coding::FailFrame &ff);

// JSON support for bool
bool tryAppendJson(bool b, std::string *result, const kosak::coding::FailFrame &ff);
bool tryParseJson(ParseContext *ctx, bool *result, const kosak::coding::FailFrame &ff);

// JSON support for various integers (I probably missed a few)
#define MAKE_JSON_SUPPORT(Type) \
inline bool tryAppendJson(Type value, std::string *result, \
    const kosak::coding::FailFrame &/*ff*/) { \
  appendf(result, "%o", value); \
  return true; \
} \
bool tryParseJson(ParseContext *ctx, Type *result, const kosak::coding::FailFrame &ff);

MAKE_JSON_SUPPORT(signed char)

MAKE_JSON_SUPPORT(unsigned char)

MAKE_JSON_SUPPORT(short)

MAKE_JSON_SUPPORT(unsigned short)

MAKE_JSON_SUPPORT(int)

MAKE_JSON_SUPPORT(unsigned int)

MAKE_JSON_SUPPORT(long)

MAKE_JSON_SUPPORT(unsigned long)

MAKE_JSON_SUPPORT(long long)

MAKE_JSON_SUPPORT(unsigned long long)
#undef MAKE_JSON_SUPPORT

// JSON support for double.
inline bool tryAppendJson(double value, std::string *result,
    const kosak::coding::FailFrame &/*ff*/) {
  appendf(result, "%o", value);
  return true;
}

bool tryParseJson(ParseContext *ctx, double *result, const kosak::coding::FailFrame &ff);

// JSON support for enum
namespace internal::enumHelpers {
template<typename ...Types>
constexpr auto makeArrayOfSize_t(Types&&...args) {
  return std::experimental::make_array((size_t)args...);
}

bool tryAppendJson(const size_t *values, const char * const *tags, size_t numElements,
  size_t value, std::string *result, const kosak::coding::FailFrame &ff);
bool tryParseJson(const size_t *values, const char * const *tags, size_t numElements,
  kosak::coding::ParseContext *ctx, size_t *result, const kosak::coding::FailFrame &ff);
std::ostream &stream(const size_t *values, const char * const *tags, size_t numElements,
  std::ostream &s, size_t item);
}  // namespace internal::enumHelpers

// Example:
// .h file:
// enum class Orientation { Horizontal, Vertical };
// "OrientationHolder" is an arbitrary (unused) class name.
//
// DECLARE_ENUM_JSON(OrientationHolder, Orientation,
//   (Orientation::Horizontal, Orientation::Vertical),
//   ("Horizontal", "Vertical")
// );
//
// .cc file:
// DEFINE_ENUM_JSON(OrientationHolder)

#define DECLARE_ENUM_JSON(HOLDER, TYPE, VALUES, TAGS) \
  struct HOLDER { \
    static constexpr auto enumValues = kosak::coding::internal::enumHelpers::makeArrayOfSize_t VALUES; \
    static constexpr auto enumTags = std::experimental::make_array<const char*> TAGS; \
    static_assert(std::is_same_v<TYPE, decltype(std::experimental::make_array VALUES)::value_type>); \
    static_assert(enumValues.size() == enumTags.size()); \
  }; \
  inline bool tryAppendJson(TYPE value, std::string *result, const kosak::coding::FailFrame &ff) { \
    return kosak::coding::internal::enumHelpers::tryAppendJson( \
      HOLDER::enumValues.data(), HOLDER::enumTags.data(), HOLDER::enumValues.size(), \
      (size_t)value, result, ff.nest(KOSAK_CODING_HERE)); \
  } \
  inline bool tryParseJson(kosak::coding::ParseContext *ctx, TYPE *result, const kosak::coding::FailFrame &ff) { \
    size_t temp; \
    if (!kosak::coding::internal::enumHelpers::tryParseJson( \
      HOLDER::enumValues.data(), HOLDER::enumTags.data(), HOLDER::enumValues.size(), ctx, &temp, ff.nest(KOSAK_CODING_HERE))) { \
      return false; \
    } \
    *result = (TYPE)temp; \
    return true; \
  } \
  inline std::ostream &operator<<(std::ostream &s, const TYPE &o) { \
    return kosak::coding::internal::enumHelpers::stream( \
      HOLDER::enumValues.data(), HOLDER::enumTags.data(), HOLDER::enumValues.size(), \
      s, (size_t)o); \
  } \
  static_assert(true) /* force our user to provide a final semicolon */

#define DEFINE_ENUM_JSON(HOLDER) \
  constexpr std::array<size_t, HOLDER::enumValues.size()> HOLDER::enumValues; \
  constexpr std::array<const char *, HOLDER::enumTags.size()> HOLDER::enumTags

// JSON support for const char * (write only)
bool tryAppendJson(const char *value, std::string *result, const kosak::coding::FailFrame &ff);
// Deliberately not implemented
bool tryParseJson(ParseContext *ctx, const char **result, const kosak::coding::FailFrame &ff);

// JSON support for strings.
bool
tryAppendJson(const std::string &value, std::string *result, const kosak::coding::FailFrame &ff);
bool tryParseJson(ParseContext *ctx, std::string *result, const kosak::coding::FailFrame &ff);

// JSON support for string_view
bool tryAppendJson(const std::string_view &value, std::string *result,
  const kosak::coding::FailFrame &ff);
bool tryParseJson(ParseContext *ctx, std::string_view *result, const kosak::coding::FailFrame &ff);

// JSON support for shared_ptr
template<typename T>
bool tryAppendJson(const std::shared_ptr<T> &value, std::string *result,
    const kosak::coding::FailFrame &ff);
template<typename T>
bool tryParseJson(ParseContext *ctx, std::shared_ptr<T> *result, const kosak::coding::FailFrame &ff);

// JSON support for unique_ptr
template<typename T, typename D>
bool tryAppendJson(const std::unique_ptr<T, D> &value, std::string *result,
  const kosak::coding::FailFrame &ff);
template<typename T, typename D>
bool
tryParseJson(ParseContext *ctx, std::unique_ptr<T, D> *result, const kosak::coding::FailFrame &ff);

// JSON support for coding::Unit
bool
tryAppendJson(const coding::Unit &value, std::string *result, const kosak::coding::FailFrame &ff);
bool tryParseJson(ParseContext *ctx, coding::Unit *result, const kosak::coding::FailFrame &ff);

// JSON support for Optional
template<typename T>
bool tryAppendJson(const std::optional<T> &value, std::string *result,
  const kosak::coding::FailFrame &ff);
template<typename T>
bool tryParseJson(ParseContext *ctx, std::optional<T> *result, const kosak::coding::FailFrame &ff);

// JSON support for pair
template<typename T1, typename T2>
bool tryAppendJson(const std::pair<T1, T2> &value, std::string *result,
  const kosak::coding::FailFrame &ff);
template<typename T1, typename T2>
bool tryParseJson(ParseContext *ctx, std::pair<T1, T2> *result, const kosak::coding::FailFrame &ff);

// JSON support for tuples
template<typename ...Args>
bool
tryAppendJson(const std::tuple<Args...> &tuple, std::string *s, const kosak::coding::FailFrame &ff);
template<typename ...Args>
bool
tryParseJson(ParseContext *ctx, std::tuple<Args...> *result, const kosak::coding::FailFrame &ff);

// JSON support for vectors
template<typename T, typename A>
bool
tryAppendJson(const std::vector<T, A> &vec, std::string *result, const kosak::coding::FailFrame &ff);
template<typename T, typename A>
bool tryParseJson(ParseContext *ctx, std::vector<T, A> *result, const kosak::coding::FailFrame &ff);

// JSON support for maps.
template<typename K, typename V, typename C, typename A>
bool tryAppendJson(const std::map<K, V, C, A> &m, std::string *result,
  const kosak::coding::FailFrame &ff);
template<typename K, typename V, typename C, typename A>
bool
tryParseJson(ParseContext *ctx, std::map<K, V, C, A> *result, const kosak::coding::FailFrame &ff);

// JSON support for my JSON-ish dictionary type
class KeyMaster {
  typedef const kosak::coding::FailFrame FailFrame;
public:
  template<size_t N>
  explicit KeyMaster(const std::array<const char *, N> &keys) : KeyMaster(keys.data(), keys.size()) {}

  KeyMaster(const char * const *keys, size_t numKeys) : keys_(keys), numKeys_(numKeys) {}

  bool tryGetIndexFromKey(std::string_view key, size_t *index, const FailFrame &ff) const;
  bool tryGetKeyFromIndex(size_t index, std::string_view *key, const FailFrame &ff) const;

private:
  const char * const *keys_ = nullptr;
  size_t numKeys_ = 0;
};

template<typename ...Args>
bool tryAppendJsonDictionary(const KeyMaster &keyMaster, const std::tuple<Args...> &values,
  const std::array<bool, sizeof...(Args)> &present, std::string *result,
  const kosak::coding::FailFrame &ff);
template<typename ...Args>
bool tryParseJsonDictionary(ParseContext *ctx, const KeyMaster &keyMaster,
  std::tuple<Args...> *result, const kosak::coding::FailFrame &ff);

// Implementations of the above stuff.
namespace internal {
class BorrowedStringHolder {
public:
  BorrowedStringHolder(std::vector<std::string> *owner, std::string str);
  DISALLOW_COPY_AND_ASSIGN(BorrowedStringHolder);
  DISALLOW_MOVE_COPY_AND_ASSIGN(BorrowedStringHolder);
  ~BorrowedStringHolder();

  std::string &str() { return str_; }
  const std::string &str() const { return str_; }

private:
  // The vector where it is returned to at destruction time. Does not own.
  std::vector<std::string> *owner_ = nullptr;
  // The borrowed string.
  std::string str_;
};

class ContextFrame final : public kosak::coding::internal::FileLineFailFrame {
public:
  ContextFrame(FailRoot *root, const FailFrame *next, const char *func, const char *file, int line,
      std::string_view context, size_t prefixSize) :
      kosak::coding::internal::FileLineFailFrame(root, next, func, file, line), context_(context),
      prefixSize_(prefixSize) {}

  void streamFrameTo(std::ostream &s) const final;
private:
  std::string_view context_;
  size_t prefixSize_ = 0;
};
}  // namespace internal

class ParseContext {
  typedef kosak::coding::FailFrame FailFrame;
public:
  explicit ParseContext(std::string_view sr);
  DISALLOW_COPY_AND_ASSIGN(ParseContext);
  ~ParseContext();

  //  explicit ParseContext(std::string_view sr) : begin_(sr.begin()), end_(sr.end()),
  //    current_(sr.begin()) {}
  bool tryConsumeChar(char ch, const FailFrame &ff);
  bool maybeConsumeChar(char ch);
  bool tryFinishConsuming(char alreadyConsumed, std::string_view suffix,
    const FailFrame &ff);
  void consumeWhitespace();

  const char *begin() const { return begin_; }

  const char *end() const { return end_; }

  const char *current() const { return current_; }

  void setCurrent(const char *newValue) { current_ = newValue; }

  std::string_view asStringView() const { return std::string_view(current_, end_ - current_); }

  internal::BorrowedStringHolder borrowBufferString();

  internal::ContextFrame noteContext(const char *func, const char *file, int line,
      const FailFrame &ff) const;

private:
  const char *begin_ = nullptr;
  const char *end_ = nullptr;
  const char *current_ = nullptr;
  // We keep some strings around for recycling, for routines that need to borrow one temporarily.
  std::vector<std::string> recycleBin_;
};

namespace internal {
template<typename FancyPointer>
bool tryAppendFancyPointer(const FancyPointer &value, std::string *result,
  const kosak::coding::FailFrame &ff) {
  if (value == nullptr) {
    result->append("null");
    return true;
  }
  return tryAppendJson(*value.get(), result, ff.nest(KOSAK_CODING_HERE));
}

template<typename FancyPointer>
bool
tryParseFancyPointer(ParseContext *ctx, FancyPointer *result, const kosak::coding::FailFrame &ff) {
  if (ctx->maybeConsumeChar('n')) {
    if (!ctx->tryFinishConsuming('n', "ull", ff.nestWithType<FancyPointer>(KOSAK_CODING_HERE))) {
      return false;
    }
    result->reset();
    return true;
  }
  // FancyPointer might point to a const type. This is not a problem. We simply make a non-const
  // type, populate it, and then point the FP to it.
  typedef typename std::remove_const<typename FancyPointer::element_type>::type element_type_nonconst;
  auto *p = new element_type_nonconst();
  result->reset(p);
  return tryParseJson(ctx, p, ff.nestWithType<FancyPointer>(KOSAK_CODING_HERE));
}
}  // internal

// JSON support for shared_ptr
template<typename T>
bool tryAppendJson(const std::shared_ptr<T> &value, std::string *result,
    const kosak::coding::FailFrame &ff) {
  return internal::tryAppendFancyPointer(value, result, ff.nest(KOSAK_CODING_HERE));
}

template<typename T>
bool
tryParseJson(ParseContext *ctx, std::shared_ptr<T> *result, const kosak::coding::FailFrame &ff) {
  return internal::tryParseFancyPointer(ctx, result, ff.nest(KOSAK_CODING_HERE));
}

// JSON support for unique_ptr
template<typename T, typename D>
bool tryAppendJson(const std::unique_ptr<T, D> &value, std::string *result,
    const kosak::coding::FailFrame &ff) {
  return internal::tryAppendFancyPointer(value, result, ff.nest(KOSAK_CODING_HERE));
}

template<typename T, typename D>
bool
tryParseJson(ParseContext *ctx, std::unique_ptr<T, D> *result, const kosak::coding::FailFrame &ff) {
  return internal::tryParseFancyPointer(ctx, result, ff.nest(KOSAK_CODING_HERE));
}

// JSON support for std::optional
template<typename T>
bool tryAppendJson(const std::optional<T> &value, std::string *result,
    const kosak::coding::FailFrame &ff) {
  if (!value.has_value()) {
    auto temp = std::tie();
    return tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE));
  }
  auto temp = std::tie(*value);
  return tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE));
}

template<typename T>
bool tryParseJson(ParseContext *ctx, std::optional<T> *result,
    const kosak::coding::FailFrame &ff) {
  if (!ctx->tryConsumeChar('[', ff.nestWithType<T>(KOSAK_CODING_HERE))) {
    return false;
  }
  if (ctx->maybeConsumeChar(']')) {
    result->reset();
    return true;
  }
  *result = T();
  return tryParseJson(ctx, &result->value(), ff.nestWithType<T>(KOSAK_CODING_HERE)) &&
    ctx->tryConsumeChar(']', ff.nestWithType<T>(KOSAK_CODING_HERE));
}

// JSON support for std::variant

namespace internal::variantHelpers {

template<typename VARIANT, size_t ...INDICES>
struct ParsersHolder {
  typedef bool (*parser_t)(ParseContext *ctx, VARIANT *result, const kosak::coding::FailFrame &ff);
  static constexpr const size_t size = std::variant_size_v<VARIANT>;

  template<size_t Index>
  static bool tryParseJsonAlternative(ParseContext *ctx, VARIANT *result,
      const kosak::coding::FailFrame &ff) {
    typename std::variant_alternative_t<Index, VARIANT> value;
    using kosak::coding::tryParseJson;
    if (!tryParseJson(ctx, &value, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
    *result = std::move(value);
    return true;
  }

  static constexpr std::array<parser_t, size> parsers = std::experimental::make_array(
    &tryParseJsonAlternative<INDICES>...);
};

template<typename HOLDER, typename VARIANT>
bool tryAppendJson(const VARIANT &value, std::string *result, const kosak::coding::FailFrame &ff) {
  size_t index = value.index();
  if (index >= HOLDER::variantTags.size()) {
    return ff.failf(KOSAK_CODING_HERE, "%o >= %o", index, HOLDER::variantTags.size());
  }
  const char *tag = HOLDER::variantTags[index];
  auto lambda = [tag, result, &ff](auto &&item) {
    auto temp = std::tie(tag, item);
    return tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE));
  };
  return std::visit(lambda, value);
}

bool tryParseJsonVariantTag(ParseContext *ctx, const char * const *variantTagsBegin,
  const char * const *variantTagsEnd, size_t parsersSize, size_t *index,
  const kosak::coding::FailFrame &ff);

template<typename HOLDER, typename VARIANT, size_t ...INDICES>
bool tryParseJson(ParseContext *ctx, std::index_sequence<INDICES...> /*sequence*/, VARIANT *result,
    const kosak::coding::FailFrame &ff) {
  size_t index;
  auto &parsers = ParsersHolder<VARIANT, INDICES...>::parsers;
  return tryParseJsonVariantTag(ctx, HOLDER::variantTags.begin(), HOLDER::variantTags.end(),
    parsers.size(), &index, ff.nestWithType<VARIANT>(KOSAK_CODING_HERE)) &&
    parsers[index](ctx, result, ff.nestWithType<VARIANT>(KOSAK_CODING_HERE)) &&
    ctx->tryConsumeChar(']', ff.nestWithType<VARIANT>(KOSAK_CODING_HERE));
}
}  // namespace internal::variantHelpers

// Example:
// workFarmOrderPayload_t is a variant
// WorkFarmOrderPayloadHolder is an arbitrary (unusued) class name.
//
// .h file:
// DECLARE_VARIANT_JSON(WorkFarmOrderPayloadHolder, workFarmOrderPayload_t,
//   ("LearnDict", "LearnJob", "ForgetJob", "NewGeneration", "Terminate"));
//
// .cc file:
// DECLARE_VARIANT_JSON(WorkFarmOrderPayloadHolder, workFarmOrderPayload_t,

#define DECLARE_VARIANT_JSON(HOLDER, VARIANT, TAGS) \
  struct HOLDER { \
    static constexpr auto variantTags = std::experimental::make_array TAGS; \
    static_assert(variantTags.size() == std::variant_size_v<VARIANT>); \
  }; \
  inline bool tryAppendJson(const VARIANT &value, std::string *result, const kosak::coding::FailFrame &ff) { \
    return kosak::coding::internal::variantHelpers::tryAppendJson<HOLDER>(value, result, ff.nest(KOSAK_CODING_HERE)); \
  }; \
  inline bool tryParseJson(kosak::coding::ParseContext *ctx, VARIANT *result, const kosak::coding::FailFrame &ff) { \
    return kosak::coding::internal::variantHelpers::tryParseJson<HOLDER>(ctx, \
      std::make_index_sequence<std::variant_size_v<VARIANT>>(), result, ff.nest(KOSAK_CODING_HERE)); \
  } \
  static_assert(true) /* force our user to provide a final semicolon */

#define DEFINE_VARIANT_JSON(HOLDER, VARIANT) static_assert(true) /* force our user to provide a final semicolon */

// Template implementation for pair
template<typename T1, typename T2>
bool tryAppendJson(const std::pair<T1, T2> &value, std::string *result,
    const kosak::coding::FailFrame &ff) {
  auto temp = std::tie(value.first, value.second);
  return tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE));
}

template<typename T1, typename T2>
bool tryParseJson(ParseContext *ctx, std::pair<T1, T2> *result, const kosak::coding::FailFrame &ff) {
  auto temp = std::tie(result->first, result->second);
  return tryParseJson(ctx, &temp, ff.nestWithType<std::pair<T1, T2>>(KOSAK_CODING_HERE));
}

struct ParseItemFuncHolder;
typedef bool (*parseItemFunc_t)(ParseContext *, void *tuple, ParseItemFuncHolder *nextParser,
    const kosak::coding::FailFrame &ff);
// I made this because I got annoyed with all the levels of indirection.
struct ParseItemFuncHolder {
  parseItemFunc_t value_ = nullptr;
};

bool tryParseSequence(ParseContext *ctx, bool useBraces, bool sizeIsDynamic,
    parseItemFunc_t parseItem, void *data, const kosak::coding::FailFrame &ff);

// Template support for tuple
namespace internal {
template<size_t Index, typename ...Args>
bool tryAppendTupleItem(const std::tuple<Args...> &tuple, std::string *result,
    const kosak::coding::FailFrame &ff) {
  if constexpr (Index == sizeof...(Args)) {
    return true;
  } else {
    if (Index != 0) {
      result->push_back(',');
    }
    return tryAppendJson(std::get<Index>(tuple), result, ff.nest(KOSAK_CODING_HERE)) &&
        tryAppendTupleItem < Index + 1 > (tuple, result, ff.nest(KOSAK_CODING_HERE));
  }
}

template<size_t Index, typename ...Args>
bool tryParseTupleItem(ParseContext *ctx, void *data, ParseItemFuncHolder *nextParser,
    const kosak::coding::FailFrame &ff_in) {
  parseItemFunc_t fp = nullptr;
  if constexpr (Index + 1 < sizeof...(Args)) {
    fp = &tryParseTupleItem < Index + 1, Args...>;
  }
  nextParser->value_ = fp;
  auto *tuple = static_cast<std::tuple<Args...>*>(data);
  auto &item = std::get<Index>(*tuple);
  auto cb = [](std::ostream &s) {
    streamf(s, "Tuple item %o", Index);
  };
  auto ffDel = ff_in.nestWithDelegate(KOSAK_CODING_HERE, &cb);
  auto ffType = ffDel.template nestWithType<decltype(item)>(KOSAK_CODING_HERE);
  return tryParseJson(ctx, &item, ffType.nest(KOSAK_CODING_HERE));
}
}  // namespace internal

template<typename ...Args>
bool tryAppendJson(const std::tuple<Args...> &tuple, std::string *result,
    const kosak::coding::FailFrame &ff) {
  result->push_back('[');
  if (!internal::tryAppendTupleItem<0>(tuple, result, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  result->push_back(']');
  return true;
}

template<typename ...Args>
bool tryParseJson(ParseContext *ctx, std::tuple<Args...> *result,
    const kosak::coding::FailFrame &ff) {
  parseItemFunc_t fp = nullptr;
  if constexpr (sizeof...(Args) > 0) {
    fp = &internal::tryParseTupleItem<0, Args...>;
  }
  void *data = static_cast<void*>(result);
  return tryParseSequence(ctx, false, false, fp, data,
      ff.nestWithType<std::tuple<Args...>>(KOSAK_CODING_HERE));
}

namespace internal {
template<typename T, typename A>
bool tryParseVectorItem(ParseContext *ctx, void *data, ParseItemFuncHolder *nextParser,
    const kosak::coding::FailFrame &ff) {
  T item;
  if (!tryParseJson(ctx, &item, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  auto *vec = static_cast<std::vector<T, A>*>(data);
  vec->push_back(std::move(item));
  nextParser->value_ = &tryParseVectorItem<T, A>;
  return true;
}
}  // namespace internal

template<typename T, typename A>
bool tryAppendJson(const std::vector<T, A> &vec, std::string *result,
    const kosak::coding::FailFrame &ff) {
  result->push_back('[');
  const char *sep = "";
  for (auto ip = vec.begin(); ip != vec.end(); ++ip) {
    result->append(sep);
    sep = ",";
    if (!tryAppendJson(*ip, result, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
  }
  result->push_back(']');
  return true;
}

template<typename T, typename A>
bool tryParseJson(ParseContext *ctx, std::vector<T, A> *result, const kosak::coding::FailFrame &ff) {
  typedef std::vector<T, A> vec_t;
  parseItemFunc_t fp = &internal::tryParseVectorItem<T, A>;
  void *data = static_cast<void*>(result);
  return tryParseSequence(ctx, false, true, fp, data, ff.nestWithType<vec_t>(KOSAK_CODING_HERE));
}

template<typename K, typename V, typename C, typename A>
bool tryAppendJson(const std::map<K, V, C, A> &m, std::string *result,
    const kosak::coding::FailFrame &ff) {
  result->push_back('[');
  const char *sep = "";
  for (auto ip = m.begin(); ip != m.end(); ++ip) {
    result->append(sep);
    sep = ",";
    auto temp = std::tie(ip->first, ip->second);
    if (!tryAppendJson(temp, result, ff.nest(KOSAK_CODING_HERE))) {
      return false;
    }
  }
  result->push_back(']');
  return true;
}

namespace internal {
template<typename K, typename V, typename C, typename A>
bool tryParseMapItem(ParseContext *ctx, void *data, ParseItemFuncHolder *nextParser,
    const kosak::coding::FailFrame &ff) {
  K key;
  V value;
  auto item = std::tie(key, value);
  if (!tryParseJson(ctx, &item, ff.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  auto *m = static_cast<std::map<K, V, C, A>*>(data);
  m->emplace(std::make_pair(std::move(key), std::move(value)));
  nextParser->value_ = &tryParseMapItem<K, V, C, A>;
  return true;
}
}  // namespace internal

template<typename K, typename V, typename C, typename A>
bool tryParseJson(ParseContext *ctx, std::map<K, V, C, A> *result, const kosak::coding::FailFrame &ff) {
  typedef std::map<K, V, C, A> map_t;
  parseItemFunc_t fp = &internal::tryParseMapItem<K, V, C, A>;
  void *data = static_cast<void*>(result);
  return tryParseSequence(ctx, false, true, fp, data, ff.nestWithType<map_t>(KOSAK_CODING_HERE));
}

template<typename ...Args>
bool tryAppendJsonDictionary(const KeyMaster &keyMaster, const std::tuple<Args...> &values,
    const std::array<bool, sizeof...(Args)> &present, std::string *result,
    const kosak::coding::FailFrame &ff) {
  return ff.failf(KOSAK_CODING_HERE, "Not implemented!");
}

namespace internal {
template<size_t INDEX, typename ...Args>
bool tryParseJsonDictionaryItem(ParseContext *ctx, size_t tupleIndex,
    std::tuple<Args...> *result, const kosak::coding::FailFrame &ff) {
  if constexpr(INDEX >= sizeof...(Args)) {
    return ff.template failf(KOSAK_CODING_HERE, "Can't get here");
  } else {
    if (tupleIndex == INDEX) {
      auto *element = &std::get<INDEX>(*result);
      return tryParseJson(ctx, element, ff.nest(KOSAK_CODING_HERE));
    }
    return tryParseJsonDictionaryItem<INDEX + 1>(ctx, tupleIndex, result, ff.nest(KOSAK_CODING_HERE));
  }
}
}  // namespace internal

// Not an especially efficient implementation, but this is only for legacy conversion.
template<typename ...Args>
bool tryParseJsonDictionary(ParseContext *ctx, const KeyMaster &keyMaster,
    std::tuple<Args...> *result, const kosak::coding::FailFrame &ff_in) {
  typedef std::tuple<Args...> tuple_t;
  auto ff_type = ff_in.nestWithType<tuple_t>(KOSAK_CODING_HERE);
  auto ff_outer = ctx->noteContext(KOSAK_CODING_HERE, ff_type);
  if (!ctx->tryConsumeChar('[', ff_outer.nest(KOSAK_CODING_HERE))) {
    return false;
  }
  std::string key;
  bool firstTime = true;
  while (true) {
    auto ff_inner = ctx->noteContext(KOSAK_CODING_HERE, ff_outer);
    key.clear();
    if (ctx->maybeConsumeChar(']')) {
      // End of dictionary
      return true;
    }
    if (!firstTime) {
      if (!ctx->tryConsumeChar(',', ff_inner.nest(KOSAK_CODING_HERE))) {
        return false;
      }
    }
    firstTime = false;

    size_t index;
    {
      if (!ctx->tryConsumeChar('[', ff_inner.nest(KOSAK_CODING_HERE)) ||  // start of [key, value]
          !tryParseJson(ctx, &key, ff_inner.nest(KOSAK_CODING_HERE)) ||  // key
          !ctx->tryConsumeChar(',', ff_inner.nest(KOSAK_CODING_HERE))) {  // comma
        return false;
      }
      if (!keyMaster.tryGetIndexFromKey(key, &index, ff_inner.nest(KOSAK_CODING_HERE))) {
        return false;
      }
      if (index >= sizeof...(Args)) {
        return ff_inner.failf(KOSAK_CODING_HERE, "Key index (%o) >= tuple size(%o)", index,
            sizeof...(Args));
      }
    }
    if (!internal::tryParseJsonDictionaryItem<0>(ctx, index, result, ff_inner.nest(KOSAK_CODING_HERE)) ||  //value
        !ctx->tryConsumeChar(']', ff_inner.nest(KOSAK_CODING_HERE))) {  // end of [key, value]
      return false;
    }
  }
}
}  // namespace kosak::coding
