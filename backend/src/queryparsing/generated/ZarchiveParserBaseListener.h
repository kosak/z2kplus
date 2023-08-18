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
#include "ZarchiveParserListener.h"


namespace z2kplus::backend::queryparsing::generated {

/**
 * This class provides an empty implementation of ZarchiveParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  ZarchiveParserBaseListener : public ZarchiveParserListener {
public:

  virtual void enterQuery(ZarchiveParser::QueryContext * /*ctx*/) override { }
  virtual void exitQuery(ZarchiveParser::QueryContext * /*ctx*/) override { }

  virtual void enterMetadataFnArgsWholeNumber(ZarchiveParser::MetadataFnArgsWholeNumberContext * /*ctx*/) override { }
  virtual void exitMetadataFnArgsWholeNumber(ZarchiveParser::MetadataFnArgsWholeNumberContext * /*ctx*/) override { }

  virtual void enterNot(ZarchiveParser::NotContext * /*ctx*/) override { }
  virtual void exitNot(ZarchiveParser::NotContext * /*ctx*/) override { }

  virtual void enterParens(ZarchiveParser::ParensContext * /*ctx*/) override { }
  virtual void exitParens(ZarchiveParser::ParensContext * /*ctx*/) override { }

  virtual void enterOr(ZarchiveParser::OrContext * /*ctx*/) override { }
  virtual void exitOr(ZarchiveParser::OrContext * /*ctx*/) override { }

  virtual void enterAdj(ZarchiveParser::AdjContext * /*ctx*/) override { }
  virtual void exitAdj(ZarchiveParser::AdjContext * /*ctx*/) override { }

  virtual void enterAnd(ZarchiveParser::AndContext * /*ctx*/) override { }
  virtual void exitAnd(ZarchiveParser::AndContext * /*ctx*/) override { }

  virtual void enterMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext * /*ctx*/) override { }
  virtual void exitMetadataFnArgsString(ZarchiveParser::MetadataFnArgsStringContext * /*ctx*/) override { }

  virtual void enterImpliedAnd(ZarchiveParser::ImpliedAndContext * /*ctx*/) override { }
  virtual void exitImpliedAnd(ZarchiveParser::ImpliedAndContext * /*ctx*/) override { }

  virtual void enterScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext * /*ctx*/) override { }
  virtual void exitScopedAdjacency(ZarchiveParser::ScopedAdjacencyContext * /*ctx*/) override { }

  virtual void enterAdjacentWords(ZarchiveParser::AdjacentWordsContext * /*ctx*/) override { }
  virtual void exitAdjacentWords(ZarchiveParser::AdjacentWordsContext * /*ctx*/) override { }

  virtual void enterQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext * /*ctx*/) override { }
  virtual void exitQuotedAdjacency(ZarchiveParser::QuotedAdjacencyContext * /*ctx*/) override { }

  virtual void enterTildedAdjacency(ZarchiveParser::TildedAdjacencyContext * /*ctx*/) override { }
  virtual void exitTildedAdjacency(ZarchiveParser::TildedAdjacencyContext * /*ctx*/) override { }

  virtual void enterLiterally(ZarchiveParser::LiterallyContext * /*ctx*/) override { }
  virtual void exitLiterally(ZarchiveParser::LiterallyContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

}  // namespace z2kplus::backend::queryparsing::generated
