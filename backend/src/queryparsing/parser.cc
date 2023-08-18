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

#include "z2kplus/backend/queryparsing/parser.h"

#include <iostream>
#include <string_view>
#include <antlr4-runtime/ANTLRInputStream.h>
#include <antlr4-runtime/BaseErrorListener.h>
#include <antlr4-runtime/CommonTokenStream.h>
#include <antlr4-runtime/tree/ParseTreeWalker.h>
#include <antlr4-runtime/tree/TerminalNode.h>
#include "./generated/ZarchiveLexer.h"
#include "./generated/ZarchiveParser.h"
#include "./generated/ZarchiveParserBaseListener.h"
#include "kosak/coding/coding.h"
#include "kosak/coding/text/conversions.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/text/misc.h"
#include "z2kplus/backend/queryparsing/util.h"
#include "z2kplus/backend/reverse_index/fields.h"
#include "z2kplus/backend/reverse_index/iterators/word/anchored.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/and.h"
#include "z2kplus/backend/reverse_index/iterators/iterator_common.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/metadata/having_reaction.h"
#include "z2kplus/backend/reverse_index/iterators/boundary/near.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/not.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/or.h"
#include "z2kplus/backend/reverse_index/iterators/word/pattern.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/popornot.h"
#include "z2kplus/backend/reverse_index/iterators/zgram/zgramid.h"
#include "z2kplus/backend/shared/magic_constants.h"
#include "z2kplus/backend/shared/zephyrgram.h"
#include "z2kplus/backend/util/automaton/automaton.h"

namespace magicConstants = z2kplus::backend::shared::magicConstants;

using antlr4::ANTLRInputStream;
using antlr4::BaseErrorListener;
using antlr4::CommonTokenStream;
using antlr4::tree::ParseTree;
using antlr4::tree::ParseTreeWalker;
using antlr4::tree::TerminalNode;
using kosak::coding::FailFrame;
using kosak::coding::FailRoot;
using kosak::coding::text::trim;
using kosak::coding::text::tryParseDecimal;
using kosak::coding::text::tryConvertUtf8ToUtf32;
using z2kplus::backend::queryparsing::generated::ZarchiveLexer;
using z2kplus::backend::queryparsing::generated::ZarchiveParser;
using z2kplus::backend::queryparsing::generated::ZarchiveParserBaseListener;
using z2kplus::backend::queryparsing::WordSplitter;
using z2kplus::backend::reverse_index::FieldMask;
using z2kplus::backend::reverse_index::FieldTag;
using z2kplus::backend::reverse_index::tryParse;
using z2kplus::backend::reverse_index::iterators::Anchored;
using z2kplus::backend::reverse_index::iterators::And;
using z2kplus::backend::reverse_index::iterators::zgram::metadata::HavingReaction;
using z2kplus::backend::reverse_index::iterators::boundary::Near;
using z2kplus::backend::reverse_index::iterators::Not;
using z2kplus::backend::reverse_index::iterators::Or;
using z2kplus::backend::reverse_index::iterators::zgram::PopOrNot;
using z2kplus::backend::reverse_index::iterators::WordIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIterator;
using z2kplus::backend::reverse_index::iterators::ZgramIdIterator;
using z2kplus::backend::reverse_index::iterators::word::Pattern;
using z2kplus::backend::shared::ZgramId;
using z2kplus::backend::util::automaton::FiniteAutomaton;
using z2kplus::backend::util::automaton::PatternChar;

#define HERE KOSAK_CODING_HERE

namespace z2kplus::backend::queryparsing {

namespace {
class MyParseListener final : public ZarchiveParserBaseListener {
public:
  MyParseListener();
  ~MyParseListener() final;

  bool tryGetResult(std::unique_ptr<ZgramIterator> *result, const FailFrame &ff);
  
  void exitQuery(ZarchiveParser::QueryContext *context) final;

  void exitMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext *context) final;
  void exitMetadataFnArgsWholeNumber(ZarchiveParser::MetadataFnArgsWholeNumberContext *context) final;
  void exitAdj(ZarchiveParser::AdjContext *context) final;
  void exitNot(ZarchiveParser::NotContext *context) final;
  void exitImpliedAnd(ZarchiveParser::ImpliedAndContext *context) final;
  void exitAnd(ZarchiveParser::AndContext *context) final;
  void exitOr(ZarchiveParser::OrContext *context) final;
  void exitParens(ZarchiveParser::ParensContext *context) final;

