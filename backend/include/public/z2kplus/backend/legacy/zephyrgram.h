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
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "kosak/coding/coding.h"
#include "kosak/coding/comparers.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/myjson.h"
#include "z2kplus/backend/util/misc.h"

namespace z2kplus::backend::legacy {

enum class DisplayStyle {
  Default, Shielded, Emphasized
};
enum class RenderStyle {
  Default, Monospace, MarkDeepMathAjax
};

DECLARE_ENUM_JSON(DisplayStyleHolder, DisplayStyle,
    (DisplayStyle::Default, DisplayStyle::Shielded, DisplayStyle::Emphasized),
    ("d", "s", "e"));

DECLARE_ENUM_JSON(RenderStyleHolder, RenderStyle,
    (RenderStyle::Default, RenderStyle::Monospace, RenderStyle::MarkDeepMathAjax),
    ("d", "m", "x"));

// Naming convention: ZgramCore is what the client would submit to the server, containing all the
// fields of the zephyrgram. Meanwhile, Zephygram is that same item, after being endowed with a
// timestamp and ID.
class ZgramCore {
public:
  ZgramCore();
  ZgramCore(std::string sender, std::string signature, std::string clss, std::string instance,
    std::string body, RenderStyle renderStyle);
  DISALLOW_COPY_AND_ASSIGN(ZgramCore);
  DECLARE_MOVE_COPY_AND_ASSIGN(ZgramCore);
  ~ZgramCore();

  const std::string &sender() const { return sender_; }
  const std::string &signature() const { return signature_; }
  const std::string &clss() const { return clss_; }
  const std::string &instance() const { return instance_; }
  const std::string &body() const { return body_; }
  std::string &sender() { return sender_; }
  std::string &signature() { return signature_; }
  std::string &clss() { return clss_; }
  std::string &instance() { return instance_; }
  std::string &body() { return body_; }
  RenderStyle renderStyle() const { return renderStyle_; }

private:
  // e.g. "kosak" or "kosak.z"
  std::string sender_;

  // e.g. "Corey Kosak" or "kosak via zarchive web" or custom signature like "Lord Cinnabon".
  std::string signature_;

  // The Zephyr class. Typically (always?) "MESSAGE"
  std::string clss_;

  // The Zephyr instance. e.g. "help.cheese.search"
  std::string instance_;

  // The body of the message. e.g. "Where can I find some good cheese?"
  std::string body_;

  // How this message should be rendered.
  RenderStyle renderStyle_ = RenderStyle::Default;

  friend std::ostream &operator<<(std::ostream &o, const ZgramCore &zgb);
  DECLARE_TYPICAL_JSON(ZgramCore);
};

class ZgramId {
public:
  ZgramId() = default;
  explicit ZgramId(int64 id) : id_(id) {}

  int64 id() const { return id_; }

  int compare(const ZgramId &other) const {
    return kosak::coding::compare(&id_, &other.id_);
  }

private:
  int64 id_ = 0;

  DEFINE_ALL_COMPARISON_OPERATORS(ZgramId);

  friend std::ostream &operator<<(std::ostream &s, const ZgramId &o) {
    return s << o.id();
  }

  LEGACY_DEFINE_DELEGATED_JSON(ZgramId, id_);
};

// Final Zephyrgram format (with ID and timestamp)
class Zephyrgram {
public:
  Zephyrgram();
  Zephyrgram(ZgramId zgramId, uint64 timesecs, bool isLogged, ZgramCore zgramCore);
  DISALLOW_COPY_AND_ASSIGN(Zephyrgram);
  DECLARE_MOVE_COPY_AND_ASSIGN(Zephyrgram);
  ~Zephyrgram();

  ZgramId zgramId() const { return zgramId_; }
  uint64 timesecs() const { return timesecs_; }
  bool isLogged() const { return isLogged_; }
  const ZgramCore &zgramCore() const { return zgramCore_; }
  ZgramCore &zgramCore() { return zgramCore_; }

private:
  ZgramId zgramId_ = ZgramId(0);
  uint64 timesecs_ = 0;
  bool isLogged_ = false;
  ZgramCore zgramCore_;

