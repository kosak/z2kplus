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

#include "z2kplus/backend/test/util/test_util.h"

#include <chrono>
#include <sys/types.h>
#include <string>
#include <utility>
#include <vector>
#include "catch/catch.hpp"
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "kosak/coding/unix.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/files/path_master.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/reverse_index/builder/index_builder.h"
#include "z2kplus/backend/reverse_index/index/consolidated_index.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/reverse_index/types.h"
#include "z2kplus/backend/shared/profile.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/automaton/automaton.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/test/util/fake_frontend.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::ParseContext;
using kosak::coding::stringf;
using kosak::coding::tryParseJson;
using kosak::coding::memory::MappedFile;
using kosak::coding::text::Splitter;
using kosak::coding::text::tryConvertUtf8ToUtf32;
using z2kplus::backend::server::Server;
using z2kplus::backend::communicator::Communicator;
using z2kplus::backend::communicator::Channel;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::coordinator::Subscription;
using z2kplus::backend::factories::LogParser;
using z2kplus::backend::files::FileKey;
using z2kplus::backend::files::FileKeyKind;
using z2kplus::backend::files::FilePosition;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::LogLocation;
using z2kplus::backend::files::PathMaster;
using z2kplus::backend::queryparsing::WordSplitter;
using z2kplus::backend::reverse_index::builder::IndexBuilder;
using z2kplus::backend::reverse_index::index::ConsolidatedIndex;
using z2kplus::backend::reverse_index::index::FrozenIndex;
using z2kplus::backend::reverse_index::iterators::IteratorContext;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::iterators::zgramRel_t;
using z2kplus::backend::shared::LogRecord;
using z2kplus::backend::shared::MetadataRecord;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::Zephyrgram;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::automaton::PatternChar;

namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;
namespace nsunix = kosak::coding::nsunix;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::test::util {

// Note: the timestamps used inside the zgrams need to be nondecreasing so as to enable
// efficient searching.
auto key20000101 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 1, true);
const char zgrams20000101[] =
    R"([["z",[[0],946684800,"kosak","Corey Kosak",true,["new-millennium","Welcome to the new millennium!!!","d"]]]])" "\n"
    R"([["z",[[1],946684801,"kosak","Corey Kosak",true,["new-millennium","I have written a chat system for you. Do you like it?","d"]]]])" "\n"
    R"([["z",[[2],946684802,"kosh","Kosh",true,["new-millennium","You are not ready.","d"]]]])" "\n"
    R"([["z",[[3],946684803,"kosak","Corey Kosak",true,["new-millennium","What?","d"]]]])" "\n"
    R"([["z",[[4],946684804,"kosh","Kosh",true,["new-millennium","kosak.","d"]]]])" "\n"

// kosak starts playing with upper case and Unicode.
    R"([["z",[[10],946684810,"kosak","Corey Kosak",true,["feelings","I love to eat pie and Cinnabon at the cafe","d"]]]])" "\n"
    R"([["z",[[11],946684811,"kosak","Corey Kosak",true,["feelings.upper","I LOVE TO EAT PIE AND CINNABON AT THE CAFE","d"]]]])" "\n"
    R"([["z",[[12],946684812,"kosak","Corey Kosak",true,["feelings.unikodez","I ❤ to eat π and 𝐂𝐈𝐍𝐍𝐀𝐁𝐎𝐍 at the café","d"]]]])" "\n"
    R"([["z",[[13],946684813,"kosak","Corey Kosak",true,["feelings.unikodez.spelling.WTF","🙀Cιηη🔥вση🙀","d"]]]])" "\n"
    R"([["z",[[14],946684814,"kosak","Corey Kosak",true,["feelings.unikodez.spelling.WTF","Why is the instance misspelled?","d"]]]])" "\n"
    R"([["z",[[15],946684815,"kosak","Corey Kosak",true,["redact me","Jenny: 867-5309","d"]]]])" "\n"

