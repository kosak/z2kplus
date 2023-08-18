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


namespace z2kplus::backend::queryparsing::generated {


class  ZarchiveParser : public antlr4::Parser {
public:
  enum {
    METADATA_FN_ARGS_STRING = 1, METADATA_FN_ARGS_WHOLE_NUMBER = 2, LITERALLY = 3, 
    AND = 4, OR = 5, NOT = 6, LPAREN = 7, RPAREN = 8, LANCHOR = 9, RANCHOR = 10, 
    FIELD_SPECIFICATION = 11, UNQUOTED_WORD_OR_PUNCT = 12, QUOTE = 13, TILDE = 14, 
    WS = 15, QUOTED_WORD_OR_PUNCT = 16, QUOTED_WS = 17, ENDQUOTE = 18, TILDED_WORD_OR_PUNCT = 19, 
    TILDED_WS = 20, ENDTILDE = 21, LITERAL = 22, WHOLE_NUMBER = 23
  };

  enum {
    RuleQuery = 0, RuleBooleanQuery = 1, RuleScopedAdjacency = 2, RuleAdjacentWords = 3, 
    RuleQuotedAdjacency = 4, RuleTildedAdjacency = 5, RuleLiterally = 6
  };

  explicit ZarchiveParser(antlr4::TokenStream *input);

  ZarchiveParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~ZarchiveParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class QueryContext;
  class BooleanQueryContext;
  class ScopedAdjacencyContext;
  class AdjacentWordsContext;
  class QuotedAdjacencyContext;
  class TildedAdjacencyContext;
  class LiterallyContext; 

  class  QueryContext : public antlr4::ParserRuleContext {
  public:
    QueryContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    BooleanQueryContext *booleanQuery();
    antlr4::tree::TerminalNode *EOF();
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  QueryContext* query();

  class  BooleanQueryContext : public antlr4::ParserRuleContext {
  public:
    BooleanQueryContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    BooleanQueryContext() = default;
    void copyFrom(BooleanQueryContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  MetadataFnArgsWholeNumberContext : public BooleanQueryContext {
  public:
    MetadataFnArgsWholeNumberContext(BooleanQueryContext *ctx);

    antlr4::tree::TerminalNode *METADATA_FN_ARGS_WHOLE_NUMBER();
    antlr4::tree::TerminalNode *WHOLE_NUMBER();
    antlr4::tree::TerminalNode *RPAREN();
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  NotContext : public BooleanQueryContext {
  public:
    NotContext(BooleanQueryContext *ctx);

    antlr4::tree::TerminalNode *NOT();
    antlr4::tree::TerminalNode *WS();
    BooleanQueryContext *booleanQuery();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ParensContext : public BooleanQueryContext {
  public:
    ParensContext(BooleanQueryContext *ctx);

    antlr4::tree::TerminalNode *LPAREN();
    BooleanQueryContext *booleanQuery();
    antlr4::tree::TerminalNode *RPAREN();
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  OrContext : public BooleanQueryContext {
  public:
    OrContext(BooleanQueryContext *ctx);

    std::vector<BooleanQueryContext *> booleanQuery();
    BooleanQueryContext* booleanQuery(size_t i);
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);
    antlr4::tree::TerminalNode *OR();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  AdjContext : public BooleanQueryContext {
  public:
    AdjContext(BooleanQueryContext *ctx);

    ScopedAdjacencyContext *scopedAdjacency();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  AndContext : public BooleanQueryContext {
  public:
    AndContext(BooleanQueryContext *ctx);

    std::vector<BooleanQueryContext *> booleanQuery();
    BooleanQueryContext* booleanQuery(size_t i);
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);
    antlr4::tree::TerminalNode *AND();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  MetadataFnArgsStringContext : public BooleanQueryContext {
  public:
    MetadataFnArgsStringContext(BooleanQueryContext *ctx);

    antlr4::tree::TerminalNode *METADATA_FN_ARGS_STRING();
    antlr4::tree::TerminalNode *LITERAL();
    antlr4::tree::TerminalNode *RPAREN();
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  class  ImpliedAndContext : public BooleanQueryContext {
  public:
    ImpliedAndContext(BooleanQueryContext *ctx);

    std::vector<BooleanQueryContext *> booleanQuery();
    BooleanQueryContext* booleanQuery(size_t i);
    antlr4::tree::TerminalNode *WS();
    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
  };

  BooleanQueryContext* booleanQuery();
  BooleanQueryContext* booleanQuery(int precedence);
  class  ScopedAdjacencyContext : public antlr4::ParserRuleContext {
  public:
    ScopedAdjacencyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AdjacentWordsContext *adjacentWords();
    QuotedAdjacencyContext *quotedAdjacency();
    TildedAdjacencyContext *tildedAdjacency();
    LiterallyContext *literally();
    antlr4::tree::TerminalNode *FIELD_SPECIFICATION();
    antlr4::tree::TerminalNode *LANCHOR();
    antlr4::tree::TerminalNode *RANCHOR();
    antlr4::tree::TerminalNode *WS();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  ScopedAdjacencyContext* scopedAdjacency();

  class  AdjacentWordsContext : public antlr4::ParserRuleContext {
  public:
    AdjacentWordsContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> UNQUOTED_WORD_OR_PUNCT();
    antlr4::tree::TerminalNode* UNQUOTED_WORD_OR_PUNCT(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  AdjacentWordsContext* adjacentWords();

  class  QuotedAdjacencyContext : public antlr4::ParserRuleContext {
  public:
    QuotedAdjacencyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *QUOTE();
    antlr4::tree::TerminalNode *ENDQUOTE();
    std::vector<antlr4::tree::TerminalNode *> QUOTED_WS();
    antlr4::tree::TerminalNode* QUOTED_WS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> QUOTED_WORD_OR_PUNCT();
    antlr4::tree::TerminalNode* QUOTED_WORD_OR_PUNCT(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  QuotedAdjacencyContext* quotedAdjacency();

  class  TildedAdjacencyContext : public antlr4::ParserRuleContext {
  public:
    TildedAdjacencyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *TILDE();
    antlr4::tree::TerminalNode *ENDTILDE();
    std::vector<antlr4::tree::TerminalNode *> TILDED_WS();
    antlr4::tree::TerminalNode* TILDED_WS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> TILDED_WORD_OR_PUNCT();
    antlr4::tree::TerminalNode* TILDED_WORD_OR_PUNCT(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  TildedAdjacencyContext* tildedAdjacency();

  class  LiterallyContext : public antlr4::ParserRuleContext {
  public:
    LiterallyContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LITERALLY();
    antlr4::tree::TerminalNode *LITERAL();
    antlr4::tree::TerminalNode *RPAREN();
    std::vector<antlr4::tree::TerminalNode *> WS();
    antlr4::tree::TerminalNode* WS(size_t i);

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;
   
  };

  LiterallyContext* literally();


  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool booleanQuerySempred(BooleanQueryContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

}  // namespace z2kplus::backend::queryparsing::generated
