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

#include <string_view>
#include "kosak/coding/coding.h"

namespace kosak::coding {
namespace internal {

namespace {
void doNothing() {
}

void dumpTillPercentOrEnd(std::ostream &result, const char **fmt) {
  const char *start = *fmt;
  const char *p = start;
  while (true) {
    char ch = *p;
    if (ch == '\0' || ch == '%') {
      break;
    }
    ++p;
  }
  if (p != start) {
    result.write(start, p - start);
    *fmt = p;
  }
}
}  // namespace

void breakpointHere() {
  doNothing();
}

bool dumpFormat(std::ostream &result, const char **fmt, bool placeholderExpected) {
  // If you escape this loop via break, then you have not found a placeholder.
  // However, if you escape it via "return true", you have.
  while (true) {
    // The easy part: dump till you hit a %
    dumpTillPercentOrEnd(result, fmt);

    // now our cursor is left at a % or a NUL
    char ch = **fmt;
    if (ch == 0) {
      // End of string, and no placeholder found. break.
      break;
    }

    // cursor is at %. Next character is NUL, o (our placeholder), or other char
    ++(*fmt);
    ch = **fmt;
    if (ch == 0) {
      // Trailing %. A mistake? Hmm, just print it. Now at end of string, so break, with no
      // placeholder found.
      result << '%';
      break;
    }

    // Character following % is not NUL, so it is either o (our placeholder), or some other
    // char which should be treated as an "escaped" char. In either case, advance the caller's
    // pointer and then deal with either a placeholder or escaped char.
    ++(*fmt);
    if (ch == 'o') {
      // Found a placeholder!
      return true;
    }

    // escaped char.
    result << ch;
  }
  if (placeholderExpected) {
    result << "[ insufficient placeholders ]";
  }
  return false;
}

pthread_mutex_t Logger::mutex_ = PTHREAD_MUTEX_INITIALIZER;
std::ostream *Logger::fileStream_;
std::string Logger::elidedPrefix_;

void Logger::elidePrefix(const char *file, int numLevelsDeep) {
  const char *end = file + strlen(file);
  while (file != end && numLevelsDeep >= 0) {
    --end;
    if (*end != '/') {
      continue;
    }
    if (numLevelsDeep > 0) {
      --numLevelsDeep;
      continue;
    }
    elidedPrefix_ = std::string(file, end + 1);
    return;
  }
  warn("Got to end without finding an appropriate prefix");
}

void Logger::setThreadPrefix(std::string threadPrefix) {
  threadPrefix_ = std::move(threadPrefix);
}

void Logger::logfSetup(MyOstringStream *buffer) {
  buffer->setf(std::ios::boolalpha);
  const char *fileToUse = file_;
  if (strncmp(file_, elidedPrefix_.data(), elidedPrefix_.size()) == 0) {
    fileToUse += elidedPrefix_.size();
  }
  if (!threadPrefix_.empty()) {
    *buffer << '[' << threadPrefix_ << "]: ";
  }
  *buffer << function_ << "() [" << fileToUse << ':' << line_ << "]: ";
}

void Logger::logfFinish(MyOstringStream *buffer) {
  *buffer << std::endl;

  pthread_mutex_lock(&mutex_);
  std::cout << buffer->str();
#ifndef NDEBUG
  std::cout.flush();
#endif
  if (fileStream_ != nullptr) {
    *fileStream_ << buffer->str();
    fileStream_->flush();
  }
  pthread_mutex_unlock(&mutex_);
}

thread_local std::string Logger::threadPrefix_;

}  // namespace internal

std::ostream &streamf(std::ostream &s, const char *fmt) {
  while (kosak::coding::internal::dumpFormat(s, &fmt, false)) {
    s << "[ extra format placeholder ]";
  }
  return s;
}

}  // namespace kosak::coding
