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


// Generated from ZarchiveParser.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"
#include "ZarchiveParser.h"


namespace z2kplus::backend::queryparsing::generated {

/**
 * This interface defines an abstract listener for a parse tree produced by ZarchiveParser.
 */
class  ZarchiveParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterQuery(ZarchiveParser::QueryContext *ctx) = 0;
  virtual void exitQuery(ZarchiveParser::QueryContext *ctx) = 0;

  virtual void enterMetadataFnArgsWholeNumber(ZarchiveParser::MetadataFnArgsWholeNumberContext *ctx) = 0;
  virtual void exitMetadataFnArgsWholeNumber(ZarchiveParser::MetadataFnArgsWholeNumberContext *ctx) = 0;

  virtual void enterNot(ZarchiveParser::NotContext *ctx) = 0;
  virtual void exitNot(ZarchiveParser::NotContext *ctx) = 0;

  virtual void enterParens(ZarchiveParser::ParensContext *ctx) = 0;
  virtual void exitParens(ZarchiveParser::ParensContext *ctx) = 0;

  virtual void enterOr(ZarchiveParser::OrContext *ctx) = 0;
  virtual void exitOr(ZarchiveParser::OrContext *ctx) = 0;

  virtual void enterAdj(ZarchiveParser::AdjContext *ctx) = 0;
  virtual void exitAdj(ZarchiveParser::AdjContext *ctx) = 0;

  virtual void enterAnd(ZarchiveParser::AndContext *ctx) = 0;
  virtual void exitAnd(ZarchiveParser::AndContext *ctx) = 0;

  virtual void enterMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext *ctx) = 0;
  virtual void exitMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext *ctx) = 0;

  virtual void enterImpliedAnd(ZarchiveParser::ImpliedAndContext *ctx) = 0;
  virtual void exitImpliedAnd(ZarchiveParser::ImpliedAndContext *ctx) = 0;

  virtual void enterScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext *ctx) = 0;
  virtual void exitScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext *ctx) = 0;

  virtual void enterAdjacentWords(ZarchiveParser::AdjacentWordsContext *ctx) = 0;
  virtual void exitAdjacentWords(ZarchiveParser::AdjacentWordsContext *ctx) = 0;

  virtual void enterQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext *ctx) = 0;
  virtual void exitQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext *ctx) = 0;

  virtual void enterTildedAdjacency(ZarchiveParser::TildedAdjacencyContext *ctx) = 0;
  virtual void exitTildedAdjacency(ZarchiveParser::TildedAdjacencyContext *ctx) = 0;

  virtual void enterLiterally(ZarchiveParser::LiterallyContext *ctx) = 0;
  virtual void exitLiterally(ZarchiveParser::LiterallyContext *ctx) = 0;


};

}  // namespace z2kplus::backend::queryparsing::generated
