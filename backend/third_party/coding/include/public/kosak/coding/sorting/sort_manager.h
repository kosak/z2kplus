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

#include <optional>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/unix.h"
#include "kosak/coding/memory/buffered_writer.h"

namespace kosak::coding::sorting {
struct SortOptions {
  SortOptions(bool stable, bool unique, char fieldSeparator, bool lineSeparatorIsNul) :
      stable_(stable), unique_(unique), fieldSeparator_(fieldSeparator),
      lineSeparatorIsNul_(lineSeparatorIsNul) {}

  bool stable_ = false;
  bool unique_ = false;
  char fieldSeparator_ = 0;
  bool lineSeparatorIsNul_ = false;
};

struct KeyOptions {
  static std::vector<KeyOptions> createVector(std::initializer_list<bool> numericFlags);

  constexpr KeyOptions() = default;
  constexpr KeyOptions(size_t oneBasedIndex, bool numeric) : oneBasedIndex_(oneBasedIndex), numeric_(numeric) {}

  std::string makeOptionText() const;

  size_t oneBasedIndex_ = 1;
  bool numeric_ = false;
};

class SortManager {
  typedef kosak::coding::FailFrame FailFrame;

public:
  // Convenience method that combines tryCreate and tryFinish.
  static bool trySort(const SortOptions &sortOptions, const std::vector<KeyOptions> &keyOptions,
      std::vector<std::string> inputPaths, std::string outputPath, const FailFrame &ff);

  static bool tryCreate(const SortOptions &sortOptions, const std::vector<KeyOptions> &keyOptions,
      std::vector<std::string> inputPaths, std::string outputPath, SortManager *result, const FailFrame &ff);

  SortManager();
  DISALLOW_COPY_AND_ASSIGN(SortManager);
  DECLARE_MOVE_COPY_AND_ASSIGN(SortManager);
  ~SortManager();

  bool tryFinish(const FailFrame &ff);

private:
  explicit SortManager(pid_t childPid);

  pid_t childPid_ = 0;
};
}  // namespace kosak::coding::sorting
