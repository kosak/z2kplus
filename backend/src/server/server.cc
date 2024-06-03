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

#include "z2kplus/backend/server/server.h"

#include <future>
#include <memory>
#include "kosak/coding/coding.h"
#include "kosak/coding/unix.h"
#include "z2kplus/backend/communicator/channel.h"
#include "z2kplus/backend/communicator/communicator.h"
#include "z2kplus/backend/communicator/session.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/reverse_index/builder/index_builder.h"
#include "z2kplus/backend/util/misc.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::makeReservedVector;
using kosak::coding::ParseContext;
using kosak::coding::streamf;
using kosak::coding::stringf;
using kosak::coding::toString;
using kosak::coding::Unit;
using z2kplus::backend::communicator::Channel;
using z2kplus::backend::communicator::Communicator;
using z2kplus::backend::communicator::CommunicatorCallbacks;
using z2kplus::backend::communicator::MessageBuffer;
using z2kplus::backend::communicator::Session;
using z2kplus::backend::coordinator::Coordinator;
using z2kplus::backend::coordinator::Subscription;
using z2kplus::backend::files::CompressedFileKey;
using z2kplus::backend::files::ExpandedFileKey;
using z2kplus::backend::files::FilePosition;
using z2kplus::backend::files::InterFileRange;
using z2kplus::backend::files::TaggedFileKey;
using z2kplus::backend::reverse_index::builder::IndexBuilder;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::shared::Profile;
using z2kplus::backend::shared::RenderStyle;
using z2kplus::backend::shared::SearchOrigin;
using z2kplus::backend::shared::ZgramCore;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::shared::protocol::Estimates;
using z2kplus::backend::shared::protocol::message::DRequest;
using z2kplus::backend::shared::protocol::message::DResponse;
using z2kplus::backend::util::BlockingQueue;

#define HERE KOSAK_CODING_HERE

namespace coordinator = z2kplus::backend::server;
namespace drequests = z2kplus::backend::shared::protocol::message::drequests;
namespace dresponses = z2kplus::backend::shared::protocol::message::dresponses;
namespace magicConstants = z2kplus::backend::shared::magicConstants;
namespace nsunix = kosak::coding::nsunix;
namespace protocol = z2kplus::backend::shared::protocol;

// Within ClientMessage we will see things like:
// 1. Subscribe to zgrams
// 2. GetMore (works with the "paging" notion of zgrams)
// 3. Post a zgram
// 4. Post metadata
//
// Furthermore, processing a ClientMessage will typically change the state of the Coordinator.
// When this happens it will also have little events (typically messages back to our subscribers)
// that it will want us to handle.
namespace z2kplus::backend::server {
struct Server::SessionAndDRequest {
  typedef z2kplus::backend::communicator::Session Session;
  typedef z2kplus::backend::shared::protocol::message::DRequest DRequest;

  SessionAndDRequest(std::shared_ptr<Session> session, DRequest request);
  DISALLOW_COPY_AND_ASSIGN(SessionAndDRequest);
  DECLARE_MOVE_COPY_AND_ASSIGN(SessionAndDRequest);
  ~SessionAndDRequest();

  std::shared_ptr<Session> session_;
  DRequest request_;
};

class Server::ServerCallbacks final : public CommunicatorCallbacks {
public:
  explicit ServerCallbacks(std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo) :
      todo_(std::move(todo)) {}
  ~ServerCallbacks() final = default;

  bool tryOnRequest(Session *session, DRequest &&message, const FailFrame &/*ff*/) final {
    SessionAndDRequest scd(session->shared_from_this(), std::move(message));
    todo_->append(std::move(scd));
    return true;
  }

private:
  std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo_;
};

struct Server::ReindexingState {
  typedef kosak::coding::FailFrame FailFrame;
  typedef z2kplus::backend::files::CompressedFileKey CompressedFileKey;
  typedef z2kplus::backend::files::PathMaster PathMaster;

  template<typename T>
  using MessageBuffer = z2kplus::backend::communicator::MessageBuffer<T>;

  static bool tryCreate(std::shared_ptr<PathMaster> pm,
      std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo,
      InterFileRange<true> loggedRange, InterFileRange<false> unloggedRange,
      std::shared_ptr<ReindexingState> *result, const FailFrame &ff);

