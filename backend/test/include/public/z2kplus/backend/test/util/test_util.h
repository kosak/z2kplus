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

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"
#include "fake_frontend.h"
#include "z2kplus/backend/communicator/communicator.h"
#include "z2kplus/backend/coordinator/coordinator.h"
#include "z2kplus/backend/factories/log_parser.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/index/frozen_index.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/server/server.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace z2kplus::backend::test::util {
struct TestUtil {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::CompressedFileKey CompressedFileKey;
  typedef z2kplus::backend::files::PathMaster PathMaster;
  typedef z2kplus::backend::reverse_index::index::ConsolidatedIndex ConsolidatedIndex;
  typedef z2kplus::backend::reverse_index::iterators::ZgramIterator ZgramIterator;
  typedef z2kplus::backend::shared::protocol::message::DResponse DResponse;
  typedef z2kplus::backend::shared::MetadataRecord MetadataRecord;
  typedef z2kplus::backend::shared::ZgramCore ZgramCore;
  typedef z2kplus::backend::shared::ZgramId ZgramId;
  typedef z2kplus::backend::util::automaton::FiniteAutomaton FiniteAutomaton;

  static std::u32string_view friendlyReset(kosak::coding::text::ReusableString32 *rs, std::string_view s);

  static bool tryGetPathMaster(std::string_view nmspace, std::shared_ptr<PathMaster> *result,
      const FailFrame &ff);

  static bool tryMakeDfa(std::string_view pattern, FiniteAutomaton *result, const FailFrame &ff);

  static bool trySetupConsolidatedIndex(std::shared_ptr<PathMaster> pm, ConsolidatedIndex *ci,
      const FailFrame &ff);

  // Expected values start at *expectedBegin and go for 'expectedSize' elements **in the direction
  // of** 'forward'
  static bool searchTest(const std::string_view &callerInfo, const ConsolidatedIndex &ci,
      const ZgramIterator *iterator, bool forward, const uint64_t *rawOptionalZgramId,
      const uint64_t *expectedBegin, size_t expectedSize, const FailFrame &ff);

  // "four ways" means: {forward, backwards} x {start at beginning, start at indicated position}
  static bool fourWaySearchTest(const std::string_view &callerInfo, const ConsolidatedIndex &ci,
      const ZgramIterator *iterator, uint64_t rawZgramId,
      std::initializer_list<uint64_t> rawExpected, const FailFrame &ff);

  static bool tryPopulateFile(const PathMaster &pm, const CompressedFileKey &fileKey, std::string_view text,
      const FailFrame &ff);

  static bool tryParseDynamicZgrams(const std::string_view &records,
      std::vector<ZgramCore> *result, const FailFrame &ff);
  static bool tryParseDynamicMetadata(const std::string_view &records,
      std::vector<MetadataRecord> *result, const FailFrame &ff);

  // Helpers for testing a Server / Client pair
  static bool tryDrainZgrams(FakeFrontend *fe, size_t frontLimit, size_t backLimit, bool sendRequest,
      std::optional<std::chrono::milliseconds> timeout, std::vector<DResponse> *responses,
      const FailFrame &ff);
};
}  // namespace z2kplus::backend::test::util