  friend std::ostream &operator<<(std::ostream &s, const Zephyrgram &o);
  DECLARE_TYPICAL_JSON(Zephyrgram);
};

enum class EmotionalReaction { None, Dislike, Like };
enum class ThreadIdClassification { None, Root, Inherited };
enum class AlertState { None, NotFired, Fired };

DECLARE_ENUM_JSON(EmotionalReactionHolder, EmotionalReaction,
  (EmotionalReaction::None, EmotionalReaction::Dislike, EmotionalReaction::Like),
  ("", "d", "l"));

DECLARE_ENUM_JSON(ThreadIdClassificationHolder, ThreadIdClassification,
    (ThreadIdClassification::None, ThreadIdClassification::Root, ThreadIdClassification::Inherited),
    ("", "r", "i"));

DECLARE_ENUM_JSON(AlertStateHolder, AlertState,
    (AlertState::None, AlertState::NotFired, AlertState::Fired),
    ("", "n", "f"));

class PerZgramMetadataCore {
public:
  // kosak -> EmotionalReaction::Like
  typedef std::map<std::string, EmotionalReaction> reactions_t;
  // #insightful -> kosak -> true
  typedef std::map<std::string, std::map<std::string, bool>> hashtags_t;
  // kosak -> true
  typedef std::map<std::string, bool> bookmarks_t;
  // Refers to two different zgrams: 1004,0 and 2001,0
  // [1004,0] -> true
  // [2001,0] -> true
  typedef std::map<ZgramId, bool> refersTo_t;
  typedef refersTo_t referredFrom_t;
  // 75 -> true
  typedef std::map<uint64, ThreadIdClassification> threads_t;
  // 0 -> foo\001bar (means s/foo/bar. the \001 is the separator). Only the zgram author
  // can edit their own zgrams.
  typedef std::map<uint64, std::string> edits_t;
  // kosak -> 59804  (total ++ so far)
  typedef std::map<std::string, int64> pluspluses_t;
  // Who is watching this zgram, and transitively, the threads that it belongs to.
  // kosak -> watchName (the watchName can be customized by the user)
  typedef std::map<std::string, std::string> watches_t;

  PerZgramMetadataCore();
  PerZgramMetadataCore(reactions_t &&reactions, hashtags_t &&hashtags,
    bookmarks_t &&bookmarks, refersTo_t &&refersTo, referredFrom_t &&referredFrom,
    threads_t &&threads, edits_t &&edits, pluspluses_t &&pluspluses, watches_t &&watches);
  //PerZgramMetadataCore(const PerZgramMetadataCore &other);
  DISALLOW_COPY_AND_ASSIGN(PerZgramMetadataCore);
  DECLARE_MOVE_COPY_AND_ASSIGN(PerZgramMetadataCore);
  ~PerZgramMetadataCore();

  reactions_t &reactions() { return reactions_; }
  hashtags_t &hashtags() { return hashtags_; }
  bookmarks_t &bookmarks() { return bookmarks_; }
  refersTo_t &refersTo() { return refersTo_; }
  referredFrom_t &referredFrom() { return referredFrom_; }
  threads_t &threads() { return threads_; }
  edits_t &edits() { return edits_; }
  pluspluses_t &pluspluses() { return pluspluses_; }
  watches_t &watches() { return watches_; }