  ReindexingState(std::shared_ptr<PathMaster> pm,
      std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo,
      InterFileRange<true> loggedRange,
      InterFileRange<false> unloggedRange);

  static void run(std::shared_ptr<ReindexingState> self);

  bool tryRunHelper(const FailFrame &ff);
  bool tryCleanup(const FailFrame &ff);

  std::shared_ptr<PathMaster> pm_;
  std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo_;
  InterFileRange<true> loggedRange_;
  InterFileRange<false> unloggedRange_;
  std::atomic<bool> done_ = false;
  std::thread activeThread_;
  std::string error_;
};

bool Server::tryCreate(Coordinator &&coordinator, int requestedPort, std::shared_ptr<Server> *result,
    const FailFrame &ff) {
  // Fake up an admin profile
  auto adminProfile = std::make_shared<Profile>(magicConstants::zalexaId,
      magicConstants::zalexaSignature);

  auto todo = std::make_shared<MessageBuffer<SessionAndDRequest>>();
  auto callbacks = std::make_shared<ServerCallbacks>(todo);
  std::shared_ptr<Communicator> communicator;
  if (!Communicator::tryCreate(requestedPort, std::move(callbacks), &communicator, ff.nest(HERE))) {
    return false;
  }
  auto now = std::chrono::system_clock::now();
  auto nextPurgeTime = now + magicConstants::purgeInterval;
  auto nextReindexingTime = now + magicConstants::reindexingInterval;
  auto server = std::make_shared<Server>(Private(), std::move(communicator), std::move(coordinator),
      std::move(adminProfile), std::move(todo), nextPurgeTime, nextReindexingTime);
  std::thread t(&threadMain, server);
  if (!nsunix::trySetThreadName(&t, serverName, ff.nest(HERE))) {
    return false;
  }
  t.detach();
  *result = std::move(server);
  return true;
}

Server::Server() = default;
Server::Server(Private, std::shared_ptr<Communicator> communicator, Coordinator coordinator,
    std::shared_ptr<Profile> adminProfile,
    std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo,
    std::chrono::system_clock::time_point nextPurgeTime,
    std::chrono::system_clock::time_point nextReindexingTime) :
    communicator_(std::move(communicator)), coordinator_(std::move(coordinator)),
    adminProfile_(std::move(adminProfile)), todo_(std::move(todo)),
    nextPurgeTime_(nextPurgeTime), nextReindexingTime_(nextReindexingTime) {}
Server::~Server() = default;

void Server::threadMain(const std::shared_ptr<Server> &self) {
  FailRoot fr;
  if (!self->tryRunForever(fr.nest(HERE))) {
    warn("%o: failed: %o", serverName, fr);
  }
  warn("%o: exiting", serverName);
}

bool Server::tryStop(const FailFrame &/*fr*/) {
  todo_->shutdown();
  return true;
}

bool Server::tryRunForever(const FailFrame &ff) {
  std::chrono::seconds timeout(30);

  while (true) {
    bool wantShutdown;
    std::vector<SessionAndDRequest> incomingBuffer;
    todo_->waitForDataAndSwap(timeout, &incomingBuffer, &wantShutdown);
    if (wantShutdown) {
      warn("%o: Shutdown requested", serverName);
      return true;
    }

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> statusMessages;
    if (!tryProcessRequests(now, std::move(incomingBuffer), ff.nest(HERE)) ||
        !tryManageReindexing(now, &statusMessages, ff.nest(HERE)) ||
        !tryManagePurging(now, &statusMessages, ff.nest(HERE))) {
      return false;
    }

    // Let's disable status messages for now. They are distracting.
    if (false) {
      using entry_t = drequests::PostZgrams::entry_t;
      auto zgs = makeReservedVector<entry_t>(statusMessages.size());
      for (auto &sm: statusMessages) {
        ZgramCore zgc("graffiti.ZSTATUS", std::move(sm), RenderStyle::Default);
        zgs.push_back(entry_t(std::move(zgc), {}));
      }
      drequests::PostZgrams req(std::move(zgs));
      std::vector<coordinatorResponse_t> responses;
      if (!coordinator_.tryPostZgramsNoSub(*adminProfile_, now, std::move(req), &responses,
          ff.nest(HERE)) ||
          !tryProcessResponses(std::move(responses), nullptr, ff.nest(HERE))) {
        return false;
      }
    }
  }
}

bool Server::tryProcessRequests(std::chrono::system_clock::time_point now,
    std::vector<SessionAndDRequest> incomingBuffer,
    const FailFrame &ff) {
  warn("There are %o items to process", incomingBuffer.size());

  for (auto &entry : incomingBuffer) {
    std::vector<coordinatorResponse_t> responses;
    handleRequest(std::move(entry.request_), entry.session_.get(), now, &responses);
    if (!tryProcessResponses(std::move(responses), entry.session_.get(), ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool Server::tryProcessResponses(std::vector<coordinatorResponse_t> responses,
    Session *optionalSenderSession, const FailFrame &ff) {
  for (auto &[sub, dresp]: responses) {
    Session *sessionToUse;
    if (sub == nullptr) {
      // If subscription is not specified in the responses, it means respond on the caller's session.
      sessionToUse = optionalSenderSession;
    } else {
      // However if it is specified, respond on the session associated with that subscription.
      auto ip = subscriptionToSession_.find(sub->id());
      if (ip == subscriptionToSession_.end()) {
        return ff.failf(HERE, "Weird. Couldn't find %o", sub->id());
      }
      sessionToUse = ip->second.get();
    }

    if (sessionToUse != nullptr &&
        !sessionToUse->trySendResponse(std::move(dresp), ff.nest(HERE))) {
      return false;
    }
  }
  return true;
}

bool Server::tryManageReindexing(std::chrono::system_clock::time_point now,
    std::vector<std::string> *statusMessages, const FailFrame &ff) {
  if (reindexingState_ == nullptr) {
    // No active reindexing thread...
    if (now < nextReindexingTime_) {
      auto distance = nextReindexingTime_ - now;
      warn("Not time to reindex... %o more seconds", std::chrono::duration_cast<std::chrono::seconds>(distance).count());
      // ...but not time to reindex yet.
      return true;
    }

    // ...and it's time to reindex!
    const char *message = "Starting the reindex process in the background. This part probably won't crash.";
    streamf(std::cerr, "%o\n", message);
    statusMessages->push_back(message);

    // 1. The logged start position is zero.
    // 2. Checkpoint the server. This will give us the logged end position and the unlogged end position
    // 3. The unlogged start position is (now - the unlogged lifespan... typically 1 week).

    FilePosition<true> loggedStartPosition;  // zero
    ExpandedFileKey unloggedStartEfk(now - magicConstants::unloggedLifespan, false);
    TaggedFileKey<false> unloggedStartKey(unloggedStartEfk.compress());
    FilePosition<false> unloggedStartPosition(unloggedStartKey, 0);

    FilePosition<true> loggedEndPosition;
    FilePosition<false> unloggedEndPosition;
    if (!coordinator_.tryCheckpoint(now, &loggedEndPosition, &unloggedEndPosition, ff.nest(HERE))) {
      return false;
    }

    InterFileRange<true> loggedRange(loggedStartPosition, loggedEndPosition);
    InterFileRange<false> unloggedRange(unloggedStartPosition, unloggedEndPosition);

    // Start the reindexing thread.
    return ReindexingState::tryCreate(coordinator_.pathMaster(), todo_, loggedRange,
        unloggedRange, &reindexingState_, ff.nest(HERE));
  }

  // There is an active reindexing thread.
  if (!reindexingState_->done_) {
    // ...but it's still running
    return true;
  }
  auto rs = std::move(reindexingState_);
  reindexingState_.reset();  // to be sure.

  // ... it's done, so join the thread and check out its error indication
  rs->activeThread_.join();
  auto error = std::move(rs->error_);

  if (!error.empty()) {
    auto message = stringf("Reindexing failure. PLEASE NOTIFY THE ADMIN. This is very bad. %o",
        error);
    warn("%o", message);
    statusMessages->push_back(std::move(message));
    // Try to keep going I guess. But don't reindex any more. Set the next reindexing time way
    // out in the future.
    nextReindexingTime_ = now + std::chrono::hours(1000 * 24);  // +1000 days. Had to say something
    return true;
  }

  // ...it's done, and successful.
  nextReindexingTime_ = now + magicConstants::reindexingInterval;

  const char *message = "Reindexing complete! Hopefully nothing broke.";
  warn("%o", message);
  statusMessages->emplace_back(message);
  return coordinator_.tryResetIndex(now, ff.nest(HERE)) &&
      rs->tryCleanup(ff.nest(HERE));
}

bool Server::tryManagePurging(std::chrono::system_clock::time_point now,
    std::vector<std::string> *statusMessages, const FailFrame &ff) {
  if (now < nextPurgeTime_) {
    return true;
  }

  nextPurgeTime_ = now + magicConstants::purgeInterval;
  return true;
}


void Server::handleRequest(DRequest &&req, Session *session, std::chrono::system_clock::time_point now,
    std::vector<coordinatorResponse_t> *responses) {
  auto *subReq = std::get_if<drequests::Subscribe>(&req.payload());
  if (subReq != nullptr) {
    return handleSubscribeRequest(std::move(*subReq), session, responses);
  }

  auto ip = sessionToSubscription_.find(session->id());
  if (ip == sessionToSubscription_.end()) {
    dresponses::GeneralError failure("Channel is not subscribed");
    responses->emplace_back(nullptr, std::move(failure));
    return;
  }

  handleNonSubRequest(std::move(req), ip->second.get(), now, responses);
}

void Server::handleNonSubRequest(DRequest &&req, Subscription *sub,
    std::chrono::system_clock::time_point now, std::vector<coordinatorResponse_t> *responses) {
  struct visitor_t {
    visitor_t(Coordinator *coordinator, Subscription *sub, std::chrono::system_clock::time_point now,
        std::vector<coordinatorResponse_t> *responses) : coordinator_(coordinator), sub_(sub),
        now_(now), responses_(responses) {}

    void operator()(drequests::Subscribe &&) const {
      crash("Not possible -- case already handled.");
    }
    void operator()(drequests::CheckSyntax &&o) const {
      coordinator_->checkSyntax(sub_, std::move(o), responses_);
    }
    void operator()(drequests::GetMoreZgrams &&o) const {
      coordinator_->getMoreZgrams(sub_, std::move(o), responses_);
    }
    void operator()(drequests::PostZgrams &&o) const {
      coordinator_->postZgrams(sub_, now_, std::move(o), responses_);
    }
    void operator()(drequests::PostMetadata &&o) const {
      coordinator_->postMetadata(sub_, std::move(o), responses_);
    }
    void operator()(drequests::GetSpecificZgrams &&o) const {
      coordinator_->getSpecificZgrams(sub_, std::move(o), responses_);
    }
    void operator()(drequests::Ping &&o) const {
      coordinator_->ping(sub_, std::move(o), responses_);
    }

    Coordinator *coordinator_ = nullptr;
    Subscription *sub_ = nullptr;
    std::chrono::system_clock::time_point now_;
    std::vector<coordinatorResponse_t> *responses_ = nullptr;
  };

  visitor_t visitor(&coordinator_, sub, now, responses);
  std::visit(visitor, std::move(req.payload()));
}

void Server::handleSubscribeRequest(drequests::Subscribe &&subReq, Session *session,
    std::vector<coordinatorResponse_t> *responses) {
  if (sessionToSubscription_.find(session->id()) != sessionToSubscription_.end()) {
    std::string error("Impossible: session is already bound to a subscription");
    responses->emplace_back(nullptr, dresponses::AckSubscribe(false, std::move(error), Estimates()));
    return;
  }
  std::shared_ptr<Subscription> possibleNewSub;
  coordinator_.subscribe(session->profile(), std::move(subReq), responses, &possibleNewSub);

  // If the subscription was successful, record it in the session.
  if (possibleNewSub != nullptr) {
    subscriptionToSession_.emplace(possibleNewSub->id(), session->shared_from_this());
    sessionToSubscription_.emplace(session->id(), std::move(possibleNewSub));
  }
}

//bool Server::tryDoPurge(std::chrono::system_clock::time_point now, Failures *failures) {
//  auto timeSinceLastPurge = now - lastPurgeTime_;
//  if (timeSinceLastPurge < std::chrono::seconds(magicConstants::idleSessionPurgeIntervalSecs)) {
//    return true;
//  }
//  lastPurgeTime_ = now;
//  // Linear search, but only happens once per idleSessionPurgeIntervalSecs
//  for (auto ip = sessions_.begin(); ip != sessions_.end(); ) {  // no ++
//    auto *session = ip->second.get();
//    auto timeSinceLastActivity = session->lastActivityTime() - now;
//    if (timeSinceLastActivity < std::chrono::seconds(magicConstants::idleSessionTimeoutSecs)) {
//      ++ip;
//      continue;
//    }
//
//    if (!session->tryCleanupForGarbageCollection(&coordinator_, &cb, failures)) {
//      return false;
//    }
//    ip = sessions_.erase(ip);
//  }
//  return true;
//}

bool Server::ReindexingState::tryCreate(std::shared_ptr<PathMaster> pm,
    std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo,
    InterFileRange<true> loggedRange, InterFileRange<false> unloggedRange,
    std::shared_ptr<ReindexingState> *result, const FailFrame &/*ff*/) {
  auto res = std::make_shared<ReindexingState>(std::move(pm), std::move(todo), loggedRange,
      unloggedRange);
  res->activeThread_ = std::thread(&run, res);
  *result = std::move(res);
  return true;
}

Server::ReindexingState::ReindexingState(std::shared_ptr<PathMaster> pm,
    std::shared_ptr<MessageBuffer<SessionAndDRequest>> todo,
    InterFileRange<true> loggedRange, InterFileRange<false> unloggedRange) : pm_(std::move(pm)),
    todo_(std::move(todo)), loggedRange_(loggedRange), unloggedRange_(unloggedRange),
    done_(false) {}

void Server::ReindexingState::run(std::shared_ptr<ReindexingState> self) {
  std::cerr << "Reindexing thread starting\n";
  FailRoot fr;
  if (!self->tryRunHelper(fr.nest(HERE))) {
    self->error_ = toString(fr);
    streamf(std::cerr, "Reindexing thread finished with error: %o", self->error_);
  } else {
    std::cerr << "Reindexing thread finished normally.\n";
  }
  self->done_ = true;
  self->todo_->interrupt();
}

bool Server::ReindexingState::tryRunHelper(const FailFrame &ff) {
  if (!IndexBuilder::tryClearScratchDirectory(*pm_, ff.nest(HERE)) ||
      !IndexBuilder::tryBuild(*pm_, loggedRange_, unloggedRange_, ff.nest(HERE)) ||
      !pm_->tryPublishBuild(ff.nest(HERE))) {
    return false;
  }
  return true;
}

bool Server::ReindexingState::tryCleanup(const FailFrame &ff) {
  auto cb = [this](const CompressedFileKey &fk, const FailFrame &f2) {
    ExpandedFileKey efk(fk);
    if (efk.isLogged()) {
      return true;
    }
    TaggedFileKey<false> tfk(fk);
    auto beginFk = unloggedRange_.begin().fileKey();
    if (!(tfk < beginFk)) {
      return true;
    }
    // file key is unlogged and prior to the 'after purge' point.
    auto path = pm_->getPlaintextPath(fk);
    return nsunix::tryUnlink(path, f2.nest(HERE));
  };
  return pm_->tryGetPlaintexts(&cb, ff.nest(HERE));
}

Server::SessionAndDRequest::SessionAndDRequest(std::shared_ptr<Session> session, DRequest request)
    : session_(std::move(session)), request_(std::move(request)) {
}
Server::SessionAndDRequest::SessionAndDRequest(SessionAndDRequest &&) noexcept = default;
Server::SessionAndDRequest &Server::SessionAndDRequest::operator=(SessionAndDRequest &&) noexcept = default;
Server::SessionAndDRequest::~SessionAndDRequest() = default;
}  // namespace z2kplus::backend::server