// Quotes and contractions and punctuation
    R"([["z",[[20],946684807,"kosak","Corey Kosak",true,["words","What's with all the hurly-burly?","d"]]]])" "\n"
    R"([["z",[[21],946684808,"wilhelm","Crown Prince Wilhelm",true,["words","\"hurly-burly\"?","d"]]]])" "\n"
    R"([["z",[[22],946684809,"kosak","Corey Kosak",true,["words","\"\"hurly-burly\"\"","d"]]]])" "\n"
    R"([["z",[[23],946684810,"kosak","Corey Kosak",true,["words.Î","You are just jealous of my élite C++ skills. And C#. And C*. And C?","d"]]]])" "\n";

// likes and dislikes
auto key20000102 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 2, true);
const char zgrams20000102[] =
    R"([["z",[[30],946771200,"kosak","Starbuck 2000",true,["tv.wilhelm","The reimagined Battlestar Galactica™ is the best thing ever","d"]]]])" "\n"
    // Prince Wilhelm and Corey both "like" zgram 30
    R"([["m",[["rx",[[30],"👍","kosak",true]]]]])" "\n"
    R"([["m",[["rx",[[30],"👍","wilhelm",true]]]]])" "\n"
    // simon dislikes 30
    R"([["m",[["rx",[[30],"👎","simon",true]]]]])" "\n"
    // Liking and disliking some old zgrams
    R"([["m",[["rx",[[1],"👎","kosak",true]]]]])" "\n"
    R"([["m",[["rx",[[0],"👍","kosak",true]]]]])" "\n"

// Zgram Revision
    R"_([["m",[["zgrev",[[14],["feelings.Unicode","(fixed)","d"]]]]]])_" "\n"
    R"([["m",[["zgrev",[[13],["feelings.Unicode","🙀Cιηη🔥вση🙀","d"]]]]]])" "\n"
    R"([["m",[["zgrev",[[12],["feelings.Unicode","I ❤ to eat π and 𝐂𝐈𝐍𝐍𝐀𝐁𝐎𝐍 at the café","d"]]]]]])" "\n";

// Reactions day. Plus a refers-to
auto key20000103 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 3, true);
const char zgrams20000103[] =
    R"xxx([["z",[[40],946857600,"simon","Simon Eriksson",true,["tv.wilhelm.delayed","I'm going to change my vote on Battlestar Galactica™","d"]]]])xxx" "\n"
    // simon changes his dislike of 30 to a like
    R"([["m",[["rx",[[30],"👎","simon",false]]]]])" "\n"
    R"([["m",[["rx",[[30],"👍","simon",true]]]]])" "\n"
    R"xxx([["z",[[41],946857603,"spock","Spock (Unpronounceable)",true,["logic","The next zgram is true.","d"]]]])xxx" "\n"
    R"xxx([["z",[[42],946857604,"spock","Spock (Unpronounceable)",true,["logic","The previous zgram is false.","d"]]]])xxx" "\n"
    // 42 refers to 41
    R"([["m",[["ref",[[42],[41],true]]]]])" "\n"
    // kosak and spock like 41
    R"([["m",[["rx",[[41],"👍","kosak",true]]]]])" "\n"
    R"([["m",[["rx",[[41],"👍","spock",true]]]]])" "\n"
    // but spock dislikes 42
    R"([["m",[["rx",[[42],"👎","spock",true]]]]])" "\n";

auto key20000104 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 4, true);
const char zgrams20000104[] =
    R"([["z",[[50],946944000,"august","August Horn of Årnäs",true,["z2kplus","Let me be the first to say it. kosak++","d"]]]])" "\n"
    R"([["z",[[51],946944001,"kosak","Corey Kosak",true,["z2kplus","This pain, no name.","d"]]]])" "\n";

// Our first graffiti zgram
auto key20000104g = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 4, false);
const char zgrams20000104g[] =
    R"([["z",[[52],946944002,"simon","Simon Eriksson",false,["graffiti.z2kplus","FAIL","d"]]]])" "\n";