  const reactions_t &reactions() const { return reactions_; }
  const hashtags_t &hashtags() const { return hashtags_; }
  const bookmarks_t &bookmarks() const { return bookmarks_; }
  const refersTo_t &refersTo() const { return refersTo_; }
  const referredFrom_t &referredFrom() const { return referredFrom_; }
  const threads_t &threads() const { return threads_; }
  const edits_t &edits() const { return edits_; }
  const pluspluses_t &pluspluses() const { return pluspluses_; }
  const watches_t &watches() const { return watches_; }

private:
  reactions_t reactions_;
  hashtags_t hashtags_;
  bookmarks_t bookmarks_;
  refersTo_t refersTo_;
  referredFrom_t referredFrom_;
  threads_t threads_;
  edits_t edits_;
  pluspluses_t pluspluses_;
  watches_t watches_;

  friend std::ostream &operator<<(std::ostream &s, const PerZgramMetadataCore &o);
  DECLARE_TYPICAL_JSON(PerZgramMetadataCore);
};

// This class is used in two different (but related) contexts:
// - To represent the metadata associated with a specific userid.
// - To represent deltas to such metadata
class PerUseridMetadataCore {
public:
  // Frequently-used emojis
  // e.g. Unit -> ❦❧💕💞🙆🙅😂
  typedef std::map<kosak::coding::Unit, std::string> zmojis_t;
  // Alerts.
  // e.g. zgram 1000 -> [alert name "important", AlertState::Fired]
  typedef std::map<ZgramId, std::pair<std::string, AlertState>> alerts_t;

  PerUseridMetadataCore();
  explicit PerUseridMetadataCore(zmojis_t &&zmojis, alerts_t &&alerts);
  DISALLOW_COPY_AND_ASSIGN(PerUseridMetadataCore);
  DECLARE_MOVE_COPY_AND_ASSIGN(PerUseridMetadataCore);
  ~PerUseridMetadataCore();

  zmojis_t &zmojis() { return zmojis_; }
  const zmojis_t &zmojis() const { return zmojis_; }

  alerts_t &alerts() { return alerts_; }
  const alerts_t &alerts() const { return alerts_; }

private:
  zmojis_t zmojis_;
  alerts_t alerts_;

  friend std::ostream &operator<<(std::ostream &s, const PerUseridMetadataCore &o);
  DECLARE_TYPICAL_JSON(PerUseridMetadataCore);
};

class Metadata {
public:
  typedef std::map<ZgramId, PerZgramMetadataCore> pzg_t;
  typedef std::map<std::string, PerUseridMetadataCore> pu_t;

  Metadata();
  Metadata(pzg_t &&pzg, pu_t &&pu);
  DISALLOW_COPY_AND_ASSIGN(Metadata);
  DECLARE_MOVE_COPY_AND_ASSIGN(Metadata);
  ~Metadata();

  pzg_t &pzg() { return pzg_; }
  pu_t &pu() { return pu_; }

  const pzg_t &pzg() const { return pzg_; }
  const pu_t &pu() const { return pu_; }

private:
  pzg_t pzg_;
  pu_t pu_;

  friend std::ostream &operator<<(std::ostream &s, const Metadata &o);
  DECLARE_TYPICAL_JSON(Metadata);
};

typedef std::variant<Zephyrgram, Metadata> payload_t;
DECLARE_VARIANT_JSON(PayloadHolder, payload_t, ("z", "m"));

class LogRecord {
public:
  typedef kosak::coding::FailFrame FailFrame;
  typedef kosak::coding::ParseContext ParseContext;

  LogRecord();
  explicit LogRecord(Zephyrgram &&zgram);
  explicit LogRecord(Metadata &&md);
  LogRecord(LogRecord &&other) noexcept;
  LogRecord &operator=(LogRecord &&other) noexcept;
  DISALLOW_COPY_AND_ASSIGN(LogRecord);
  ~LogRecord();

  payload_t &payload() { return payload_; }
  const payload_t &payload() const { return payload_; }

private:
  payload_t payload_;

  friend std::ostream &operator<<(std::ostream &s, const LogRecord &o) {
    return s << o.payload_;
  }

  LEGACY_DEFINE_DELEGATED_JSON(LogRecord, payload_);
};
}  // namespace z2kplus::backend::legacy

