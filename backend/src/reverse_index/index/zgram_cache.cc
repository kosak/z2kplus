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

#include "z2kplus/backend/reverse_index/index/zgram_cache.h"

#include <string_view>
#include <tuple>
#include <utility>
#include "kosak/coding/coding.h"
#include "kosak/coding/memory/mapped_file.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FailFrame;
using kosak::coding::memory::MappedFile;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::Location;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::Zephyrgram;

using z2kplus::backend::factories::LogParser;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::reverse_index::index {
ZgramCache::ZgramCache() = default;
ZgramCache::ZgramCache(size_t capacity) : capacity_(capacity) {}
ZgramCache::ZgramCache(ZgramCache &&) noexcept = default;
ZgramCache& ZgramCache::operator=(ZgramCache &&) noexcept = default;
ZgramCache::~ZgramCache() = default;

bool ZgramCache::tryLookupOrResolve(const PathMaster &pm,
  const std::vector<std::pair<ZgramId, Location>> &locators,
    std::vector<std::shared_ptr<const Zephyrgram>> *result, const FailFrame &ff) {
  // Do a little extra work to make us append to the result rather than overwrite it.
  auto offset = result->size();
  result->resize(offset + locators.size());

  std::vector<std::tuple<ZgramId, Location, size_t>> todo;
  todo.reserve(locators.size());

  // First, populate what we can from the cache.
  for (size_t i = 0; i < locators.size(); ++i) {
    const auto &locator = locators[i];
    const auto &zgramId = locator.first;
    const auto &location = locator.second;
    const auto &cp = cache_.find(zgramId);
    if (cp == cache_.end()) {
      todo.emplace_back(zgramId, location, i);
      continue;
    }
    (*result)[offset + i] = cp->second;
  }
  // sort 'todo' by Location so that file accesses are more efficient.
  std::sort(todo.begin(), todo.end(), [](const auto &lhs, const auto &rhs) {
    return std::get<1>(lhs) < std::get<1>(rhs);
  });

  MappedFile<const char> currentFile;
  FileKey currentFileKey;
  for (const auto &[zgramId, location, index] : todo) {
    // If first time, or the file key is different from the file we have open, open a new file.
    if (currentFile.get() == nullptr || currentFileKey != location.fileKey()) {
      currentFileKey = location.fileKey();
      auto fileName = pm.getPlaintextPath(currentFileKey);
      if (!currentFile.tryMap(fileName, false, ff.nest(HERE))) {
        return false;
      }
    }

    if (location.position() + location.size() > currentFile.byteSize()) {
      return ff.failf(HERE, "What happened? pos %o size %o cf.size %o",
          location.position(), location.size(), currentFile.byteSize());
    }
    std::string_view sr(currentFile.get() + location.position(), location.size());
    LogRecord lr;
    if (!LogParser::tryParseLogRecord(sr, &lr, ff.nest(HERE))) {
      return false;
    }
    auto *zg = std::get_if<Zephyrgram>(&lr.payload());
    if (zg == nullptr) {
      return ff.failf(HERE, "Location %o does not refer to a zgram", location);
    }
    auto sharedZg = std::make_shared<Zephyrgram>(std::move(*zg));
    if (cache_.size() < capacity_) {
      cache_.try_emplace(zgramId, sharedZg);
    } else if (!cache_.empty() && cache_.begin()->first < zgramId) {
      auto node = cache_.extract(cache_.begin());
      node.key() = sharedZg->zgramId();
      node.mapped() = sharedZg;
      cache_.insert(std::move(node));
    }
    (*result)[offset + index] = std::move(sharedZg);
  }
  return true;
}
}  // namespace z2kplus::backend::reverse_index::index
