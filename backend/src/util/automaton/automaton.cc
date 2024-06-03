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

#include "z2kplus/backend/util/automaton/automaton.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <set>
#include <string>
#include <unordered_set>

#include "kosak/coding/coding.h"
#include "kosak/coding/dumping.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/conversions.h"
#include "z2kplus/backend/util/automaton/fuzzy_unicode.h"
#include "z2kplus/backend/util/misc.h"
#include "z2kplus/backend/util/myallocator.h"

using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::Hexer;
using kosak::coding::streamf;
using kosak::coding::text::ReusableString8;
using kosak::coding::text::tryConvertUtf8ToUtf32;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::util::automaton {
namespace {
struct NDFANode {
  typedef std::pair<char32_t, NDFANode*> transition_t;

  NDFANode();
  NDFANode(bool accepting, std::vector<transition_t> transitions, NDFANode *otherwise, NDFANode *empty);
  DECLARE_MOVE_COPY_AND_ASSIGN(NDFANode);
  ~NDFANode();

  // If this node is an accepting state.
  bool accepting_ = false;
  // Transitions are ordered and unique.
  std::vector<transition_t> transitions_;
  // Optional transition for the "otherwise" case.
  NDFANode *otherwise_ = nullptr;
  // The epsilon transition.
  NDFANode *empty_ = nullptr;

  friend std::ostream &operator<<(std::ostream &s, const NDFANode &o);
};

class NDFAFactory {
public:
  NDFAFactory(const PatternChar *begin, size_t patternSize);

  NDFANode *start() const { return start_; }

private:
  NDFANode createLoose(char32_t ch, NDFANode *done);

  std::u32string expansionsStorage_;
  std::vector<NDFANode> storage_;
  NDFANode *start_ = nullptr;
};

struct IntermediateNode;

struct INodeEqual {
  bool operator()(const IntermediateNode *lhs, const IntermediateNode *rhs) const noexcept;
};

struct INodeHash {
  std::size_t operator()(const IntermediateNode *node) const noexcept;
};

typedef std::unordered_set<IntermediateNode *, INodeHash, INodeEqual> unique_t;

struct IntermediateNode {
  explicit IntermediateNode(unique_t::iterator uniqueIterator);
  ~IntermediateNode();

  typedef std::pair<char32_t, IntermediateNode*> transition_t;
  typedef std::pair<IntermediateNode*, size_t> incomingTransition_t;

  void addTransition(char32_t ch, IntermediateNode *target);
  void addOtherwiseTransition(IntermediateNode *target);

  bool accepting_ = false;
  std::vector<transition_t> transitions_;
  IntermediateNode *otherwise_ = nullptr;

  unique_t::iterator uniqueIterator_;
  bool isStart_ = false;
  std::vector<incomingTransition_t> incomingTransitions_;
  std::vector<IntermediateNode*> incomingOtherwises_;

  DFANode *dfaNode_ = nullptr;

  friend std::ostream &operator<<(std::ostream &s, const IntermediateNode &o);
};

class IntermediateKey {
public:
  explicit IntermediateKey(std::vector<const NDFANode*> items) : items_(std::move(items)) {}
  DEFINE_MOVE_COPY_AND_ASSIGN(IntermediateKey);

  const NDFANode *get(size_t index) const { return items_[index]; }
  size_t size() const { return items_.size(); }

  const std::vector<const NDFANode *> &items() const {
    return items_;
  }

private:
  std::vector<const NDFANode*> items_;

  friend bool operator==(const IntermediateKey &lhs, const IntermediateKey &rhs) {
    return lhs.items_ == rhs.items_;
  }

  [[maybe_unused]]
  friend std::ostream &operator<<(std::ostream &s, const IntermediateKey &o) {
    return s << o.items_;
  }
};

//struct IKeyEqual {
//  bool operator()(const IntermediateKey *lhs, const IntermediateKey *rhs) const noexcept;
//};

struct IKeyHash {
  std::size_t operator()(const IntermediateKey &key) const noexcept;
};

class IntermediateKeyBuilder {
public:
  void reset() { items_.clear(); }
  void add(NDFANode *node);

  bool empty() const { return items_.empty(); }
  IntermediateKey makeKey();

private:
  std::vector<const NDFANode*> items_;
};

class Converter {
public:
  explicit Converter(NDFANode *start);

  void squish();

  // Returns a vector of all the nodes (used to manage deallocation) and a pointer to the start
  std::pair<std::vector<DFANode>, DFANode *> finish();

private:
  IntermediateNode *lookupOrCreate(IntermediateKey &&key);
  void populateIntermediateNode(const IntermediateKey &key, IntermediateNode *node);
  void forwardReferences(IntermediateNode *from, IntermediateNode *to);
  void evictFromUnique(IntermediateNode *node);

  IntermediateKeyBuilder reusableBuilder_;
  typedef std::unordered_map<IntermediateKey, IntermediateNode, IKeyHash> intermediates_t;
  intermediates_t intermediates_;
  std::vector<intermediates_t::iterator> populateWork_;

  std::vector<IntermediateNode *> squishWork_;
  std::unordered_set<IntermediateNode *, INodeHash, INodeEqual> unique_;
};

bool tryRecursiveDump(std::ostream &s, const char *separator, const DFANode *node,
    std::set<const DFANode *> *beenHere, ReusableString8 *rs8, const FailFrame &ff);
}  // namespace

FiniteAutomaton::FiniteAutomaton(const PatternChar *begin, size_t patternSize,
    std::string description) : description_(std::move(description)) {
  NDFAFactory nodeFactory(begin, patternSize);

  // Owns the InternalNodes and keeps them alive
  Converter converter(nodeFactory.start());
  converter.squish();
  auto [dfaNodes, start] = converter.finish();
  nodes_ = std::move(dfaNodes);
  start_ = start;
}
FiniteAutomaton::FiniteAutomaton() = default;
FiniteAutomaton::FiniteAutomaton(FiniteAutomaton &&) noexcept = default;
FiniteAutomaton &FiniteAutomaton::operator=(FiniteAutomaton &&) noexcept = default;
FiniteAutomaton::~FiniteAutomaton() = default;

std::ostream &operator<<(std::ostream &s, const FiniteAutomaton &o) {
  std::set<const DFANode *> beenHere;
  FailRoot fr;
  ReusableString8 rs8;
  if (!tryRecursiveDump(s, "", o.start(), &beenHere, &rs8, fr.nest(HERE))) {
    streamf(s, "FAIL: %o", fr);
  }
  return streamf(s, "\nThe DFA has %o nodes", beenHere.size());
}

DFANode::DFANode() = default;
DFANode::DFANode(bool accepting, std::vector<transition_t> transitions, const DFANode *otherwise) :
    accepting_(accepting), transitions_(std::move(transitions)), otherwise_(otherwise) {}
DFANode::~DFANode() = default;

const DFANode *DFANode::tryAdvance(char32_t key) const {
  // binary search? linear search? hybrid? I think transitions is probably short except at the root
  for (const auto &t : transitions_) {
    if (t.first == key) {
      return t.second;
    }
  }
  return otherwise_;
}

const DFANode *DFANode::tryAdvance(std::u32string_view key) const {
  const auto *self = this;
  for (auto ch : key) {
    self = self->tryAdvance(ch);
    if (self == nullptr) {
      break;
    }
  }
  return self;
}

void DFANode::tryAdvanceMulti(std::u32string_view keys, const DFANode **result) const {
  const auto *tb = transitions_.data();
  const auto *te = tb + transitions_.size();
  for (size_t i = 0; i < keys.size(); ++i) {
    auto key = keys[i];
    while (tb != te && tb->first < key) {
      ++tb;
    }
    if (tb != te && tb->first == key) {
      result[i] = tb->second;
      continue;
    }
    // tb == te || tb->first > key
    // In other words, there is no transition on this key
    result[i] = otherwise_;
  }
}

bool DFANode::acceptsEverything() const {
  return accepting_ && transitions_.empty() && otherwise_ == this;
}

std::ostream &operator<<(std::ostream &s, const PatternChar &o) {
  return streamf(s, "%o(%o)", o.type_, o.ch_);
}

namespace internal {
std::ostream &operator<<(std::ostream &s, CharType ct) {
  static std::array<const char *, 4> charTypeNames = {"Exact", "Loose", "MatchOne", "MatchMany"};
  auto index = static_cast<size_t>(ct);
  passert(index < charTypeNames.size());
  return s << charTypeNames[index];
}
}  // namespace internal

namespace {
NDFAFactory::NDFAFactory(const PatternChar *begin, size_t patternSize) {
  std::vector<NDFANode::transition_t> empty;
  storage_.resize(patternSize +  1);
  auto *current = &storage_.back();
  *current = NDFANode(true, empty, nullptr, nullptr);

  // Work backwards to build our machine. Because we are working backwards, "next" is the node we
  // created in the previous step.
  for (auto rp = begin + patternSize; rp != begin; ) {  // The -- is inside
    --rp;
    auto *next = current--;
    switch (rp->type()) {
      case internal::CharType::Loose: {
        *current = createLoose(rp->ch(), next);
        break;
      }
      case internal::CharType::Exact: {
        std::vector<NDFANode::transition_t> transitions = {{rp->ch(), next}};
        *current = NDFANode(false, std::move(transitions), nullptr, nullptr);
        break;
      }
      case internal::CharType::MatchOne: {
        *current = NDFANode(false, empty, next, nullptr);
        break;
      }
      case internal::CharType::MatchN: {
        // Self-loop on every character
        *current = NDFANode(false, empty, current, next);
        break;
      }
      default:
        passert("Unexpected tag %o", rp->type());
    }
  }
  start_ = current;
}

NDFANode NDFAFactory::createLoose(char32_t ch, NDFANode *done) {
  auto asciiCh = static_cast<char>(ch);

  auto expansions = getFuzzyEquivalents(asciiCh);
  FailRoot fr;
  expansionsStorage_.clear();
  if (!tryConvertUtf8ToUtf32(expansions, &expansionsStorage_, fr.nest(HERE))) {
    crash("Impossible: failed on UTF-8 conversion %o", fr);
  }
  std::vector<NDFANode::transition_t> transitions;
  transitions.reserve(2 + expansionsStorage_.size());
  transitions.emplace_back(asciiCh, done);
  transitions.emplace_back(toupper(asciiCh), done);
  for (auto ch32 : expansionsStorage_) {
    transitions.emplace_back(ch32, done);
  }
  std::sort(transitions.begin(), transitions.end());
  return NDFANode(false, std::move(transitions), nullptr, nullptr);
}

Converter::Converter(NDFANode *start) {
  reusableBuilder_.add(start);
  auto *intermediateStart = lookupOrCreate(reusableBuilder_.makeKey());
  intermediateStart->isStart_ = true;

  while (!populateWork_.empty()) {
    auto ip = populateWork_.back();
    populateWork_.pop_back();
    populateIntermediateNode(ip->first, &ip->second);
  }
}

void Converter::populateIntermediateNode(const IntermediateKey &key, IntermediateNode *inode) {
  const auto size = key.size();
  const NDFANode::transition_t *currents[size];
  const NDFANode::transition_t *ends[size];
  for (size_t i = 0; i < size; ++i) {
    const auto &t = key.get(i)->transitions_;
    currents[i] = t.data();
    ends[i] = t.data() + t.size();
  }

  while (true) {
    // find minimum transition
    char32_t min = std::numeric_limits<char32_t>::max();
    for (size_t i = 0; i < size; ++i) {
      const auto *c = currents[i];
      const auto *e = ends[i];
      if (c != e && c->first < min) {
        min = c->first;
      }
    }

    if (min == std::numeric_limits<char32_t>::max()) {
      break;
    }

    reusableBuilder_.reset();
    for (size_t i = 0; i < size; ++i) {
      const auto *n = key.get(i);
      const auto *b = currents[i];
      const auto *e = ends[i];
      if (b != e) {
        if (b->first == min) {
          reusableBuilder_.add(b->second);
          ++currents[i];
          continue;
        }
      }
      // Either element i is exhausted or it doesn't have an entry for this transition. So add its
      // "otherwise" transition if it has one.
      if (n->otherwise_ != nullptr) {
        reusableBuilder_.add(n->otherwise_);
      }
    }

    auto *target = lookupOrCreate(reusableBuilder_.makeKey());
    inode->addTransition(min, target);
  }

  // Populate this node's "accepting" flag and "otherwise" transition
  bool accepting = false;
  reusableBuilder_.reset();
  for (size_t i = 0; i < size; ++i) {
    const auto *n = key.get(i);
    accepting |= n->accepting_;
    if (n->otherwise_ != nullptr) {
      reusableBuilder_.add(n->otherwise_);
    }
  }

  inode->accepting_ = accepting;
  if (!reusableBuilder_.empty()) {
    auto *target = lookupOrCreate(reusableBuilder_.makeKey());
    inode->addOtherwiseTransition(target);
  }
}

void Converter::squish() {
  squishWork_.reserve(intermediates_.size());
  for (auto &entry : intermediates_) {
    squishWork_.push_back(&entry.second);
  }

  while (!squishWork_.empty()) {
    auto *item = squishWork_.back();
    squishWork_.pop_back();

    auto [uniquep, inserted] = unique_.insert(item);
    if (inserted) {
      // Success!
      item->uniqueIterator_ = uniquep;
      continue;
    }

    // 'item' matches (by value) something already in unique_. So we can abandon item, but we'll
    // need to forward all of incoming its references to the object in unique_. Also, any object
    // that we modify thanks to this forwarding process will have to be ejected from unique and
    // reevaluated (moved back to squishWork_).
    forwardReferences(item, *uniquep);
  }
}

std::pair<std::vector<DFANode>, DFANode *> Converter::finish() {
  std::vector<DFANode> storage;
  storage.resize(unique_.size());
  auto *next = storage.data();
  for (auto *inode : unique_) {
    inode->dfaNode_ = next++;
  }
  DFANode *start = nullptr;
  for (const auto *inode : unique_) {
    std::vector<DFANode::transition_t> transitions;
    transitions.reserve(inode->transitions_.size());
    for (const auto &t : inode->transitions_) {
      transitions.emplace_back(t.first, t.second->dfaNode_);
    }
    auto *d = inode->dfaNode_;
    const auto *otherwiseToUse = inode->otherwise_ != nullptr ? inode->otherwise_->dfaNode_ : nullptr;
    *d = DFANode(inode->accepting_, std::move(transitions), otherwiseToUse);
    if (inode->isStart_) {
      if (start != nullptr) {
        crash("Multiple starts");
      }
      start = d;
    }
  }
  passert(start != nullptr);
  return std::make_pair(std::move(storage), start);
}

void Converter::forwardReferences(IntermediateNode *from, IntermediateNode *to) {
  if (from->isStart_) {
    to->isStart_ = true;
  }
  for (auto [incoming, incomingOffset] : from->incomingTransitions_) {
    // Evict because we're changing 'incoming'
    evictFromUnique(incoming);
    incoming->transitions_[incomingOffset].second = to;  // was 'from'
    to->incomingTransitions_.emplace_back(incoming, incomingOffset);
  }
  // The same thing, in principle
  for (auto *incoming : from->incomingOtherwises_) {
    // Evict because we're changing 'incoming'
    evictFromUnique(incoming);
    incoming->otherwise_ = to;  // was 'from'
    to->incomingOtherwises_.emplace_back(incoming);
  }
  // from->markDead();  // if you want
}

IntermediateNode *Converter::lookupOrCreate(IntermediateKey &&key) {
  auto [ip, inserted] = intermediates_.try_emplace(std::move(key), unique_.end());
  if (inserted) {
    populateWork_.push_back(ip);
  }
  return &ip->second;
}

void Converter::evictFromUnique(IntermediateNode *node) {
  if (node->uniqueIterator_ == unique_.end()) {
    return;
  }
  unique_.erase(node->uniqueIterator_);
  node->uniqueIterator_ = unique_.end();
  squishWork_.push_back(node);
}

std::ostream &operator<<(std::ostream &s, const IntermediateNode &o);

IntermediateNode::IntermediateNode(unique_t::iterator uniqueIterator) :
    uniqueIterator_(uniqueIterator) {
  // Get in a fight with Clang about unused functions.
  std::ostream &(*fp)(std::ostream &, const IntermediateNode &) = operator<<;
  (void)fp;
}
IntermediateNode::~IntermediateNode() = default;

void IntermediateNode::addTransition(char32_t ch, IntermediateNode *target) {
  if (!transitions_.empty() && transitions_.back().first >= ch) {
    crash("Added character %o out of order\n", ch);
  }
  auto offset = transitions_.size();
  transitions_.emplace_back(ch, target);
  if (target != this) {
    target->incomingTransitions_.emplace_back(this, offset);
  }
}

void IntermediateNode::addOtherwiseTransition(IntermediateNode *target) {
  if (otherwise_ != nullptr) {
    crash("Otherwise is already set!");
  }
  otherwise_ = target;
  if (target != this) {
    target->incomingOtherwises_.push_back(this);
  }
}

std::ostream &operator<<(std::ostream &s, const IntermediateNode &o) {
  streamf(s, "node=%o, accept=%o, numTrans=%o", &o, o.accepting_, o.transitions_.size());
  FailRoot fr;
  ReusableString8 rs8;
  for (const auto &kvp : o.transitions_) {
    if (!rs8.tryReset(kvp.first, fr.nest(HERE))) {
      crash("tryReset failed: %o", fr);
    }
    streamf(s, "\n%o - %o", rs8.storage(), kvp.second);
  }
  streamf(s, "\notherwise - %o", o.otherwise_);
  return s;
}

void IntermediateKeyBuilder::add(NDFANode *node) {
  for (; node != nullptr; node = node->empty_) {
    items_.push_back(node);
  }
}

IntermediateKey IntermediateKeyBuilder::makeKey() {
  std::sort(items_.begin(), items_.end());
  auto ip = std::unique(items_.begin(), items_.end());
  items_.erase(ip, items_.end());
  return IntermediateKey(std::move(items_));
}

bool tryRecursiveDump(std::ostream &s, const char *separator, const DFANode *node,
    std::set<const DFANode *> *beenHere, ReusableString8 *rs8, const FailFrame &ff) {
  if (!beenHere->insert(node).second) {
    return true;
  }
  streamf(s, "%onode=0x%o, accept=%o, everything=%o, numTrans=%o", separator,
      Hexer((uintptr_t)node), node->accepting(), node->acceptsEverything(),
      node->transitions().size());
  auto render = [rs8, &ff](std::ostream &s, char32_t item) {
    if (rs8->tryReset(item, ff.nest(HERE))) {
      s << rs8->storage();
    }
  };
  {
    std::map<const DFANode *, std::vector<char32_t>> reverse;
    for (const auto &entry : node->transitions()) {
      reverse[entry.second].push_back(entry.first);
    }
    std::vector<std::pair<std::vector<char32_t>, const DFANode *>> forward;
    forward.reserve(reverse.size());
    for (auto &kvp : reverse) {
      forward.emplace_back(std::move(kvp.second), kvp.first);
    }
    std::sort(forward.begin(), forward.end());
    for (const auto &[vec, target] : forward) {
      streamf(s, "\n%o - %o",
          kosak::coding::dump(vec.begin(), vec.end(), "[", "]", ", ", render),
          target);
    }
  }
  streamf(s, "\notherwise - 0x%o", Hexer((uintptr_t)node->otherwise()));
  for (const auto &entry : node->transitions()) {
    if (!tryRecursiveDump(s, "\n", entry.second, beenHere, rs8, ff.nest(HERE))) {
      return false;
    }
  }
  if (node->otherwise() != nullptr) {
    if (!tryRecursiveDump(s, "\n", node->otherwise(), beenHere, rs8, ff.nest(HERE))) {
      return false;
    }
  }
  return ff.ok();
}

std::ostream &operator<<(std::ostream &s, const NDFANode &o) {
  streamf(s, "NDFANode=%o, accept=%o, numTs=%o", &o, o.accepting_, o.transitions_.size());
  ReusableString8 rs8;
  for (const auto &kvp : o.transitions_) {
    FailRoot fr;
    if (rs8.tryReset(kvp.first, fr.nest(HERE))) {
      streamf(s, "\n%o - %o", rs8.storage(), kvp.second);
    } else {
      streamf(s, "FAIL: %o", fr);
    }
  }
  streamf(s, "\notherwise - %o", o.otherwise_);
  return s;
}

NDFANode::NDFANode() = default;
NDFANode::NDFANode(bool accepting, std::vector<transition_t> transitions, NDFANode *otherwise,
    NDFANode *empty) : accepting_(accepting), transitions_(std::move(transitions)),
    otherwise_(otherwise), empty_(empty) {
  // Get in a fight with Clang about unused functions.
  std::ostream &(*fp)(std::ostream &, const NDFANode &) = operator<<;
  (void)fp;
}
NDFANode::NDFANode(NDFANode &&) noexcept = default;
NDFANode & NDFANode::operator=(NDFANode &&) noexcept = default;
NDFANode::~NDFANode() = default;

inline uint32 swizzle(uintptr_t item) {
  static_assert(sizeof(uintptr_t) == 8);
  auto low = static_cast<uint32>(item);
  auto high = static_cast<uint32>(item >> 32U);
  return low ^ high;
}

// Return a special distinguished value if item == self
uintptr_t canonicalize(const IntermediateNode *item, const IntermediateNode *self) {
  if (item == self) {
    // Arbitrary integer. Note that because this is an odd number, it could never conflict with any
    // actual IntermediateNode*.
    const uintptr_t result = 0xe9cd708f4b33e595UL;
    static_assert(alignof(IntermediateNode) > 1);
    static_assert((result & 1U) == 1);
    return result;
  }

  auto result = reinterpret_cast<uintptr_t>(item);
  passert((result & 1U) == 0);
  return result;
}

// Our notion of value equality is basically structural equality (for the "Node" parts of the data
// structure: accepting, transitions, otherwise; not the extra helper stuff) EXCEPT that self pointers
// (transitions that point to self) will compare equal.  That is, if lhs has a transition to itself,
// and rhs has a transition to itself, those transitions will compare equal.
bool INodeEqual::operator()(const IntermediateNode *lhs,
    const IntermediateNode *rhs) const noexcept {
  if (lhs->accepting_ != rhs->accepting_) {
    return false;
  }
  if (lhs->transitions_.size() != rhs->transitions_.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs->transitions_.size(); ++i) {
    const auto &lt = lhs->transitions_[i];
    const auto &rt = rhs->transitions_[i];
    if (lt.first != rt.first) {
      return false;
    }
    if (canonicalize(lt.second, lhs) != canonicalize(rt.second, rhs)) {
      return false;
    }
  }
  return canonicalize(lhs->otherwise_, lhs) == canonicalize(rhs->otherwise_, rhs);
}

// The same rules apply to our getHashCode
// Worst getHashCode ever.
std::size_t INodeHash::operator()(const IntermediateNode *inode) const noexcept {
  const size_t rand0 = 0x12a8453097dbd558UL;
  const size_t rand1 = 0xe3402f4325a06b76UL;
  const size_t rand2 = 0x1f54cd3c2a5d168bUL;
  const size_t rand3 = 0x4d4b1bbee85d9403UL;
  const size_t rand4 = 0x37f66d30e0b022a3UL;
  size_t result = inode->accepting_ ? rand0 : rand1;
  for (const auto &tran : inode->transitions_) {
    result = (result * rand2) + swizzle(tran.first);
    result = (result * rand3) + swizzle(canonicalize(tran.second, inode));
  }
  result = (result * rand4) + swizzle(canonicalize(inode->otherwise_, inode));
  return result;
}

std::size_t IKeyHash::operator()(const IntermediateKey &key) const noexcept {
  const size_t rand0 = 0xe78c1e9fc23700a7UL;
  const size_t rand1 = 0xb0f8b89f99d0b098UL;
  size_t result = rand0;
  for (const auto *n : key.items()) {
    result = (result * rand1) + swizzle(reinterpret_cast<uintptr_t>(n));
  }
  return result;
}
}  // namespace
}  // namespace z2kplus::backend::util
