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

#include <string>
#include <utility>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/mystream.h"

#define KOSAK_CODING_HERE __FUNCTION__, __FILE__, __LINE__

namespace kosak::coding {
class FailFrame;
class FailRoot;

namespace internal {
class SimpleFailFrame;
template<typename T>
class TypedFailFrame;
class DelegateFailFrame;
}  // namespace internal

class FailFrame {
public:
  FailFrame(FailRoot *root, const FailFrame *next) : root_(root), next_(next) {}
  DISALLOW_COPY_AND_ASSIGN(FailFrame);
  DISALLOW_MOVE_COPY_AND_ASSIGN(FailFrame);
  virtual ~FailFrame() = default;

  [[nodiscard]]
  bool fail(const char *func, const char *file, int line, std::string_view s = "") const;

  template<typename ...Args>
  [[nodiscard]]
  bool failf(const char *func, const char *file, int line, const char *fmt, Args &&...args) const;

  internal::SimpleFailFrame nest(const char *func, const char *file, int line) const;

  template<typename T>
  internal::TypedFailFrame<T> nestWithType(const char *func, const char *file, int line) const;

  internal::DelegateFailFrame nestWithDelegate(const char *func, const char *file, int line,
      const kosak::coding::Delegate<void, std::ostream &> &cb) const;

  bool ok() const;
  FailRoot *root() const { return root_; }

  virtual void streamFrameTo(std::ostream &s) const = 0;

protected:
  void maybeAddSeparator() const;
  void setFailFlagAndMaybeAddStackTrace(const char *func, const char *file, int line) const;

  FailRoot *root_ = nullptr;
  const FailFrame *next_ = nullptr;
};

class FailRoot final : public FailFrame {
public:
  FailRoot();

  explicit FailRoot(bool quiet);
  DISALLOW_COPY_AND_ASSIGN(FailRoot);
  DISALLOW_MOVE_COPY_AND_ASSIGN(FailRoot);

  ~FailRoot() final;

  bool quiet() const { return quiet_; }

  void streamFrameTo(std::ostream &s) const final;

private:
  bool quiet_ = false;
  bool ok_ = true;
  kosak::coding::MyOstringStream oss_;

  friend std::ostream &operator<<(std::ostream &s, const FailRoot &o) {
    return s << o.oss_.str();
  }

  friend class FailFrame;
};

inline bool FailFrame::ok() const {
  return root_->ok_;
}

namespace internal {
class FileLineFailFrame : public FailFrame {
public:
  FileLineFailFrame(FailRoot *root, const FailFrame *next, const char *func, const char *file, int line) :
      FailFrame(root, next), func_(func), file_(file), line_(line) {}

protected:
  void streamToHelper(std::ostream &s) const;

  const char *func_ = nullptr;
  const char *file_ = nullptr;
  int line_ = 0;
};

class SimpleFailFrame final : public FileLineFailFrame {
public:
  SimpleFailFrame(FailRoot *root, const FailFrame *next, const char *func, const char *file, int line) :
      FileLineFailFrame(root, next, func, file, line) {}

  void streamFrameTo(std::ostream &s) const final;
};

template<typename T>
class TypedFailFrame final : public FileLineFailFrame {
public:
  TypedFailFrame(FailRoot *root, const FailFrame *next, const char *func, const char *file, int line) :
      FileLineFailFrame(root, next, func, file, line) {}

  void streamFrameTo(std::ostream &s) const final {
    streamToHelper(s);
    streamf(s, " type %o", kosak::coding::humanReadableTypeName<T>());
  }
};

class DelegateFailFrame : public FileLineFailFrame {
public:
  DelegateFailFrame(FailRoot *root, const FailFrame *next, const char *func, const char *file,
      int line, const Delegate<void, std::ostream &> cb) :
      FileLineFailFrame(root, next, func, file, line), cb_(cb) {}

  void streamFrameTo(std::ostream &s) const final;

private:
  const Delegate<void, std::ostream &> cb_;
};
}  // namespace internal

inline internal::SimpleFailFrame FailFrame::nest(const char *func, const char *file, int line) const {
  return {root_, this, func, file, line};
}

template<typename T>
inline internal::TypedFailFrame<T>
FailFrame::nestWithType(const char *func, const char *file, int line) const {
  return {root_, this, func, file, line};
}

inline internal::DelegateFailFrame
FailFrame::nestWithDelegate(const char *func, const char *file, int line,
    const kosak::coding::Delegate<void, std::ostream &> &cb) const {
  return {root_, this, func, file, line, cb};
}

template<typename ...Args>
[[nodiscard]]
bool FailFrame::failf(const char *func, const char *file, int line, const char *fmt, Args &&...args)
  const {
  maybeAddSeparator();
  if (!root_->quiet()) {
    kosak::coding::streamf(root_->oss_, fmt, std::forward<Args>(args)...);
  }
  setFailFlagAndMaybeAddStackTrace(func, file, line);
  return false;
}
}  // namespace kosak::coding
