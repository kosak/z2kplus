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
#include <pthread.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include "kosak/coding/mystream.h"

//--------------------------------------------------------------------------
// other "extensions" to C++
//--------------------------------------------------------------------------

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete; \
  TypeName &operator=(const TypeName&) = delete

#define DISALLOW_MOVE_COPY_AND_ASSIGN(TypeName) \
  TypeName(TypeName&&) = delete; \
  TypeName &operator=(TypeName&&) = delete

#define DECLARE_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  TypeName &operator=(const TypeName&)

#define DECLARE_MOVE_COPY_AND_ASSIGN(TypeName) \
  TypeName(TypeName&&) noexcept; \
  TypeName &operator=(TypeName&&) noexcept

#define DEFINE_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = default; \
  TypeName &operator=(const TypeName&) noexcept = default

#define DEFINE_MOVE_COPY_AND_ASSIGN(TypeName) \
  TypeName(TypeName&&) noexcept = default; \
  TypeName &operator=(TypeName&&) noexcept = default

#define DEFINE_ALL_COMPARISON_OPERATORS(TypeName) \
  friend bool operator<(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) < 0; } \
  friend bool operator<=(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) <= 0; } \
  friend bool operator==(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) == 0; } \
  friend bool operator!=(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) != 0; } \
  friend bool operator>=(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) >= 0; } \
  friend bool operator>(const TypeName &lhs, const TypeName &rhs) { return lhs.compare(rhs) > 0; }

#define DISALLOW_NEW_AND_DELETE() \
    void *operator new(size_t) = delete; \
    void *operator new[](size_t) = delete; \
    void operator delete(void *) = delete; \
    void operator delete[](void *) = delete

typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

// need to get rid of the above. Until then...
static_assert(std::is_same_v<int8, int8_t>);
static_assert(std::is_same_v<int16, int16_t>);
static_assert(std::is_same_v<int32, int32_t>);
static_assert(std::is_same_v<int64, int64_t>);

static_assert(std::is_same_v<uint8, uint8_t>);
static_assert(std::is_same_v<uint16, uint16_t>);
static_assert(std::is_same_v<uint32, uint32_t>);
static_assert(std::is_same_v<uint64, uint64_t>);

//------------------------------------------------------------------------
// classes & macros for debugging/tracing
//------------------------------------------------------------------------

namespace kosak::coding::internal {
// Dumps chars up to the next %o or NUL. Updates *fmt to the point past the %o or at the NUL.
// Returns true iff %o was the last thing seen.
bool dumpFormat(std::ostream &s, const char **fmt, bool placeholderExpected);
}  // namespace kosak::coding::internal


namespace kosak::coding {
template<typename T>
std::string toString(const T &o) {
  kosak::coding::MyOstringStream s;
  s << o;
  return std::move(s.str());
}

std::ostream &streamf(std::ostream &s, const char *fmt);

template<typename HEAD, typename... REST>
std::ostream &streamf(std::ostream &s, const char *fmt, const HEAD &head, REST&&... rest) {
  (void)kosak::coding::internal::dumpFormat(s, &fmt, true);
  s << head;
  return streamf(s, fmt, std::forward<REST>(rest)...);
}

template<typename... ARGS>
std::string stringf(const char *fmt, ARGS&&... args) {
  kosak::coding::MyOstringStream s;
  streamf(s, fmt, std::forward<ARGS>(args)...);
  return std::move(s.str());
}

template<typename... ARGS>
void appendf(std::string *buffer, const char *fmt, ARGS&&... args) {
  kosak::coding::MyOstringStream s(buffer);
  streamf(s, fmt, std::forward<ARGS>(args)...);
}

template<typename... ARGS>
std::ostream &coutf(const char *fmt, ARGS&&... args) {
  streamf(std::cout, fmt, std::forward<ARGS>(args)...);
#ifndef NDEBUG
  std::cout.flush();
#endif
  return std::cout;
}

template<typename... ARGS>
std::ostream &cerrf(const char *fmt, ARGS&&... args) {
  streamf(std::cerr, fmt, std::forward<ARGS>(args)...);
#ifndef NDEBUG
  std::cerr.flush();
#endif
  return std::cerr;
}

// in case the template version fails
#define STATIC_ARRAYSIZE(A) (sizeof(A) / sizeof(0[A]))  /* NOLINT */

//template<class T, size_t N>
//constexpr size_t arraySize(const T (&)[N] ) { return N; }

namespace internal {

void breakpointHere();

class Logger {
public:
  Logger(const char *file, size_t line, const char *function) :
    file_(file), line_(line), function_(function) {}

