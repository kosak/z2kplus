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

#include <clocale>
#include <ctime>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/memory/mapped_file.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/builder/index_builder.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/shared/magic_constants.h"

using kosak::coding::Delegate;
using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::memory::MappedFile;
using kosak::coding::streamf;
using kosak::coding::text::tryParseDecimal;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::reverse_index::builder::IndexBuilder;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::ZgramInfo;
using z2kplus::backend::server::Server;
using z2kplus::backend::shared::Zephyrgram;

namespace nsunix = kosak::coding::nsunix;
namespace magicConstants = z2kplus::backend::shared::magicConstants;

#define HERE KOSAK_CODING_HERE

namespace {
struct OldFileKey {
  OldFileKey(size_t year, size_t month, size_t day, size_t part, bool isLogged) :
      year_(year), month_(month), day_(day), part_(part), isLogged_(isLogged) {}

  size_t year_ = 0;
  size_t month_ = 0;
  size_t day_ = 0;
  size_t part_ = 0;
  bool isLogged_ = false;

  friend bool operator<(const OldFileKey &lhs, const OldFileKey &rhs) {
    auto diff = kosak::coding::compare(&lhs.year_, &rhs.year_);
    if (diff != 0) return diff < 0;
    diff = kosak::coding::compare(&lhs.month_, &rhs.month_);
    if (diff != 0) return diff < 0;
    diff = kosak::coding::compare(&lhs.day_, &rhs.day_);
    if (diff != 0) return diff < 0;
    diff = kosak::coding::compare(&lhs.part_, &rhs.part_);
    if (diff != 0) return diff < 0;
    diff = kosak::coding::compare(&lhs.isLogged_, &rhs.isLogged_);
    return diff < 0;
  }
};
bool tryRun(int argc, char **argv, const FailFrame &ff);

bool tryGetLegacyPlaintexts(const std::string &loggedRoot, const std::string &unloggedRoot,
    const Delegate<bool, OldFileKey, const FailFrame &> &cb, const FailFrame &ff);
bool tryGetLegacyPlaintextsHelper(const std::string &root, bool expectLogged,
    const Delegate<bool, OldFileKey, const FailFrame &> &cb, const FailFrame &ff);
bool tryParseRestrictedDecimal(const char *humanReadable, std::string_view src,
    std::string_view expectedPrefix, size_t beginValue, size_t endValue, size_t *result,
    std::string_view *residual, const FailFrame &ff);
bool maybeConsume(std::string_view src, std::string_view prefix, std::string_view *residual);
}  // namespace

int main(int argc, char **argv) {
  kosak::coding::internal::Logger::elidePrefix(__FILE__, 0);

  FailRoot fr;
  if (!tryRun(argc, argv, fr.nest(HERE))) {
    streamf(std::cerr, "Failed: %o\n", fr);
    exit(1);
  }
}