// Some more "the the the", and testing our MathJax rendering.
// Kosak sets his zmojis a few times
// T'Pring applies a bunch of k-wrongs
auto key20000105 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 5, true);
const char zgrams20000105[] =
    R"([["z",[[60],947073600,"kosak","Corey Kosak",true,["repetition","the the zamboni the the","d"]]]])" "\n"
    R"([["z",[[61],947073601,"kosak","Corey Kosak",true,["repetition","the the the the the","d"]]]])" "\n"
    R"([["z",[[62],947073602,"kosak","Corey Kosak",true,["relativity","$ E=mc^2 $","d"]]]])" "\n"
    R"xxx([["z",[[63],947073603,"kosak","Corey Kosak",true,["test","kosak)","d"]]]])xxx" "\n"
    R"([["m",[["zmojis",["kosak","💕"]]]]])" "\n"
    R"([["m",[["zmojis",["kosak","❦,❧,💕,💞,🙆,🙅,😂"]]]]])" "\n"
    R"([["m",[["zmojis",["simon","☢"]]]]])" "\n"
    R"([["m",[["rx",[[15],"k-wrong","t'pring",true]]]]])" "\n"
    R"([["m",[["rx",[[14],"k-wrong","t'pring",true]]]]])" "\n"
    R"([["m",[["rx",[[13],"k-wrong","t'pring",true]]]]])" "\n"
    R"([["m",[["rx",[[50],"k-wrong","t'pring",true]]]]])" "\n";

// It's ++ and -- day
auto key20000106 = FileKey<FileKeyKind::Either>::createUnsafe(2000, 1, 6, true);
const char zgrams20000106[] =
    R"([["z",[[70],947073600,"simon","Simon Eriksson",true,["appreciation","kosak++ blah kosak++","d"]]]])" "\n"
    R"([["z",[[71],947073601,"kosak","Corey Kosak",true,["appreciation.anti","kosak--","d"]]]])" "\n";

auto loggedStartKey = FileKey<FileKeyKind::Logged>::createUnsafe(2000, 1, 7, true);
auto unloggedStartKey = FileKey<FileKeyKind::Unlogged>::createUnsafe(2000, 1, 7, false);
FilePosition<FileKeyKind::Logged> loggedStart(loggedStartKey, 0);
FilePosition<FileKeyKind::Unlogged> unloggedStart(unloggedStartKey, 0);

// This ends up being zgramId 72
const char dynamicZgrams[] =
    R"(["⒣⒲⒤⒯⒤⒜","Hello, what is this instance about?","d"])" "\n";

const char dynamicMetadata[] =
    // simon revokes his like of 30 and changes it to the "radioactive" symbol. He also makes 12 radioactive.
    R"([["rx",[[30],"👍","simon",false]]])" "\n"
    R"([["rx",[[30],"☢","simon",true]]])" "\n"
    R"([["rx",[[12],"☢","simon",true]]])" "\n";

namespace {
bool getTestRootDir(std::string_view nmSpace, std::string *result, const FailFrame &ff);
bool tryPopulateTestFiles(const PathMaster &pm, const FailFrame &ff);
}  // namespace

std::u32string_view TestUtil::friendlyReset(kosak::coding::text::ReusableString32 *rs,
    std::string_view s) {
  FailRoot fr;
  if (!rs->tryReset(s, fr.nest(HERE))) {
    FAIL(fr);
  }
  return rs->storage();
}

bool TestUtil::tryGetPathMaster(std::string_view nmspace, std::shared_ptr<PathMaster> *result,
    const FailFrame &ff) {
  std::string root;
  return getTestRootDir(nmspace, &root, ff.nest(HERE)) &&
      PathMaster::tryCreate(std::move(root), result, ff.nest(HERE));
}