  template<typename... ARGS>
  void logf(ARGS&&... args);

  static void alsoLogTo(std::ostream *s) {
    fileStream_ = s;
  }

  // Depending on how we are compiled (CMake likes to use full path names), __FILE__ can expand to
  // something verbose. This entry point allows the logger to eliminate a common verbose prefix
  // from __FILE__. It is meant to be called with 'function' set to __FUNCTION__ and 'numLevelsDeep'
  // set to how far away the caller is relative to the root of your project source tree. For example,
  // if main.cc is at /home/kosak/git/life/zarchive2000/experiments/008-filebased-backend/main.cc
  // then you would set numLevelsDeep to 0.
  // But if it were at .../blah/main.cc, then you would set it to 1.
  static void elidePrefix(const char *file, int numLevelsDeep);

  static void setThreadPrefix(std::string threadPrefix);

private:
  void logfSetup(MyOstringStream *buffer);
  void logfFinish(MyOstringStream *buffer);

  static pthread_mutex_t mutex_;
  static std::ostream *fileStream_;
  static std::string elidedPrefix_;
  friend class ArgLogger;

  static thread_local std::string threadPrefix_;

  const char *file_ = nullptr;
  size_t line_ = 0;
  const char *function_ = nullptr;
};

template<typename... ARGS>
void Logger::logf(ARGS&&... args) {
  MyOstringStream buffer;
  logfSetup(&buffer);
  streamf(buffer, std::forward<ARGS>(args)...);
  logfFinish(&buffer);
}

// Usage:
//   ArgLogger("a,b,c").Log(a, b, c)
//
// The first case prints (assuming a,b,c == 3,4,5):
// a,b,c: [3] [4] [5]

class ArgLogger {
public:
  explicit ArgLogger(const char *message) : message_(message) {}

  template<typename... ARGS>
  void Log(ARGS&&... args);

private:
  void LogHelper(std::ostream &s) {}

  template<typename HEAD, typename... REST>
  void LogHelper(std::ostream &s, HEAD &&head, REST&&... args);

  const char *message_;
};

template<typename... ARGS>
void ArgLogger::Log(ARGS&&... args) {
  MyOstringStream s;
  s << message_ << ':';
  LogHelper(s, std::forward<ARGS>(args)...);

  pthread_mutex_lock(&Logger::mutex_);
  std::cout << s.str();
#ifndef NDEBUG
  std::cout.flush();
#endif
  if (Logger::fileStream_ != nullptr) {
    *Logger::fileStream_ << s.str();
    Logger::fileStream_->flush();
  }
  pthread_mutex_unlock(&Logger::mutex_);
}

template<typename HEAD, typename... REST>
void ArgLogger::LogHelper(std::ostream &s, HEAD &&head, REST&&... args) {
  s << " [" << head << ']';
  LogHelper(s, std::forward<REST>(args)...);
}

}  // namespace internal

inline void elideLoggerPrefix(const char *file, int numLevelsDeep) {
  internal::Logger::elidePrefix(file, numLevelsDeep);
}

template<typename Result, typename... Args>
class Callback {
public:
  template<typename Callable>
  static std::shared_ptr<Callback> createFromCallable(Callable &&callable);

  virtual ~Callback() = default;
  virtual Result invoke(Args... args) = 0;
};

namespace internal {
template<typename Callable, typename Result, typename... Args>
class CallableCallback final : public Callback<Result, Args...> {
public:
  explicit CallableCallback(Callable &&callable) : callable_(std::move(callable)) {}
  ~CallableCallback() final = default;