  void exitScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext *context) final;
  void exitAdjacentWords(ZarchiveParser::AdjacentWordsContext *context) final;
  void exitQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext *context) final;
  void exitTildedAdjacency(ZarchiveParser::TildedAdjacencyContext *context) final;
  void exitLiterally(ZarchiveParser::LiterallyContext *context) final;

private:
  void nodesToAdjacencyGroups(size_t nearMargin, antlr4::tree::TerminalNode **begin,
    antlr4::tree::TerminalNode **end);

  std::unique_ptr<ZgramIterator> query_;
  std::vector<std::unique_ptr<ZgramIterator>> zgramIterators_;
  std::vector<std::unique_ptr<ZgramIterator>> adjacencies_;
  // Near margin, words
  std::vector<std::pair<uint32, std::vector<std::string>>> adjacencyGroups_;
  FailRoot failRoot_;
};

class MyErrorListener final : public BaseErrorListener {
public:
  MyErrorListener();
  ~MyErrorListener() final;
  void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
    size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override;

  bool checkOk(const FailFrame &ff);

private:
  FailRoot failRoot_;
};
}  // namespace

bool parse(std::string_view text, bool emptyMeansEverything, std::unique_ptr<ZgramIterator> *result,
  const FailFrame &ff) {
  text = trim(text);
  if (text.empty()) {
    auto fm = emptyMeansEverything ? FieldMask::all : FieldMask::none;
    *result = PopOrNot::create(fm, fm);
    return true;
  }
  ANTLRInputStream inputStream(text.data(), text.size());
  // create a lexer that feeds off of inputStream
  ZarchiveLexer lexer(&inputStream);
  // create a buffer of tokens pulled from the lexer
  CommonTokenStream tokens(&lexer);
  // create a parser that feeds off the tokens buffer
  ZarchiveParser parser(&tokens);

  // Listen for errors.
  MyErrorListener myErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&myErrorListener);

  parser.removeErrorListeners();
  parser.addErrorListener(&myErrorListener);
  auto tree = parser.query();

  MyParseListener parseListener;
  ParseTreeWalker::DEFAULT.walk(&parseListener, tree);
  return myErrorListener.checkOk(ff.nest(HERE)) && parseListener.tryGetResult(result, ff.nest(HERE));
}

namespace {
bool maybeGetString(TerminalNode *node, std::string *result);
bool tryGetString(TerminalNode *node, std::string *result, const FailFrame &ff);
bool tryStripQuotesAndUnescape(std::string *s, const FailFrame &ff);

template<typename T>
bool tryGetTopN(std::vector<T> *src, std::vector<T> *target, size_t count, const FailFrame &ff);

template<typename T>
bool tryGetTop(std::vector<T> *src, T *target, const FailFrame &ff);

bool expectEmptyHelper(const char *name, size_t actualSize, const FailFrame &ff);

template<typename T>
bool expectEmpty(const char *name, const std::vector<T> &vec, const FailFrame &ff) {
  return expectEmptyHelper(name, vec.size(), ff.nest(HERE));
}

bool tryParseFieldMask(std::string_view fieldText, FieldMask *result, const FailFrame &ff);

MyParseListener::MyParseListener() = default;
MyParseListener::~MyParseListener() = default;

bool MyParseListener::tryGetResult(std::unique_ptr<ZgramIterator> *result, const FailFrame &ffResult) {
  if (query_ == nullptr) {
    (void)failRoot_.fail(HERE, "Expected parse result value, but there was nothing.");
  }
  (void)expectEmpty("zgramIterators", zgramIterators_, failRoot_.nest(HERE));
  (void)expectEmpty("adjacencies", adjacencies_, failRoot_.nest(HERE));
  (void)expectEmpty("simpleOrQuotedAdjacencyGroups", adjacencyGroups_, failRoot_.nest(HERE));
  if (!failRoot_.ok()) {
    return ffResult.failf(HERE, "%o", failRoot_);
  }
  *result = std::move(query_);
  return true;
}

// query: WS? booleanQuery WS? EOF ;
//
// input: zgramIterators_ stack (there should be exactly one)
// result of evaluation: query_
void MyParseListener::exitQuery(ZarchiveParser::QueryContext */*context*/) {
  tryGetTop(&zgramIterators_, &query_, failRoot_.nest(HERE));
}

//booleanQuery:
//// e.g. hasreaction("#C++")
//METADATA_FN_ARGS_STRING WS* LITERAL WS* RPAREN # metadataFnArgsString |
//// e.g. threadid(12345)
//METADATA_FN_ARGS_WHOLE_NUMBER WS* WHOLE_NUMBER WS* RPAREN # metadataFnArgsWholeNumber |
//// e.g. sender: kosak
//scopedAdjacency #adj |
//// e.g. not sender: kosak
//NOT WS booleanQuery #not |
//// Whitespace between booleanQueries gets you implied AND
//// e.g. sender:kosak instance:love  // AND(sender:kosak, instance: love)
//booleanQuery WS booleanQuery #impliedAnd |
//// e.g. kosak and not cinnabon
//booleanQuery WS AND WS booleanQuery #and |
//// e.g. kosak or cinnabon
//booleanQuery WS OR WS booleanQuery #or |
//// e.g. (sender:kosak and instance:C++) and not likedby("agroce")
//LPAREN WS? booleanQuery WS? RPAREN #parens ;
//
// result of evaluation: zgramIterators_ stack
//
// First rule:
// METADATA_FN_ARGS_STRING WS* LITERAL WS* RPAREN # metadataFnArgsString |
// inputs: various tokens
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext *context) {
  std::string functionName;
  std::string literal;
  if (!tryGetString(context->METADATA_FN_ARGS_STRING(), &functionName, failRoot_.nest(HERE)) ||
    !tryGetString(context->LITERAL(), &literal, failRoot_.nest(HERE)) ||
    !tryStripQuotesAndUnescape(&literal, failRoot_.nest(HERE))) {
    return;
  }
  std::unique_ptr<ZgramIterator> it;
  if (functionName == "hasreaction(") {
    it = HavingReaction::create(std::move(literal));
  } else {
    (void)failRoot_.failf(HERE, "Can't happen. FunctionName %o", functionName);
    return;
  }
  zgramIterators_.push_back(std::move(it));
}

