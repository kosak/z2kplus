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

#pragma once

#include <ostream>
#include <tuple>
#include "kosak/coding/coding.h"
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/sorting/sort_manager.h"
#include "kosak/coding/memory/mapped_file.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/files/keys.h"
#include "z2kplus/backend/util/frozen/frozen_string_pool.h"

namespace z2kplus::backend::reverse_index::builder {
namespace schemas {

class Zephyrgram {
  typedef z2kplus::backend::files::FileKeyKind FileKeyKind;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::Zephyrgram Zgram;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;
  typedef z2kplus::backend::util::frozen::frozenStringRef_t frozenStringRef_t;

  template<FileKeyKind Kind>
  using FileKey = z2kplus::backend::files::FileKey<Kind>;

public:
  // zgramId, timesecs, sender, signature, isLogged
  // instance, body
  // fileKey, offset, size
  typedef std::tuple<ZgramId, uint64_t, std::string_view, std::string_view, bool,
      std::string_view, std::string_view,
      FileKey<FileKeyKind::Either>, uint32_t, uint32_t> tuple_t;

  static tuple_t createTuple(const Zgram &zgram,
      FileKey<FileKeyKind::Either> fileKey, size_t offset, size_t size);

  explicit Zephyrgram(const tuple_t &tuple) : tuple_(tuple) {}

  ZgramId zgramId() const { return std::get<0>(tuple_); }
  uint64_t timesecs() const { return std::get<1>(tuple_); }
  std::string_view sender() const { return std::get<2>(tuple_); }
  std::string_view signature() const { return std::get<3>(tuple_); }
  bool isLogged() const { return std::get<4>(tuple_); }
  std::string_view instance() const { return std::get<5>(tuple_); }
  std::string_view body() const { return std::get<6>(tuple_); }
  FileKey<FileKeyKind::Either> fileKey() const { return std::get<7>(tuple_); }
  uint32_t offset() const { return std::get<8>(tuple_); }
  uint32_t size() const { return std::get<9>(tuple_); }

private:
  const tuple_t &tuple_;
};

class ReactionsByZgramId {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::zgMetadata::Reaction Reaction;
public:
  // zgramId, reaction, creator, wantAdd
  typedef std::tuple<ZgramId, std::string_view, std::string_view, bool> tuple_t;
  static constexpr size_t keySize = 3;

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions::createVector({true, false, false});
  }

  // I need this to false so I can see a remove that may follow an add.
  static constexpr bool keyIsUnique = false;

  static tuple_t createTuple(const Reaction &reaction);

private:
};

class Reactions {
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::zgMetadata::Reaction Reaction;
public:
  // zgramId, reaction, creator
  typedef std::tuple<ZgramId, std::string_view, std::string_view> tuple_t;
  static constexpr bool keyIsUnique = true;
};

class ReactionsByReaction {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::zgMetadata::Reaction Reaction;
public:
  // reaction, zgramId, creator, wantAdd
  typedef std::tuple<std::string_view, ZgramId, std::string_view, bool> tuple_t;
  static constexpr size_t keySize = 3;
  static constexpr bool keyIsUnique = true;

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions ::createVector({false, true, false});
  }

  static tuple_t createTuple(const Reaction &reaction);

private:
};

class ReactionsCounts {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
public:
  // reaction, zgramId, count
  typedef std::tuple<std::string_view, ZgramId, uint32_t> tuple_t;

  static constexpr bool keyIsUnique = false;
  static constexpr size_t keySize = 2;

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions ::createVector({false, true});
  }

private:

};

class ZgramRevisions {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::zgMetadata::ZgramRevision ZgramRevision;
public:
  // zgramId, instance, body, renderStyle
  typedef std::tuple<ZgramId, std::string_view, std::string_view, uint32_t> tuple_t;
  static constexpr bool keyIsUnique = false;
  static constexpr size_t keySize = 1;

  static tuple_t createTuple(const ZgramRevision &zgramRevision);

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions ::createVector({true});
  }

  explicit ZgramRevisions(const tuple_t &tuple) : tuple_(tuple) {}

  ZgramId zgramId() const { return std::get<0>(tuple_); }
  std::string_view instance() const { return std::get<1>(tuple_); }
  std::string_view body() const { return std::get<2>(tuple_); }

private:
  const tuple_t &tuple_;
};

class ZgramRefersTos {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::zgMetadata::ZgramRefersTo ZgramRefersTo;
public:
  // zgramId, refersTo, valid
  typedef std::tuple<ZgramId, ZgramId, bool> tuple_t;
  static constexpr bool keyIsUnique = false;
  static constexpr size_t keySize = 2;

  static tuple_t createTuple(const ZgramRefersTo &refersTo);

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions ::createVector({true, true});
  }

  explicit ZgramRefersTos(const tuple_t &tuple) : tuple_(tuple) {}

  ZgramId zgramId() const { return std::get<0>(tuple_); }
  ZgramId refersTo() const { return std::get<1>(tuple_); }
  bool valid() const { return std::get<2>(tuple_); }

private:
  const tuple_t &tuple_;
};

class ZmojisRevisions {
  typedef kosak::coding::sorting::KeyOptions KeyOptions;
  typedef z2kplus::backend::shared::userMetadata::Zmojis Zmojis;

public:
  // userid -> emojis
  typedef std::tuple<std::string_view, std::string_view> tuple_t;
  static constexpr size_t keySize = 1;
  // False because later zmojis override earlier ones
  static constexpr bool keyIsUnique = false;

  static std::vector<KeyOptions> keyOptions() {
    return KeyOptions::createVector({false});
  }

  static tuple_t createTuple(const Zmojis &isLoggedRevision);

  explicit ZmojisRevisions(const tuple_t &tuple) : tuple_(tuple) {}

  std::string_view userId() const { return std::get<0>(tuple_); }
  std::string_view zmojis() const { return std::get<1>(tuple_); }

private:
  const tuple_t &tuple_;
};

class PlusPluses {
  typedef z2kplus::backend::shared::ZgramId ZgramId;
public:
  typedef std::tuple<std::string_view, ZgramId> tuple_t;
  static constexpr size_t keySize = 2;
  static constexpr bool keyIsUnique = true;

  explicit PlusPluses(const tuple_t &tuple) : tuple_(tuple) {}

  std::string_view key() const { return std::get<0>(tuple_); }
  ZgramId zgramId() const { return std::get<1>(tuple_); }

private:
  const tuple_t &tuple_;
};

class PlusPlusKeys {
  typedef z2kplus::backend::shared::ZgramId ZgramId;
public:
  typedef std::tuple<ZgramId, std::string_view> tuple_t;
  static constexpr size_t keySize = 2;
  static constexpr bool keyIsUnique = true;

  explicit PlusPlusKeys(const tuple_t &tuple) : tuple_(tuple) {}

  ZgramId zgramId() const { return std::get<0>(tuple_); }
  std::string_view key() const { return std::get<1>(tuple_); }

private:
  const tuple_t &tuple_;
};
}  // namespace schemas
}  // namespace z2kplus::backend::reverse_index::builder