  Result invoke(Args... args) final {
    return callable_(std::forward<Args>(args)...);
  }

private:
  Callable callable_;
};
}  // namespace internal

template<typename Result, typename... Args>
template<typename Callable>
std::shared_ptr<Callback<Result, Args...>> Callback<Result, Args...>::createFromCallable(Callable &&callable) {
  return std::make_shared<internal::CallableCallback<Callable, Result, Args...>>(std::move(callable));
}

namespace internal {
template<typename Lambda>
class AtScopeExit {
public:
  explicit AtScopeExit(Lambda lambda) : lambda_(lambda), active_(true) {}
  ~AtScopeExit() {
    if (active_) {
      lambda_();
    }
  }

  void release() { active_ = false; }

private:
  Lambda lambda_;
  bool active_;
};
}  // namespace internal

template<typename Lambda>
internal::AtScopeExit<Lambda> atScopeExit(Lambda lambda) {
  return internal::AtScopeExit<Lambda>(std::move(lambda));
}

}   // namespace kosak::coding

//------------------------------------------------------------------------

#define crash(...) do { warn(__VA_ARGS__); kosak::coding::internal::breakpointHere(); exit(1); } while(false)

#define warn(...) ::kosak::coding::internal::Logger( \
    __FILE__, __LINE__, __FUNCTION__).logf(__VA_ARGS__)