namespace {
bool tryRun(int argc, char **argv, const FailFrame &ff) {
  if (argc != 2) {
    return ff.failf(HERE, "Expected 1 arguments: fileRoot");
  }

  std::shared_ptr<PathMaster> pm;
  std::vector<OldFileKey> fileKeys;
  auto cb = [&fileKeys](OldFileKey fk, const FailFrame &) {
    fileKeys.push_back(fk);
    return true;
  };
  if (!PathMaster::tryCreate(argv[1], &pm, ff.nest(HERE)) ||
     !tryGetLegacyPlaintexts(pm->loggedRoot(), pm->unloggedRoot(), &cb, ff.nest(HERE))) {
    return false;
  }
  std::sort(fileKeys.begin(), fileKeys.end());

  std::map<OldFileKey, std::vector<OldFileKey>> grouped;
  for (const auto &fk : fileKeys) {
    OldFileKey canonical(fk.year_, fk.month_, fk.day_, 0, fk.isLogged_);
    grouped[canonical].push_back(fk);
  }

  for (const auto &[group, keys] : grouped) {
    FileKey<FileKeyKind::Either> destFk;
    if (!FileKey<FileKeyKind::Either>::tryCreate(group.year_, group.month_, group.day_, group.isLogged_,
        &destFk, ff.nest(HERE))) {
      return false;
    }
    auto srcPrefix = pm->getPlaintextPath(destFk);
    auto destFilename = pm->getPlaintextPath(destFk);

    std::string allContents;
    for (const auto &key : keys) {
      char fakeSuffix[32];
      snprintf(fakeSuffix, sizeof(fakeSuffix), ".%03d", (int)key.part_);
      auto srcFilename = srcPrefix + fakeSuffix;

      std::string contents;
      if (!nsunix::tryReadAll(srcFilename, &contents, ff.nest(HERE))) {
        return false;
      }
      allContents += contents;
    }
    if (!nsunix::tryWriteAll(destFilename, allContents, ff.nest(HERE))) {
      return false;
    }
  }

  return true;
}

bool tryGetLegacyPlaintexts(const std::string &loggedRoot, const std::string &unloggedRoot,
    const Delegate<bool, OldFileKey, const FailFrame &> &cb, const FailFrame &ff) {
  return tryGetLegacyPlaintextsHelper(loggedRoot, true, cb, ff.nest(HERE)) &&
      tryGetLegacyPlaintextsHelper(unloggedRoot, false, cb, ff.nest(HERE));
}

bool tryGetLegacyPlaintextsHelper(const std::string &root, bool expectLogged,
    const Delegate<bool, OldFileKey, const FailFrame &> &cb, const FailFrame &ff) {
  // example: 2000/01/20000104.unlogged
  auto myCallback = [expectLogged, &cb](std::string_view fullName, bool isDir, const FailFrame &f2) {
    if (isDir) {
      return true;
    }
    auto contextCb = [fullName](std::ostream &s) {
      s << "While processing " << fullName;
    };
    auto f3 = f2.nestWithDelegate(HERE, &contextCb);

    size_t pos = fullName.size();
    for (size_t i = 0; i < 3; ++i) {
      if (pos == 0) {
        return f3.failf(HERE, "Ran off the front of: %o", fullName);
      }
      pos = fullName.find_last_of('/', pos - 1);
      if (pos == std::string_view::npos) {
        return f3.failf(HERE, "This pathname does not have enough trailing pieces for me to parse: %o", fullName);
      }
    }
    auto suffix = fullName.substr(pos + 1);
    std::string_view yearRes, monthRes, yyyyMMddRes;
    size_t year, month, yyyyMMdd;

    if (!tryParseRestrictedDecimal("year", suffix, "", 1970, 2100 + 1,
        &year, &yearRes, f3.nest(HERE)) ||
        !tryParseRestrictedDecimal("month", yearRes, "/", 1, 12 + 1,
            &month, &monthRes, f3.nest(HERE)) ||
        !tryParseRestrictedDecimal("yyyyMMdd", monthRes, "/", 19700101, 21001231 + 1,
            &yyyyMMdd, &yyyyMMddRes, f3.nest(HERE))) {
      return false;
    }

    bool logged;
    std::string_view loggedRes;
    if (maybeConsume(yyyyMMddRes, ".logged", &loggedRes)) {
      logged = true;
    } else if (maybeConsume(yyyyMMddRes, ".unlogged", &loggedRes)) {
      logged = false;
    } else {
      return f3.failf(HERE, "Can't find logged/unlogged indicator in %o", fullName);
    }

    if (expectLogged != logged) {
      return f3.failf(HERE, "Expected this directory to have logged=%o. Got logged=%o", expectLogged, logged);
    }

    size_t part;
    std::string_view residual;
    if (!tryParseRestrictedDecimal("part", loggedRes, ".", 0, 1000, &part, &residual, f3.nest(HERE))) {
      return false;
    }

    if (!residual.empty()) {
      return f3.failf(HERE, R"(Trailing matter "%o" found, was supposed to be empty)", loggedRes);
    }

    auto day = yyyyMMdd % 100;
    auto reconstructed = (year * 100 + month) * 100 + day;
    if (yyyyMMdd != reconstructed) {
      return f3.failf(HERE, "Subdir parts inconsistent; got %o vs %o in %o", yyyyMMdd, reconstructed,
          fullName);
    }

    auto fk = OldFileKey(year, month, day, part, logged);
    return cb(fk, f3.nest(HERE));
  };
  return nsunix::tryEnumerateFilesAndDirsRecursively(root, &myCallback, ff.nest(HERE));
}

bool tryParseRestrictedDecimal(const char *humanReadable, std::string_view src,
    std::string_view expectedPrefix, size_t beginValue, size_t endValue, size_t *result,
    std::string_view *residual, const FailFrame &ff) {
  std::string_view temp;
  if (!maybeConsume(src, expectedPrefix, &temp)) {
    return ff.failf(HERE, "%o did not start with %o", src, expectedPrefix);
  }
  if (!tryParseDecimal(temp, result, residual, ff.nest(HERE))) {
    return false;
  }
  if (*result < beginValue || *result >= endValue) {
    return ff.failf(HERE, "Expected %o in the range [%o..%o), got %o", humanReadable, beginValue,
        endValue, *result);
  }
  return true;
}

bool maybeConsume(std::string_view src, std::string_view prefix, std::string_view *residual) {
  if (src.size() < prefix.size() ||
      src.substr(0, prefix.size()) != prefix) {
    return false;
  }
  *residual = src.substr(prefix.size());
  return true;
}
}  // namespace
