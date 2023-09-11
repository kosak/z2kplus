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

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "z2kplus/backend/coordinator/subscription.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/plusplus_scanner.h"
#include "z2kplus/backend/shared/protocol/misc.h"
#include "z2kplus/backend/shared/protocol/message/drequest.h"
#include "z2kplus/backend/shared/protocol/message/dresponse.h"
#include "z2kplus/backend/shared/zephyrgram.h"

// The Coordinator takes incoming requests and processes them. It also manages subscriptions and
// periodic tasks such as index rebuilds. It also owns the ConsolidatedIndex. Requests to
// dynamically add new zgrams or metadata have to go through the Coordinator. It also owns the log
// files. When incoming zephyrgrams and metadata are submitted here, they get appended to the
// currently-open log file. In addition, incoming zephyrgrams are assigned their permanent
// IDs.

namespace z2kplus::backend::coordinator {
namespace internal {
struct SubComparer {
  using is_transparent = std::true_type;

  bool operator()(const std::shared_ptr<Subscription> &lhs,
      const std::shared_ptr<Subscription> &rhs) const {
    return lhs.get() < rhs.get();
  }

  bool operator()(const std::shared_ptr<Subscription> &lhs, const Subscription *rhs) const {
    return lhs.get() < rhs;
  }

  bool operator()(const Subscription *lhs, const std::shared_ptr<Subscription> &rhs) const {
    return lhs < rhs.get();
  }
};
}  // namespace internal

class Coordinator {
public:
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::DateAndPartKey DateAndPartKey;
  typedef z2kplus::backend::files::FileKey FileKey;
  typedef z2kplus::backend::files::Location Location;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;
  typedef z2kplus::backend::reverse_index::iterators::ZgramIterator ZgramIterator;
  typedef z2kplus::backend::reverse_index::zgramOff_t zgramOff_t;
  typedef z2kplus::backend::shared::protocol::message::drequests::CheckSyntax CheckSyntax;
  typedef z2kplus::backend::shared::protocol::message::drequests::GetMoreZgrams GetMoreZgrams;
  typedef z2kplus::backend::shared::protocol::message::drequests::GetSpecificZgrams GetSpecificZgrams;
  typedef z2kplus::backend::shared::protocol::message::drequests::Ping Ping;
  typedef z2kplus::backend::shared::protocol::message::drequests::PostMetadata PostMetadata;
  typedef z2kplus::backend::shared::protocol::message::drequests::PostZgrams PostZgrams;
  typedef z2kplus::backend::shared::protocol::message::drequests::Subscribe Subscribe;
  typedef z2kplus::backend::shared::LogRecord LogRecord;
  typedef z2kplus::backend::shared::MetadataRecord MetadataRecord;
  typedef z2kplus::backend::shared::PlusPlusScanner PlusPlusScanner;
  typedef z2kplus::backend::shared::Profile Profile;
  typedef z2kplus::backend::shared::Zephyrgram Zephyrgram;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::protocol::message::dresponses::AckSubscribe AckSubscribe;

  template<typename R, typename ...Args>
  using Delegate = kosak::coding::Delegate<R, Args...>;

  // pm - PathMaster knows where all of our configured path names are in the filesystem.
  // ci - The ConsolidatedIndex
  // pageSize - we give results back in pages of this size
  // lookBeyondCount - Our limit of how far we look beyond the query to estimate to the user how
  //   many more zgrams there are (for example if lookBeyoundCount is 5, our answers will be
  //   "3", "4", "5", "5+" etc).
  static bool tryCreate(std::shared_ptr<PathMaster> pm, ConsolidatedIndex ci, Coordinator *result,
      const FailFrame &ff);
  Coordinator();
  DISALLOW_COPY_AND_ASSIGN(Coordinator);
  DECLARE_MOVE_COPY_AND_ASSIGN(Coordinator);
  ~Coordinator();

private:
  Coordinator(std::shared_ptr<PathMaster> &&pm, ConsolidatedIndex &&ci);

public:
  typedef std::pair<Subscription*, DResponse> response_t;

  void subscribe(std::shared_ptr<Profile> profile, Subscribe &&req, std::vector<response_t> *responses,
      std::shared_ptr<Subscription> *possibleNewSub);

  void unsubscribe(Subscription *sub, std::vector<response_t> *responses);

  void checkSyntax(Subscription *sub, CheckSyntax &&cs, std::vector<response_t> *responses);
  void getMoreZgrams(Subscription *sub, GetMoreZgrams &&o, std::vector<response_t> *responses);

  void postZgrams(Subscription *sub, std::chrono::system_clock::time_point now, PostZgrams &&o,
      std::vector<response_t> *responses);
  void postMetadata(Subscription *sub, PostMetadata &&o, std::vector<response_t> *responses);

  bool tryPostZgramsNoSub(const Profile &profile, std::chrono::system_clock::time_point now,
      PostZgrams &&o, std::vector<response_t> *responses, const FailFrame &ff);
  bool tryPostMetadataNoSub(const Profile &profile, PostMetadata &&o, std::vector<response_t> *responses,
      const FailFrame &ff);

  void getSpecificZgrams(Subscription *sub, GetSpecificZgrams &&o, std::vector<response_t> *responses);

  void ping(Subscription *sub, Ping &&o, std::vector<response_t> *responses);

  bool tryCheckpoint(std::chrono::system_clock::time_point now, DateAndPartKey *endKey,
        const FailFrame &ff) {
    return index_.tryCheckpoint(now, endKey, ff.nest(KOSAK_CODING_HERE));
  }

  bool tryResetIndex(std::chrono::system_clock::time_point now, const FailFrame &ff);

  const std::shared_ptr<PathMaster> &pathMaster() const { return pathMaster_; }

  const ConsolidatedIndex &index() const { return index_; }

private:
  void notifySubscribersAboutMetadata(std::vector<MetadataRecord> &&metadata,
      std::vector<response_t> *responses);
  void notifySubscribersAboutPpChanges(const ConsolidatedIndex::ppDeltaMap_t &deltaMap,
      std::vector<response_t> *responses);
  void notifySubscribersAboutEstimates(std::vector<response_t> *responses);

  bool trySanitize(const Profile &profile, std::vector<MetadataRecord> *records,
      const FailFrame &ff);

  std::shared_ptr<PathMaster> pathMaster_;
  ConsolidatedIndex index_;
  std::set<std::shared_ptr<Subscription>, internal::SubComparer> subscriptions_;
};
}  // namespace z2kplus::backend::coordinator