//The difference between myassert and passert is that myassert macros are removed
//entirely in the production version, and passert are not
#define passert(CONDITION, ...) \
  do { \
    if (CONDITION) ; else { \
      warn("Assertion failed: %o.", #CONDITION); \
      ::kosak::coding::internal::ArgLogger(#__VA_ARGS__) \
        .Log(__VA_ARGS__); \
      kosak::coding::internal::breakpointHere(); \
      exit(1); \
    } \
  } while (false)
#define notreached crash("notreached!")

#ifndef NDEBUG
#define debug(...) ::kosak::coding::internal::Logger( \
    __FILE__, __LINE__, __FUNCTION__).logf(__VA_ARGS__)
#define myassert(CONDITION, ...) passert(CONDITION, __VA_ARGS__)
#else
#define debug(...) /* nothing */
#define myassert(CONDITION, ...)
#endif

namespace kosak::coding {

class Unit final {
public:
  int compare(const Unit &other) const { return 0; }
  DEFINE_ALL_COMPARISON_OPERATORS(Unit);

  friend std::ostream &operator<<(std::ostream &s, const Unit &o) {
    return s << "Unit()";
  }
};

// Provides a human-readable explanation of type T by extracting out part of the string that gcc
// provides with __PRETTY_FUNCTION__. Even though this looks very inefficient, gcc can compile it
// it down to extremely tight code because __PRETTY_FUNCTION__ is a compile-time string constant.
template<typename T>
std::string_view humanReadableTypeName() {
  constexpr const char *funcText = __PRETTY_FUNCTION__;
  static constexpr const char needle[] = "T = ";
  const auto *start = strstr(funcText, needle) + strlen(needle);
  if (start == nullptr) {
    throw std::runtime_error("sad");
  }
  const auto *end = strchr(start, ';');
  if (end == nullptr) {
    throw std::runtime_error("sad");
  }
  return std::string_view(start, end - start);
}

namespace internal {
class Manip {
public:
  typedef std::ostream &(*fp_t)(std::ostream &s);

  explicit Manip(fp_t fp) : fp_(fp) {}

private:
  fp_t fp_;

  friend std::ostream &operator<<(std::ostream &s, const Manip &manip) {
    (*manip.fp_)(s);
    return s;
  }
};
}  // namespace internal

inline internal::Manip makeManip(internal::Manip::fp_t fp) {
  return internal::Manip(fp);
}

template<typename T>
std::vector<T> makeReservedVector(size_t size) {
  std::vector<T> result;
  result.reserve(size);
  return result;
}

template <class Dest, class Source>
inline Dest bit_cast(Source const &source) {
    static_assert(sizeof(Dest) == sizeof(Source),
        "size of destination and source objects must be equal");
    static_assert(std::is_trivially_copyable<Dest>::value,
       "destination type must be trivially copyable.");
    static_assert(std::is_trivially_copyable<Source>::value,
       "source type must be trivially copyable");

    Dest dest;
    std::memcpy(static_cast<void*>(&dest), static_cast<const void*>(&source), sizeof(dest));
    return dest;
}

template<typename T, typename... Args>
void reconstructInPlace(T *obj, Args&&... args) {
  obj->~T();
  try {
    new((void *)obj) T(std::forward<Args>(args)...);
  } catch (...) {
    abort();
  }
}


namespace internal {
struct NoDelete {
  template<typename T>
  void operator()(T *item) { }
};
}  // namespace internal

// We use this for raw pointers when we want the compiler's auto-generated move operations
// to be safe (i.e. to leave a nullptr in the moved-from variable).
template<typename T>
using UnownedPtr = std::unique_ptr<T, internal::NoDelete>;
}  // namespace kosak::coding

// These are relatively evil (because I have no business adding stuff to std, particularly an
// unspecialized template like this). But it's sooooo convenient, and close to necessary. So, in
// for a dime, in for a dollar.
namespace kosak::coding::internal {
template<int WHICH, typename ...ITEMS>
void dumpTuple(std::ostream &s, const std::tuple<ITEMS...> &tuple);

template<bool Done, size_t Index, typename ...Types>
struct TupleDumper {
  static void dump(std::ostream &s, const std::tuple<Types...> &tuple) {
    if (Index > 0) {
      s << ", ";
    }
    s << std::get<Index>(tuple);
    TupleDumper<Index + 1 == sizeof...(Types), Index + 1, Types...>::dump(s, tuple);
  }
};

template<size_t Index, typename ...Types>
struct TupleDumper<true, Index, Types...> {
  static void dump(std::ostream &s, const std::tuple<Types...> &tuple) {
    // Base case. Do nothing.
  }
};

template<typename ...Types>
void dumpTuple(std::ostream &s, const std::tuple<Types...> &tuple) {
  constexpr bool done = sizeof...(Types) == 0;
  TupleDumper<done, 0, Types...>::dump(s, tuple);
}
}  // namespace kosak::coding::internal {

namespace std {  // NOLINT
template<typename T>
ostream &operator<<(ostream &s, const shared_ptr<T> &o) {
  if (o == nullptr) {
    return s << "(nullptr)";
  }
  return s << '^' << *o;
}

template<typename T>
ostream &operator<<(ostream &s, const unique_ptr<T> &o) {
  if (o == nullptr) {
    return s << "(nullptr)";
  }
  return s << '^' << *o;
}

template<typename K, typename V>
ostream &operator<<(ostream &s, const pair<K, V> &o) {
  return kosak::coding::streamf(s, "(%o,%o)", o.first, o.second);
}

template<typename T>
ostream &operator<<(ostream &s, const optional<T> &o) {
  if (!o.has_value()) {
    return s << "(none)";
  }
  return s << o.value();
}

// Do it this way so we don't try to match against variant<> (empty arg list) which leads to
// compilation errors.
template<typename First, typename... Rest>
ostream &operator<<(ostream &s, const std::variant<First, Rest...> &o) {
  auto index = o.index();
  auto lambda = [&s, index](auto &&item) -> std::ostream& {
      return kosak::coding::streamf(s, "[%o aka %o]: %o",
          index, kosak::coding::humanReadableTypeName<decltype(item)>(), item);
  };
  return std::visit(lambda, o);
}

template<typename... ITEMS>
ostream &operator<<(ostream &s, const tuple<ITEMS...> &o) {
  s << '[';
  kosak::coding::internal::dumpTuple(s, o);
  s << ']';
  return s;
}

template<typename T, typename A>
ostream &operator<<(ostream &s, const vector<T, A> &o) {
  s << '[';
  const char *comma = "";
  for (const auto &item : o) {
    kosak::coding::streamf(s, "%o%o", comma, item);
    comma = ", ";
  }
  return s << ']';
}

template<typename T, typename C, typename A>
ostream &operator<<(ostream &s, const set<T, C, A> &o) {
  s << '{';
  const char *comma = "";
  for (const auto &item : o) {
    kosak::coding::streamf(s, "%o%o", comma, item);
    comma = ", ";
  }
  return s << '}';
}

template<typename K, typename V, typename C, typename A>
ostream &operator<<(ostream &s, const map<K, V, C, A> &o) {
  s << '{';
  const char *comma = "";
  for (const auto &kvp : o) {
    kosak::coding::streamf(s, "%o%o: %o", comma, kvp.first, kvp.second);
    comma = ", ";
  }
  return s << '}';
}
}  // namespace std