bool TestUtil::tryMakeDfa(std::string_view pattern, FiniteAutomaton *result, const FailFrame &ff) {
  std::u32string u32Chars;
  if (!tryConvertUtf8ToUtf32(pattern, &u32Chars, ff.nest(HERE))) {
    return false;
  }
  std::vector<PatternChar> pcs;
  WordSplitter::translateToPatternChar(u32Chars, &pcs);

  *result = FiniteAutomaton(pcs.data(), pcs.size(), std::string(pattern));
  return true;
}

bool TestUtil::trySetupConsolidatedIndex(std::shared_ptr<PathMaster> pm, ConsolidatedIndex *ci,
    const FailFrame &ff) {
  Profile profile("kosak", "Corey Kosak");
  MappedFile<FrozenIndex> frozenIndex;
  std::vector<ZgramCore> zgramCores;
  std::vector<MetadataRecord> metadataRecords;
  ConsolidatedIndex::ppDeltaMap_t deltaMap;
  std::vector<Zephyrgram> zgrams;
  std::vector<MetadataRecord> movedMetadata;
  auto now = std::chrono::system_clock::now();
  return tryPopulateTestFiles(*pm, ff.nest(HERE)) &&
      IndexBuilder::tryBuild(*pm, InterFileRange<FileKeyKind::Logged>::everything,
          InterFileRange<FileKeyKind::Unlogged>::everything, ff.nest(HERE)) &&
      pm->tryPublishBuild(ff.nest(HERE)) &&
      frozenIndex.tryMap(pm->getIndexPath(), false, ff.nest(HERE)) &&
      ConsolidatedIndex::tryCreate(std::move(pm), loggedStart, unloggedStart, std::move(frozenIndex), ci,
          ff.nest(HERE)) &&
      tryParseDynamicZgrams(dynamicZgrams, &zgramCores, ff.nest(HERE)) &&
      tryParseDynamicMetadata(dynamicMetadata, &metadataRecords, ff.nest(HERE)) &&
      ci->tryAddZgrams(now, profile, std::move(zgramCores), &deltaMap, &zgrams, ff.nest(HERE)) &&
      ci->tryAddMetadata(std::move(metadataRecords), &deltaMap, &movedMetadata, ff.nest(HERE));
}

bool TestUtil::searchTest(const std::string_view &callerInfo, const ConsolidatedIndex &ci,
    const ZgramIterator *iterator, bool forward, const uint64_t *optionalStartPos,
    const uint64_t *expectedStart, size_t expectedSize, const FailFrame &ff) {
  IteratorContext ctx(ci, forward);
  auto state = iterator->createState(ctx);
  zgramRel_t startToUse; // default 0
  if (optionalStartPos != nullptr) {
    startToUse = ctx.offToRel(ci.lowerBound(ZgramId(*optionalStartPos)));
    if (!forward) {
      ++startToUse;
    }
  }
  debug ("Search test: forward=%o, startToUse=%o, iterator %o", forward, startToUse, iterator);
  std::vector<ZgramId> actual;
  while (true) {
    zgramRel_t buffer[100];
    auto size = iterator->getMore(ctx, state.get(), startToUse, buffer, STATIC_ARRAYSIZE(buffer));
    if (size == 0) {
      break;
    }
    actual.reserve(actual.size() + size);
    for (size_t i = 0; i < size; ++i) {
      const auto &zg = ci.getZgramInfo(ctx.relToOff(buffer[i]));
      actual.push_back(zg.zgramId());
    }
  }

  // Make a vector to reduce my pain
  std::vector<ZgramId> expected;
  expected.reserve(expectedSize);
  const uint64 *current = expectedStart;
  for (size_t i = 0; i < expectedSize; ++i) {
    expected.emplace_back(*current);
    if (forward) {
      ++current;
    } else {
      --current;
    }
  }
  if (expected != actual) {
    return ff.failf(HERE, "%o: forward=%o, start pos=%o, expected=%o, actual=%o",
        callerInfo, forward, startToUse, expected, actual);
  }
  return true;
}

