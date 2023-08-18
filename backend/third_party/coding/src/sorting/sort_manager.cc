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

#include "kosak/coding/sorting/sort_manager.h"

#include <fcntl.h>
#include <string_view>
#include "kosak/coding/dumping.h"
#include "kosak/coding/unix.h"

using kosak::coding::FailFrame;
using kosak::coding::nsunix::FileCloser;
namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace kosak::coding::sorting {
std::vector<KeyOptions> KeyOptions::createVector(std::initializer_list<bool> numericFlags) {
  std::vector<KeyOptions> result;
  // Flags are 1-based.
  size_t nextIndex = 1;
  for (auto flag : numericFlags) {
    result.emplace_back(nextIndex++, flag);
  }
  return result;
}
std::string KeyOptions::makeOptionText() const {
  const auto *optionalNumeric = numeric_ ? "n" : "";
  return stringf("%o%o,%o%o", oneBasedIndex_, optionalNumeric, oneBasedIndex_, optionalNumeric);
}

bool SortManager::trySort(const SortOptions &sortOptions, const std::vector<KeyOptions> &keyOptions,
    std::vector<std::string> inputPaths, std::string outputPath, const FailFrame &ff) {
  SortManager sm;
  return tryCreate(sortOptions, keyOptions, std::move(inputPaths), std::move(outputPath), &sm,
      ff.nest(HERE)) &&
    sm.tryFinish(ff.nest(HERE));
}

bool SortManager::tryCreate(const SortOptions &sortOptions, const std::vector<KeyOptions> &keyOptions,
    std::vector<std::string> inputPaths, std::string outputPath, SortManager *result,
    const FailFrame &ff) {
  if (outputPath.empty()) {
    return ff.fail(HERE, "WHY");
  }

  static char sortExecutable[] = "/usr/bin/sort";

  std::vector<std::string> args;
  // argv[0] is the name of the program
  args.emplace_back(sortExecutable);
  if (sortOptions.unique_) {
    args.emplace_back("--unique");
  }
  if (sortOptions.stable_) {
    args.emplace_back("--stable");
  }
  args.emplace_back("--field-separator");
  args.emplace_back(&sortOptions.fieldSeparator_, 1);
  if (sortOptions.lineSeparatorIsNul_) {
    args.emplace_back("--zero-terminated");
  }

  for (const auto &ko : keyOptions) {
    args.emplace_back("--key");
    args.push_back(ko.makeOptionText());
  }
  args.emplace_back("--output");
  args.emplace_back(std::move(outputPath));

  args.insert(args.end(), std::make_move_iterator(inputPaths.begin()),
      std::make_move_iterator(inputPaths.end()));

  std::vector<std::string> envs = { "LC_ALL=C" };
  warn("%o %o\nenv: %o", sortExecutable,
      kosak::coding::dump(args.begin(), args.end(), "", "", " "),
      kosak::coding::dump(envs.begin(), envs.end(), "", "", ", "));

  pid_t pid;
  if (!nsunix::tryFork(&pid, ff.nest(HERE))) {
    return false;
  }

  if (pid != 0) {
    // I am the parent
    *result = SortManager(pid);
    return true;
  }

  // I am the child.
  FailRoot childRoot;
  if (!nsunix::tryExecve(sortExecutable, args, envs, childRoot.nest(HERE))) {
    crash("Child failed: %o", childRoot);
  }
  notreached;
}

SortManager::SortManager() = default;
SortManager::SortManager(pid_t childPid) : childPid_(childPid) {}
SortManager::SortManager(SortManager &&other) noexcept : childPid_(other.childPid_) {
  other.childPid_ = 0;
}
SortManager &SortManager::operator=(SortManager &&other) noexcept {
  childPid_ = other.childPid_;
  other.childPid_ = 0;
  return *this;
}
SortManager::~SortManager() {
  if (childPid_ != 0) {
    warn("*** Did you forget to call SortManager::tryFinish?");
  }
}

bool SortManager::tryFinish(const FailFrame &ff) {
  if (childPid_ == 0) {
    return ff.fail(HERE, "Can't finish. I don't have a child.");
  }
  auto temp = childPid_;
  childPid_ = 0;
  pid_t pidResult;
  int status;
  return nsunix::tryWaitPid(temp, &status, 0, &pidResult, ff.nest(HERE));
}
}  // namespace kosak::coding::sorting
