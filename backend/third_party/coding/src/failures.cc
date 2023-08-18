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

#include "kosak/coding/failures.h"

namespace kosak::coding {
FailRoot::FailRoot() : FailFrame(this, nullptr) {}
FailRoot::FailRoot(bool quiet) : FailFrame(this, nullptr), quiet_(quiet) {}
FailRoot::~FailRoot() = default;

bool FailFrame::fail(const char *func, const char *file, int line, std::string_view s) const {
  maybeAddSeparator();
  if (!root_->quiet()) {
    root_->oss_ << s;
  }
  setFailFlagAndMaybeAddStackTrace(func, file, line);
  return false;
}

void FailFrame::maybeAddSeparator() const {
  if (!root_->ok()) {
    root_->oss_ << '\n';
  }
}

void FailFrame::setFailFlagAndMaybeAddStackTrace(const char *func, const char *file,
    int line) const {
  if (root_->ok() && !root_->quiet()) {
    auto temp = nest(func, file, line);

    for (const FailFrame *p = &temp; p != nullptr; p = p->next_) {
      root_->oss_ << '\n';
      p->streamFrameTo(root_->oss_);
    }
  }

  root_->ok_ = false;
}

void FailRoot::streamFrameTo(std::ostream &s) const {
  // do nothing.
}

namespace internal {
void FileLineFailFrame::streamToHelper(std::ostream &s) const {
  streamf(s, "%o %o:%o", func_, file_, line_);
}

void SimpleFailFrame::streamFrameTo(std::ostream &s) const {
  streamToHelper(s);
}

void DelegateFailFrame::streamFrameTo(std::ostream &s) const {
  streamToHelper(s);
  s << ": ";
  cb_(s);
}
}  // namespace internal
}  // namespace kosak::coding