bool TestUtil::fourWaySearchTest(const std::string_view &callerInfo, const ConsolidatedIndex &ci,
    const ZgramIterator *iterator, uint64_t rawZgramId, std::initializer_list<uint64_t> rawExpected,
    const FailFrame &ff) {
  // Forward, starting from begin, should get everything
  const auto *eb = rawExpected.begin();
  const auto *ee = rawExpected.end();
  auto eSize = rawExpected.size();
  if (!searchTest(callerInfo, ci, iterator, true, nullptr, eb, eSize, ff.nest(HERE))) {
    return false;
  }

  // Forward, starting from startPos, should get the items in 'expected' >= startPos
  auto ip = std::lower_bound(eb, ee, rawZgramId);
  auto offset = std::distance(eb, ip);
  if (!searchTest(callerInfo, ci, iterator, true, &rawZgramId, eb + offset, eSize - offset,
      ff.nest(HERE))) {
    return false;
  }

  // Reverse, starting from end, should get everything
  const uint64 *rbeginToUse = eSize == 0 ? nullptr : eb + eSize - 1;
  if (!searchTest(callerInfo, ci, iterator, false, nullptr, rbeginToUse, eSize, ff.nest(HERE))) {
    return false;
  }

  // Reverse, starting from selected point, should get the items in 'expected' < startPos
  const uint64_t *selectedToUse = offset == 0 ? nullptr : eb + offset - 1;
  return searchTest(callerInfo, ci, iterator, false, &rawZgramId, selectedToUse, offset, ff.nest(HERE));
}

bool TestUtil::tryPopulateFile(const PathMaster &pm, FileKey<FileKeyKind::Either> fileKey,
    std::string_view text, const FailFrame &ff) {
  auto fileName = pm.getPlaintextPath(fileKey);
  return nsunix::tryEnsureBaseExists(fileName, 0755, ff.nest(HERE)) &&
      nsunix::tryMakeFile(fileName, 0644, text, ff.nest(HERE));
}

bool TestUtil::tryParseDynamicZgrams(const std::string_view &records, std::vector<ZgramCore> *result,
    const FailFrame &ff) {
  auto splitter = Splitter::ofRecords(records, '\n');
  std::string_view record;
  while (splitter.moveNext(&record)) {
    ParseContext ctx(record);
    ZgramCore zgc;
    using kosak::coding::tryParseJson;
    if (!tryParseJson(&ctx, &zgc, ff.nest(HERE))) {
      return false;
    }
    debug("So painful: --%o-- ==%o==", zgc.instance(), zgc.body()); // ##REMOVE##
    result->emplace_back(std::move(zgc));
  }
  return true;
}

bool TestUtil::tryParseDynamicMetadata(const std::string_view &records,
    std::vector<MetadataRecord> *result, const FailFrame &ff) {
  auto splitter = Splitter::ofRecords(records, '\n');
  std::string_view record;
  while (splitter.moveNext(&record)) {
    ParseContext ctx(record);
    MetadataRecord mr;
    using kosak::coding::tryParseJson;
    if (!tryParseJson(&ctx, &mr, ff.nest(HERE))) {
      return false;
    }
    result->emplace_back(std::move(mr));
  }
  return true;
}