void MyParseListener::exitMetadataFnArgsWholeNumber(
  ZarchiveParser::MetadataFnArgsWholeNumberContext *context) {
  std::string functionName;
  std::string literal;
  if (!tryGetString(context->METADATA_FN_ARGS_WHOLE_NUMBER(), &functionName, failRoot_.nest(HERE)) ||
      !tryGetString(context->WHOLE_NUMBER(), &literal, failRoot_.nest(HERE))) {
    return;
  }
  uint64 argument;
  if (!tryParseDecimal(literal, &argument, nullptr, failRoot_.nest(HERE))) {
    return;
  }
  std::unique_ptr<ZgramIterator> it;
  if (functionName == "zgramid(") {
    it = ZgramIdIterator::create(ZgramId(argument));
  } else {
    (void)failRoot_.failf(HERE, "Can't happen. FunctionName %o", functionName);
    return;
  }
  zgramIterators_.push_back(std::move(it));
}

// Next rule:
// scopedAdjacency #adj
// input: the top of the adjacencies_ stack
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitAdj(ZarchiveParser::AdjContext */*context*/) {
  std::unique_ptr<ZgramIterator> result;
  if (!tryGetTop(&adjacencies_, &result, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(std::move(result));
}

// NOT WS booleanQuery #not
// input: the top of the zgramIterators_ stack
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitNot(ZarchiveParser::NotContext */*context*/) {
  std::unique_ptr<ZgramIterator> target;
  if (!tryGetTop(&zgramIterators_, &target, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(Not::create(std::move(target)));
}

// Next rule:
// booleanQuery WS booleanQuery #adj
// input: the top two items of the zgramIterators_ stack
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitImpliedAnd(ZarchiveParser::ImpliedAndContext */*context*/) {
  std::vector<std::unique_ptr<ZgramIterator>> children;
  if (!tryGetTopN(&zgramIterators_, &children, 2, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(And::create(std::move(children)));
}

// booleanQuery WS AND WS booleanQuery #and
// input: the top two items of the zgramIterators_ stack
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitAnd(ZarchiveParser::AndContext */*context*/) {
  std::vector<std::unique_ptr<ZgramIterator>> children;
  if (!tryGetTopN(&zgramIterators_, &children, 2, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(And::create(std::move(children)));
}

// booleanQuery WS OR WS booleanQuery #or
// input: the top two items of the zgramIterators_ stack
// results: push 1 item to zgramIterators_ stack
void MyParseListener::exitOr(ZarchiveParser::OrContext */*context*/) {
  std::vector<std::unique_ptr<ZgramIterator>> children;
  if (!tryGetTopN(&zgramIterators_, &children, 2, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(Or::create(std::move(children)));
}

// LPAREN WS? booleanQuery WS? RPAREN #parens
// input: the top two item of the zgramIterators_ stack
// results: push that same item back to the zgramIterators_ stack
// Note: this whole method is basically a no-op, but we pop an item just to make sure one is there.
void MyParseListener::exitParens(ZarchiveParser::ParensContext */*context*/) {
  std::unique_ptr<ZgramIterator> result;
  if (!tryGetTop(&zgramIterators_, &result, failRoot_.nest(HERE))) {
    return;
  }
  zgramIterators_.push_back(std::move(result));
}

// scopedAdjacency: (FIELD_SPECIFICATION WS?)? LANCHOR?
//   (simpleAdjacency | quotedAdjacency | tildedAdjacency | literally)
//   RANCHOR?;
// inputs: the top of the adjacencyGroups_ stack.
// results: pushes 1 item to the adjacencies_ stack.
void MyParseListener::exitScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext *context) {
  std::string fieldTextStorage;
  std::string_view fieldText;
  if (maybeGetString(context->FIELD_SPECIFICATION(), &fieldTextStorage)) {
    fieldText = fieldTextStorage;
  } else {
    fieldText = "instance,body:";
  }
  FieldMask fieldMask;
  std::pair<uint32, std::vector<std::string>> group;
  if (!tryParseFieldMask(fieldText, &fieldMask, failRoot_.nest(HERE)) ||
    !tryGetTop(&adjacencyGroups_, &group, failRoot_.nest(HERE))) {
    return;
  }
  std::u32string text32;
  std::vector<std::unique_ptr<WordIterator>> wordIterators;
  std::vector<PatternChar> patternChars;
  for (auto &s : group.second) {
    text32.clear();
    patternChars.clear();
    if (!tryConvertUtf8ToUtf32(s, &text32, failRoot_.nest(HERE))) {
      continue;
    }
    WordSplitter::translateToPatternChar(text32, &patternChars);
    FiniteAutomaton dfa(patternChars.data(), patternChars.size(), std::move(s));
    auto pattern = Pattern::create(std::move(dfa), fieldMask);
    wordIterators.push_back(std::move(pattern));
  }
  if (group.second.size() != wordIterators.size()) {
    // There were some failures
    return;
  }
  if (wordIterators.empty()) {
    // Sometimes our users like to say things like signature:""  (empty signature)
    auto result = PopOrNot::create(FieldMask::none, fieldMask);
    adjacencies_.push_back(std::move(result));
    return;
  }
  // If size is 1, then these two clauses operate on the same item, which works out fine.
  if (context->LANCHOR() != nullptr) {
    auto temp = Anchored::create(std::move(wordIterators.front()), true, false);
    wordIterators.front() = std::move(temp);
  }
  if (context->RANCHOR() != nullptr) {
    auto temp = Anchored::create(std::move(wordIterators.back()), false, true);
    wordIterators.back() = std::move(temp);
  }
  auto result = Near::create(group.first, std::move(wordIterators));
  adjacencies_.push_back(std::move(result));
}

// adjacentWords: UNQUOTED_WORD_OR_PUNCT+ ;
// inputs: the words.
// results: pushes 1 item to the adjacencyGroups_ stack.
void MyParseListener::exitAdjacentWords(ZarchiveParser::AdjacentWordsContext *context) {
  auto wordNodes = context->UNQUOTED_WORD_OR_PUNCT();
  nodesToAdjacencyGroups(1, &*wordNodes.begin(), &*wordNodes.end());
}

// quotedAdjacency: QUOTE QUOTED_WS? (QUOTED_WORD_OR_PUNCT QUOTED_WS?)* ENDQUOTE ;
// inputs: all the quoted words.
// results: pushes 1 item to the adjacencyGroups_ stack.
void MyParseListener::exitQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext *context) {
  auto wordNodes = context->QUOTED_WORD_OR_PUNCT();
  nodesToAdjacencyGroups(1, &*wordNodes.begin(), &*wordNodes.end());
}

// tildedAdjacency: TILDE TILDED_WS? (TILDED_WORD_OR_PUNCT TILDED_WS?)* ENDTILDE ;
// inputs: all the tilded words.
// results: pushes 1 item to the adjacencyGroups_ stack.
void MyParseListener::exitTildedAdjacency(ZarchiveParser::TildedAdjacencyContext *context) {
  auto wordNodes = context->TILDED_WORD_OR_PUNCT();
  nodesToAdjacencyGroups(magicConstants::nearMargin, &*wordNodes.begin(), &*wordNodes.end());
}

// literally: LITERALLY WS* LITERAL WS* RPAREN ;
// inputs: the literal string (which we split into tokens)
// results: pushes 1 item to the adjacencyGroups_ stack.
void MyParseListener::exitLiterally(ZarchiveParser::LiterallyContext *context) {
  std::string literal;
  if (!tryGetString(context->LITERAL(), &literal, failRoot_.nest(HERE)) ||
    !tryStripQuotesAndUnescape(&literal, failRoot_.nest(HERE))) {
    return;
  }
  std::vector<std::string_view> svWords;
  WordSplitter::split(literal, &svWords);
  std::vector<std::string> words(svWords.begin(), svWords.end());
  adjacencyGroups_.emplace_back(1, std::move(words));
}

void MyParseListener::nodesToAdjacencyGroups(size_t nearMargin, antlr4::tree::TerminalNode **begin,
  antlr4::tree::TerminalNode **end) {
  std::vector<std::string> words;
  for ( ; begin != end; ++begin) {
    std::string next;
    if (!tryGetString(*begin, &next, failRoot_.nest(HERE))) {
      return;
    }
    words.push_back(std::move(next));
  }
  adjacencyGroups_.emplace_back(nearMargin, std::move(words));
}

MyErrorListener::MyErrorListener() = default;
MyErrorListener::~MyErrorListener() = default;

void MyErrorListener::syntaxError(antlr4::Recognizer */*recognizer*/,
  antlr4::Token */*offendingSymbol*/, size_t /*line*/, size_t charPositionInLine,
  const std::string &msg, std::exception_ptr /*e*/) {
  (void)failRoot_.failf(HERE, "At position %o: %o", charPositionInLine, msg);
}

bool MyErrorListener::checkOk(const FailFrame &ffResult) {
  if (!failRoot_.ok()) {
    return ffResult.failf(HERE, "%o", failRoot_);
  }
  return true;
}

bool maybeGetString(TerminalNode *node, std::string *result) {
  if (node == nullptr) {
    return false;
  }
  *result = node->getText();
  return true;
}

bool tryGetString(TerminalNode *node, std::string *result, const FailFrame &ff) {
  if (!maybeGetString(node, result)) {
    return ff.fail(HERE, "Tried to get a string from a TerminalNode, but failed.");
  }
  return true;
}

bool tryStripQuotesAndUnescape(std::string *s, const FailFrame &ff) {
  if (s->size() < 2 || s->front() != '"' || s->back() != '"') {
    return ff.failf(HERE, "%o doesn't appear to be surrounded by quotes", s);
  }
  auto src = s->begin() + 1;
  auto end = s->end() - 1;
  auto dest = s->begin();
  for ( ; src != end; ++src) {
    char ch = *src;
    if (ch == '\\') {
      if (++src == end) {
        break;
      }
      ch = *src;
    }
    *dest++ = ch;
  }
  s->erase(dest, s->end());
  return true;
}

template<typename T>
bool tryGetTopN(std::vector<T> *src, std::vector<T> *target, size_t count, const FailFrame &ff) {
  if (src->size() < count) {
    return ff.failf(HERE, "Needed %o entries but only %o were available", count, src->size());
    return false;
  }
  size_t offset = src->size() - count;
  auto begin = src->begin() + offset;
  auto end = src->end();
  target->insert(target->end(), std::make_move_iterator(begin), std::make_move_iterator(end));
  src->erase(begin, end);
  return true;
}

template<typename T>
bool tryGetTop(std::vector<T> *src, T *target, const FailFrame &ff) {
  std::vector<T> temp;
  if (!tryGetTopN(src, &temp, 1, ff.nest(HERE))) {
    return false;
  }
  *target = std::move(temp[0]);
  return true;
}

bool tryParseFieldMask(std::string_view fieldText, FieldMask *result, const FailFrame &ff) {
  uint32_t rawResult = 0;
  while (!fieldText.empty()) {
    auto next = fieldText.find_first_of(" ,:");
    if (next == std::string_view::npos) {
      break;
    }
    auto sr = fieldText.substr(0, next);
    fieldText = fieldText.substr(next + 1);
    FieldTag fieldTag;
    if (!tryParse(sr, &fieldTag)) {
      return ff.failf(HERE, "Unrecognized field tag %o", sr);
    }
    rawResult |= 1U << (uint32_t)fieldTag;
  }
  *result = static_cast<FieldMask>(rawResult);
  return true;
}

bool expectEmptyHelper(const char *name, size_t actualSize, const FailFrame &ff) {
  if (actualSize == 0) {
    return true;
  }
  return ff.failf(HERE, "For %o, expected empty stack, but have %o items.", name, actualSize);
}

}  // namespace
}  // namespace z2kplus::backend::queryparsing
