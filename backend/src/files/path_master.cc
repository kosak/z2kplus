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

#include "z2kplus/backend/files/path_master.h"

#include <string_view>
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/files/keys.h"

namespace z2kplus::backend::files {

using kosak::coding::Delegate;
using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::text::tryParseDecimal;
namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace {
bool tryGetPlaintextsHelper(const std::string &root, bool expectLogged,
    const Delegate<bool, const FileKey &, const FailFrame &> &cb, const FailFrame &ff);
bool tryParseRestrictedDecimal(const char *humanReadable, std::string_view src,
    std::string_view expectedPrefix, size_t beginValue, size_t endValue, size_t *result,
    std::string_view *residual, const FailFrame &ff);
bool tryConsume(std::string_view src, std::string_view prefix, std::string_view *residual,
    const FailFrame &ff);
}  // namespace

const char PathMaster::z2kIndexName[] = "z2k.index";

bool PathMaster::tryCreate(std::string root, std::shared_ptr<PathMaster> *result,
    const FailFrame &ff) {
  if (root.empty() || root.back() != '/') {
    root.push_back('/');
  }

  auto loggedRoot = root + "logged/";
  auto unloggedRoot = root + "unlogged/";
  auto indexRoot = root + "index/";
  auto scratchRoot = root + "scratch/";
  auto mediaRoot = root + "media/";

  auto mkdirIfNotExists = [](const std::string &name, const FailFrame &f2) {
    bool exists;
    if (!nsunix::tryExists(name, &exists, f2.nest(HERE))) {
      return false;
    }
    auto mode = S_IRWXU | S_IRGRP | S_IXGRP;
    return exists || nsunix::tryMakeDirectory(name, mode, f2.nest(HERE));
  };
  if (!mkdirIfNotExists(loggedRoot, ff.nest(HERE)) ||
      !mkdirIfNotExists(unloggedRoot, ff.nest(HERE)) ||
      !mkdirIfNotExists(indexRoot, ff.nest(HERE)) ||
      !mkdirIfNotExists(scratchRoot, ff.nest(HERE)) ||
      !mkdirIfNotExists(mediaRoot, ff.nest(HERE))) {
    return false;
  }

  *result = std::make_shared<PathMaster>(Private(), std::move(loggedRoot),
      std::move(unloggedRoot), std::move(indexRoot), std::move(scratchRoot),
      std::move(mediaRoot));
  return true;
}

PathMaster::PathMaster(Private, std::string loggedRoot, std::string unloggedRoot,
    std::string indexRoot, std::string scratchRoot, std::string mediaRoot) :
    loggedRoot_(std::move(loggedRoot)),
    unloggedRoot_(std::move(unloggedRoot)), indexRoot_(std::move(indexRoot)),
    scratchRoot_(std::move(scratchRoot)), mediaRoot_(std::move(mediaRoot)) {}
PathMaster::~PathMaster() = default;

std::string PathMaster::getPlaintextPath(const FileKey &fileKey) const {
  auto efk = fileKey.asExpandedFileKey();
  auto result = efk.isLogged() ? loggedRoot_ : unloggedRoot_;

  // yyyy/mm/yyyymmdd.logged.nnn
  // yyyy/mm/yyyymmdd.unlogged.nnn
  char buffer[128];
  snprintf(buffer, STATIC_ARRAYSIZE(buffer), "%04zu/%02zu/%04zu%02zu%02zu.%s.%03zu",
      efk.year(), efk.month(), efk.year(), efk.month(), efk.day(),
      efk.isLogged() ? "logged" : "unlogged", efk.part());

  result.append(buffer);
  return result;
}

std::string PathMaster::getIndexPath() const {
  return indexRoot_ + z2kIndexName;
}

std::string PathMaster::getScratchIndexPath() const {
  return scratchRoot_ + z2kIndexName;
}

std::string PathMaster::getScratchPathFor(std::string_view name) const {
  return scratchRoot_ + std::string(name);
}

bool PathMaster::tryGetPlaintexts(
    const Delegate<bool, const FileKey &, const FailFrame &> &cb, const FailFrame &ff) const {
  return tryGetPlaintextsHelper(loggedRoot_, true, cb, ff.nest(HERE)) &&
      tryGetPlaintextsHelper(unloggedRoot_, false, cb, ff.nest(HERE));
}

bool PathMaster::tryPublishBuild(const FailFrame &ff) const {
  auto src = getScratchIndexPath();
  auto dest = getIndexPath();
  return nsunix::tryRename(src, dest, ff.nest(HERE));
}

namespace {
bool tryGetPlaintextsHelper(const std::string &root, bool expectLogged,
    const Delegate<bool, const FileKey &, const FailFrame &> &cb, const FailFrame &ff) {
  // example: 2000/01/20000104.unlogged.000
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
    size_t year, month, yyyyMMdd, part;

    using FKC = internal::FileKeyCore;
    // 19700101
    auto yyyyMMddBegin = (((FKC::yearBegin) * 100) + FKC::monthBegin) * 100 + FKC::dayBegin;
    // 21010000
    auto yyyyMMddEnd = FKC::yearEnd * 100 * 100;
    if (!tryParseRestrictedDecimal("year", suffix, "", FKC::yearBegin, FKC::yearEnd,
            &year, &yearRes, f3.nest(HERE)) ||
        !tryParseRestrictedDecimal("month", yearRes, "/", FKC::monthBegin, FKC::monthEnd,
            &month, &monthRes, f3.nest(HERE)) ||
        !tryParseRestrictedDecimal("yyyyMMdd", monthRes, "/", yyyyMMddBegin, yyyyMMddEnd, &yyyyMMdd,
            &yyyyMMddRes, f3.nest(HERE))) {
      return false;
    }

    bool logged;
    std::string_view loggedRes;
    FailRoot ignore(true);
    if (tryConsume(yyyyMMddRes, ".logged", &loggedRes, ignore.nest(HERE))) {
      logged = true;
    } else if (tryConsume(yyyyMMddRes, ".unlogged", &loggedRes, ignore.nest(HERE))) {
      logged = false;
    } else {
      return f3.failf(HERE, "Can't find logged/unlogged indicator in %o", fullName);
    }

    if (expectLogged != logged) {
      return f3.failf(HERE, "Expected this directory to have logged=%o. Got logged=%o", expectLogged, logged);
    }

    std::string_view expectedEmpty;
    if (!tryParseRestrictedDecimal("part", loggedRes, ".", FKC::partBegin, FKC::partEnd, &part,
        &expectedEmpty, f3.nest(HERE))) {
      return false;
    }

    if (!expectedEmpty.empty()) {
      return f3.failf(HERE, R"(Trailing matter "%o" found, was supposed to be empty)", expectedEmpty);
    }

    auto day = yyyyMMdd % 100;
    auto reconstructed = (year * 100 + month) * 100 + day;
    if (yyyyMMdd != reconstructed) {
      return f3.failf(HERE, "Subdir parts inconsistent; got %o vs %o in %o", yyyyMMdd, reconstructed,
          fullName);
    }
    FileKey fk;
    if (!FileKey::tryCreate(year, month, day, part, logged, &fk, f3.nest(HERE))) {
      return false;
    }
    return cb(fk, f3.nest(HERE));
  };
  return nsunix::tryEnumerateFilesAndDirsRecursively(root, &myCallback, ff.nest(HERE));
}

bool tryParseRestrictedDecimal(const char *humanReadable, std::string_view src,
    std::string_view expectedPrefix, size_t beginValue, size_t endValue, size_t *result,
    std::string_view *residual, const FailFrame &ff) {
  std::string_view temp;
  if (!tryConsume(src, expectedPrefix, &temp, ff.nest(HERE)) ||
      !tryParseDecimal(temp, result, residual, ff.nest(HERE))) {
    return false;
  }
  if (*result < beginValue || *result >= endValue) {
    return ff.failf(HERE, "Expected %o in the range [%o..%o), got %o", humanReadable, beginValue,
        endValue, *result);
  }
  return true;
}

bool tryConsume(std::string_view src, std::string_view prefix, std::string_view *residual,
    const FailFrame &ff) {
  if (src.size() < prefix.size() ||
      src.substr(0, prefix.size()) != prefix) {
    return ff.failf(HERE, R"("%o" does not start with prefix "%o")", src, prefix);
  }
  *residual = src.substr(prefix.size());
  return true;
}
}  // namespace
}  // namespace z2kplus::backend::files