namespace {
bool getTestRootDir(std::string_view nmSpace, std::string *result, const FailFrame &ff) {
  auto root = stringf("/tmp/zarchive-test-%o-XXXXXX", nmSpace);
  auto retval = mkdtemp(&root[0]);
  if (!retval) {
    return ff.failf(HERE, "mkdtemp failed, errno=%o", errno);
  }
  root += '/';
  *result = std::move(root);
  return true;
}

bool tryPopulateTestFiles(const PathMaster &pm, const FailFrame &ff) {
  auto fileKeys = std::experimental::make_array(
      key20000101,
      key20000102,
      key20000103,
      key20000104,
      key20000104g,
      key20000105,
      key20000106
  );
  auto zgrams = std::experimental::make_array(
      zgrams20000101,
      zgrams20000102,
      zgrams20000103,
      zgrams20000104,
      zgrams20000104g,
      zgrams20000105,
      zgrams20000106
  );

  static_assert(fileKeys.size() == zgrams.size());
  for (size_t i = 0; i < fileKeys.size(); ++i) {
    if (!TestUtil::tryPopulateFile(pm, fileKeys[i], zgrams[i], ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}
}  // namespace

bool TestUtil::tryDrainZgrams(FakeFrontend *fe, size_t frontLimit, size_t backLimit, bool sendRequest,
    std::optional<std::chrono::milliseconds> timeout, std::vector<DResponse> *responses,
    const FailFrame &ff) {
  std::vector<DResponse> responseBuffer;
  while (frontLimit != 0 || backLimit != 0) {
    bool isBack = backLimit != 0;
    size_t count = isBack ? backLimit : frontLimit;
    if (sendRequest) {
      DRequest req(drequests::GetMoreZgrams(isBack, count));
      if (!fe->trySend(std::move(req), ff.nest(HERE))) {
        return false;
      }
      debug("so I successfully sent %o", req);
    } else {
      sendRequest = true;
    }

    // wait for the AckMore
    while (true) {
      bool wantShutdown;
      fe->waitForDataAndSwap(timeout, &responseBuffer, &wantShutdown);
      if (wantShutdown) {
        return ff.fail(HERE, "Frontend wants to shut down");
      }
      if (responseBuffer.empty()) {
        return ff.fail(HERE, "Timed out waiting for AckMore");
      }
      bool gotAckMore = false;
      for (auto &resp : responseBuffer) {
        responses->push_back(std::move(resp));
        const auto *am = std::get_if<dresponses::AckMoreZgrams>(&responses->back().payload());
        if (am == nullptr) {
          continue;
        }
        size_t *whichLimit = isBack ? &backLimit : &frontLimit;
        auto amountToSubtract = std::min(*whichLimit, am->zgrams().size());
        *whichLimit -= amountToSubtract;

        const auto &est = am->estimates();
        if (est.front().exact() && est.front().count() == 0) {
          frontLimit = 0;
        }
        if (est.back().exact() && est.back().count() == 0) {
          backLimit = 0;
        }
        gotAckMore = true;
      }

      if (gotAckMore) {
        break;
      }
    }
  }
  return true;
}

#if 0
namespace {
void checkForExhaustion(size_t *limit, const Estimate &estimate) {
  if (estimate.count() == 0 && !estimate.isLowerBound()) {
    // Server telling me that there's nothing left.
    *limit = 0;
  }
}
}  // namespace
#endif

#if 0
namespace coordinator {

bool trySendThenGetUpToNMore(Coordinator *c, const std::shared_ptr<Subscription> &sub,
  vector<SubscriptionResponse> *notifications, size_t bufferStart,
  size_t frontLimit, size_t backLimit, Failures *failures) {
  if (frontLimit > 0 &&
    !c->tryGetMore(sub, drequests::GetMore(Side::Front, frontLimit), notifications, failures)) {
    return false;
  }
  if (backLimit > 0 &&
    !c->tryGetMore(sub, drequests::GetMore(Side::Back, backLimit), notifications, failures)) {
      return false;
  }
  return tryGetUpToNMore(c, notifications, bufferStart, frontLimit, backLimit, failures);
}

bool tryGetUpToNMore(Coordinator *c, vector<SubscriptionResponse> *notifications,
  size_t bufferStart, size_t frontLimit, size_t backLimit, Failures *failures) {
  while (bufferStart < notifications->size()) {
    const auto &response = (*notifications)[bufferStart++];
    const auto &sub = response.subscription();
    const auto &payload = response.response().payload();

    const auto *am = std::get_if<dresponses::AckMore>(&payload);
    if (am != nullptr) {
      auto *which = am->side() == Side::Front ? &frontLimit : &backLimit;
      auto amountToSubtract = std::min<size_t>(am->numSatisfied(), *which);
      *which -= amountToSubtract;

      if (*which != 0 &&
          !c->tryGetMore(sub, drequests::GetMore(am->side(), *which), notifications, failures)) {
        return false;
      }
      continue;
    }

    const auto *est = std::get_if<dresponses::Estimates>(&payload);
    if (est != nullptr) {
      checkForExhaustion(&frontLimit, est->front());
      checkForExhaustion(&backLimit, est->back());
      continue;
    }
  }
  return true;
}
}  // namespace coordinator
#endif

#if 0
namespace {
typedef std::function<bool(DRequest &&, Failures *)> trySendFunc_t;
typedef std::function<void(size_t, std::vector<DResponse> *, bool *)> waitForDataAndSwapFunc_t;

bool tryGetMessagesHelper(const trySendFunc_t &trySendFunc,
    const waitForDataAndSwapFunc_t &waitForDataAndSwapFunc, size_t frontLimit, size_t backLimit,
    std::vector<DResponse> *responses, Failures *failures);
}  // namespace

bool tryGetMessages(FakeMiddleware *fm, const std::string_view &sessionId,
    MessageBuffer<DResponse> *responseBuffer, size_t frontLimit, size_t backLimit,
    std::vector<DResponse> *responses, Failures *failures) {

  auto sf = [fm, &sessionId](DRequest &&request, Failures *f2) {
    return fm->trySend(sessionId, std::move(request), f2);
  };

  auto wdsf = [responseBuffer](size_t timeoutMillis, std::vector<DResponse> *resp2, bool *wantShutdown) {
    responseBuffer->waitForDataAndSwap(timeoutMillis, resp2, wantShutdown);
  };
  return tryGetMessagesHelper(sf, wdsf, frontLimit, backLimit, responses, failures);
}

namespace {
bool trySendGetMore(const trySendFunc_t &trySendFunc, Side side, size_t limit,
    Failures *failures) {
  DRequest request(drequests::GetMore(side, limit));
  return trySendFunc(std::move(request), failures);
}

bool isExhausted(const Estimate &estimate) {
  return estimate.count() == 0 && !estimate.isLowerBound();
}

bool tryGetMessagesHelper(const trySendFunc_t &trySendFunc,
    const waitForDataAndSwapFunc_t &waitForDataAndSwapFunc, size_t frontLimit, size_t backLimit,
    std::vector<DResponse> *responses, Failures *failures) {
  auto frontWaiting = false;
  auto backWaiting = false;
  vector<DResponse> responseChunk;
  while (true) {
    if (!frontWaiting && frontLimit > 0) {
      if (!trySendGetMore(trySendFunc, Side::Front, frontLimit, failures)) {
        return false;
      }
      frontWaiting = true;
    }

    if (!backWaiting && backLimit > 0) {
      if (!trySendGetMore(trySendFunc, Side::Back, backLimit, failures)) {
        return false;
      }
      backWaiting = true;
    }

    if (!frontWaiting && !backWaiting) {
      // We are done
      return true;
    }

    bool wantShutdown;
    waitForDataAndSwapFunc(-1, &responseChunk, &wantShutdown);
    if (wantShutdown) {
      failures->add("Unexpected shutdown");
      return false;
    }

    for (auto &resp : responseChunk) {
      responses->push_back(std::move(resp));

      const auto &payload = responses->back().payload();
      const auto *am = std::get_if<dresponses::AckMore>(&payload);
      if (am != nullptr) {
        auto *which = am->side() == Side::Front ? &frontLimit : &backLimit;
        auto amountToSubtract = std::min<size_t>(am->numSatisfied(), *which);
        *which -= amountToSubtract;
        continue;
      }

      const auto *est = std::get_if<dresponses::Estimates>(&payload);
      if (est != nullptr) {
        if (isExhausted(est->front())) {
          frontLimit = 0;
        }
        if (isExhausted(est->back())) {
          backLimit = 0;
        }
      }
    }
  }
}

}  // namespace
#endif

}  // namespace z2kplus::backend::test::util
