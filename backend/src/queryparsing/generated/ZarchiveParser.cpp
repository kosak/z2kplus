
// Generated from ZarchiveParser.g4 by ANTLR 4.11.1


#include "ZarchiveParserListener.h"

#include "ZarchiveParser.h"


using namespace antlrcpp;
using namespace z2kplus::backend::queryparsing::generated;

using namespace antlr4;

namespace {

struct ZarchiveParserStaticData final {
  ZarchiveParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  ZarchiveParserStaticData(const ZarchiveParserStaticData&) = delete;
  ZarchiveParserStaticData(ZarchiveParserStaticData&&) = delete;
  ZarchiveParserStaticData& operator=(const ZarchiveParserStaticData&) = delete;
  ZarchiveParserStaticData& operator=(ZarchiveParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag zarchiveparserParserOnceFlag;
ZarchiveParserStaticData *zarchiveparserParserStaticData = nullptr;

void zarchiveparserParserInitialize() {
  assert(zarchiveparserParserStaticData == nullptr);
  auto staticData = std::make_unique<ZarchiveParserStaticData>(
    std::vector<std::string>{
      "query", "booleanQuery", "scopedAdjacency", "adjacentWords", "quotedAdjacency", 
      "tildedAdjacency", "literally"
    },
    std::vector<std::string>{
      "", "", "", "'literally('", "'and'", "'or'", "'not'", "'('", "')'", 
      "'^'", "'$'"
    },
    std::vector<std::string>{
      "", "METADATA_FN_ARGS_STRING", "METADATA_FN_ARGS_WHOLE_NUMBER", "LITERALLY", 
      "AND", "OR", "NOT", "LPAREN", "RPAREN", "LANCHOR", "RANCHOR", "FIELD_SPECIFICATION", 
      "UNQUOTED_WORD_OR_PUNCT", "QUOTE", "TILDE", "WS", "QUOTED_WORD_OR_PUNCT", 
      "QUOTED_WS", "ENDQUOTE", "TILDED_WORD_OR_PUNCT", "TILDED_WS", "ENDTILDE", 
      "LITERAL", "WHOLE_NUMBER"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,23,158,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,1,
  	0,3,0,16,8,0,1,0,1,0,3,0,20,8,0,1,0,1,0,1,1,1,1,1,1,5,1,27,8,1,10,1,12,
  	1,30,9,1,1,1,1,1,5,1,34,8,1,10,1,12,1,37,9,1,1,1,1,1,1,1,5,1,42,8,1,10,
  	1,12,1,45,9,1,1,1,1,1,5,1,49,8,1,10,1,12,1,52,9,1,1,1,1,1,1,1,1,1,1,1,
  	1,1,1,1,3,1,61,8,1,1,1,1,1,3,1,65,8,1,1,1,1,1,3,1,69,8,1,1,1,1,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,1,84,8,1,10,1,12,1,87,9,1,1,
  	2,1,2,3,2,91,8,2,3,2,93,8,2,1,2,3,2,96,8,2,1,2,1,2,1,2,1,2,3,2,102,8,
  	2,1,2,3,2,105,8,2,1,3,4,3,108,8,3,11,3,12,3,109,1,4,1,4,3,4,114,8,4,1,
  	4,1,4,3,4,118,8,4,5,4,120,8,4,10,4,12,4,123,9,4,1,4,1,4,1,5,1,5,3,5,129,
  	8,5,1,5,1,5,3,5,133,8,5,5,5,135,8,5,10,5,12,5,138,9,5,1,5,1,5,1,6,1,6,
  	5,6,144,8,6,10,6,12,6,147,9,6,1,6,1,6,5,6,151,8,6,10,6,12,6,154,9,6,1,
  	6,1,6,1,6,0,1,2,7,0,2,4,6,8,10,12,0,0,181,0,15,1,0,0,0,2,68,1,0,0,0,4,
  	92,1,0,0,0,6,107,1,0,0,0,8,111,1,0,0,0,10,126,1,0,0,0,12,141,1,0,0,0,
  	14,16,5,15,0,0,15,14,1,0,0,0,15,16,1,0,0,0,16,17,1,0,0,0,17,19,3,2,1,
  	0,18,20,5,15,0,0,19,18,1,0,0,0,19,20,1,0,0,0,20,21,1,0,0,0,21,22,5,0,
  	0,1,22,1,1,0,0,0,23,24,6,1,-1,0,24,28,5,1,0,0,25,27,5,15,0,0,26,25,1,
  	0,0,0,27,30,1,0,0,0,28,26,1,0,0,0,28,29,1,0,0,0,29,31,1,0,0,0,30,28,1,
  	0,0,0,31,35,5,22,0,0,32,34,5,15,0,0,33,32,1,0,0,0,34,37,1,0,0,0,35,33,
  	1,0,0,0,35,36,1,0,0,0,36,38,1,0,0,0,37,35,1,0,0,0,38,69,5,8,0,0,39,43,
  	5,2,0,0,40,42,5,15,0,0,41,40,1,0,0,0,42,45,1,0,0,0,43,41,1,0,0,0,43,44,
  	1,0,0,0,44,46,1,0,0,0,45,43,1,0,0,0,46,50,5,23,0,0,47,49,5,15,0,0,48,
  	47,1,0,0,0,49,52,1,0,0,0,50,48,1,0,0,0,50,51,1,0,0,0,51,53,1,0,0,0,52,
  	50,1,0,0,0,53,69,5,8,0,0,54,69,3,4,2,0,55,56,5,6,0,0,56,57,5,15,0,0,57,
  	69,3,2,1,5,58,60,5,7,0,0,59,61,5,15,0,0,60,59,1,0,0,0,60,61,1,0,0,0,61,
  	62,1,0,0,0,62,64,3,2,1,0,63,65,5,15,0,0,64,63,1,0,0,0,64,65,1,0,0,0,65,
  	66,1,0,0,0,66,67,5,8,0,0,67,69,1,0,0,0,68,23,1,0,0,0,68,39,1,0,0,0,68,
  	54,1,0,0,0,68,55,1,0,0,0,68,58,1,0,0,0,69,85,1,0,0,0,70,71,10,4,0,0,71,
  	72,5,15,0,0,72,84,3,2,1,5,73,74,10,3,0,0,74,75,5,15,0,0,75,76,5,4,0,0,
  	76,77,5,15,0,0,77,84,3,2,1,4,78,79,10,2,0,0,79,80,5,15,0,0,80,81,5,5,
  	0,0,81,82,5,15,0,0,82,84,3,2,1,3,83,70,1,0,0,0,83,73,1,0,0,0,83,78,1,
  	0,0,0,84,87,1,0,0,0,85,83,1,0,0,0,85,86,1,0,0,0,86,3,1,0,0,0,87,85,1,
  	0,0,0,88,90,5,11,0,0,89,91,5,15,0,0,90,89,1,0,0,0,90,91,1,0,0,0,91,93,
  	1,0,0,0,92,88,1,0,0,0,92,93,1,0,0,0,93,95,1,0,0,0,94,96,5,9,0,0,95,94,
  	1,0,0,0,95,96,1,0,0,0,96,101,1,0,0,0,97,102,3,6,3,0,98,102,3,8,4,0,99,
  	102,3,10,5,0,100,102,3,12,6,0,101,97,1,0,0,0,101,98,1,0,0,0,101,99,1,
  	0,0,0,101,100,1,0,0,0,102,104,1,0,0,0,103,105,5,10,0,0,104,103,1,0,0,
  	0,104,105,1,0,0,0,105,5,1,0,0,0,106,108,5,12,0,0,107,106,1,0,0,0,108,
  	109,1,0,0,0,109,107,1,0,0,0,109,110,1,0,0,0,110,7,1,0,0,0,111,113,5,13,
  	0,0,112,114,5,17,0,0,113,112,1,0,0,0,113,114,1,0,0,0,114,121,1,0,0,0,
  	115,117,5,16,0,0,116,118,5,17,0,0,117,116,1,0,0,0,117,118,1,0,0,0,118,
  	120,1,0,0,0,119,115,1,0,0,0,120,123,1,0,0,0,121,119,1,0,0,0,121,122,1,
  	0,0,0,122,124,1,0,0,0,123,121,1,0,0,0,124,125,5,18,0,0,125,9,1,0,0,0,
  	126,128,5,14,0,0,127,129,5,20,0,0,128,127,1,0,0,0,128,129,1,0,0,0,129,
  	136,1,0,0,0,130,132,5,19,0,0,131,133,5,20,0,0,132,131,1,0,0,0,132,133,
  	1,0,0,0,133,135,1,0,0,0,134,130,1,0,0,0,135,138,1,0,0,0,136,134,1,0,0,
  	0,136,137,1,0,0,0,137,139,1,0,0,0,138,136,1,0,0,0,139,140,5,21,0,0,140,
  	11,1,0,0,0,141,145,5,3,0,0,142,144,5,15,0,0,143,142,1,0,0,0,144,147,1,
  	0,0,0,145,143,1,0,0,0,145,146,1,0,0,0,146,148,1,0,0,0,147,145,1,0,0,0,
  	148,152,5,22,0,0,149,151,5,15,0,0,150,149,1,0,0,0,151,154,1,0,0,0,152,
  	150,1,0,0,0,152,153,1,0,0,0,153,155,1,0,0,0,154,152,1,0,0,0,155,156,5,
  	8,0,0,156,13,1,0,0,0,25,15,19,28,35,43,50,60,64,68,83,85,90,92,95,101,
  	104,109,113,117,121,128,132,136,145,152
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  zarchiveparserParserStaticData = staticData.release();
}

}

ZarchiveParser::ZarchiveParser(TokenStream *input) : ZarchiveParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

ZarchiveParser::ZarchiveParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  ZarchiveParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *zarchiveparserParserStaticData->atn, zarchiveparserParserStaticData->decisionToDFA, zarchiveparserParserStaticData->sharedContextCache, options);
}

ZarchiveParser::~ZarchiveParser() {
  delete _interpreter;
}

const atn::ATN& ZarchiveParser::getATN() const {
  return *zarchiveparserParserStaticData->atn;
}

std::string ZarchiveParser::getGrammarFileName() const {
  return "ZarchiveParser.g4";
}

const std::vector<std::string>& ZarchiveParser::getRuleNames() const {
  return zarchiveparserParserStaticData->ruleNames;
}

const dfa::Vocabulary& ZarchiveParser::getVocabulary() const {
  return zarchiveparserParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView ZarchiveParser::getSerializedATN() const {
  return zarchiveparserParserStaticData->serializedATN;
}


//----------------- QueryContext ------------------------------------------------------------------

ZarchiveParser::QueryContext::QueryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::QueryContext::booleanQuery() {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(0);
}

tree::TerminalNode* ZarchiveParser::QueryContext::EOF() {
  return getToken(ZarchiveParser::EOF, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::QueryContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::QueryContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}


size_t ZarchiveParser::QueryContext::getRuleIndex() const {
  return ZarchiveParser::RuleQuery;
}

void ZarchiveParser::QueryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuery(this);
}

void ZarchiveParser::QueryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuery(this);
}

ZarchiveParser::QueryContext* ZarchiveParser::query() {
  QueryContext *_localctx = _tracker.createInstance<QueryContext>(_ctx, getState());
  enterRule(_localctx, 0, ZarchiveParser::RuleQuery);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(15);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::WS) {
      setState(14);
      match(ZarchiveParser::WS);
    }
    setState(17);
    booleanQuery(0);
    setState(19);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::WS) {
      setState(18);
      match(ZarchiveParser::WS);
    }
    setState(21);
    match(ZarchiveParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BooleanQueryContext ------------------------------------------------------------------

ZarchiveParser::BooleanQueryContext::BooleanQueryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t ZarchiveParser::BooleanQueryContext::getRuleIndex() const {
  return ZarchiveParser::RuleBooleanQuery;
}

void ZarchiveParser::BooleanQueryContext::copyFrom(BooleanQueryContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- MetadataFnArgsWholeNumberContext ------------------------------------------------------------------

tree::TerminalNode* ZarchiveParser::MetadataFnArgsWholeNumberContext::METADATA_FN_ARGS_WHOLE_NUMBER() {
  return getToken(ZarchiveParser::METADATA_FN_ARGS_WHOLE_NUMBER, 0);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsWholeNumberContext::WHOLE_NUMBER() {
  return getToken(ZarchiveParser::WHOLE_NUMBER, 0);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsWholeNumberContext::RPAREN() {
  return getToken(ZarchiveParser::RPAREN, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::MetadataFnArgsWholeNumberContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsWholeNumberContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}

ZarchiveParser::MetadataFnArgsWholeNumberContext::MetadataFnArgsWholeNumberContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::MetadataFnArgsWholeNumberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMetadataFnArgsWholeNumber(this);
}
void ZarchiveParser::MetadataFnArgsWholeNumberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMetadataFnArgsWholeNumber(this);
}
//----------------- NotContext ------------------------------------------------------------------

tree::TerminalNode* ZarchiveParser::NotContext::NOT() {
  return getToken(ZarchiveParser::NOT, 0);
}

tree::TerminalNode* ZarchiveParser::NotContext::WS() {
  return getToken(ZarchiveParser::WS, 0);
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::NotContext::booleanQuery() {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(0);
}

ZarchiveParser::NotContext::NotContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::NotContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNot(this);
}
void ZarchiveParser::NotContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNot(this);
}
//----------------- ParensContext ------------------------------------------------------------------

tree::TerminalNode* ZarchiveParser::ParensContext::LPAREN() {
  return getToken(ZarchiveParser::LPAREN, 0);
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::ParensContext::booleanQuery() {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(0);
}

tree::TerminalNode* ZarchiveParser::ParensContext::RPAREN() {
  return getToken(ZarchiveParser::RPAREN, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::ParensContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::ParensContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}

ZarchiveParser::ParensContext::ParensContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::ParensContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterParens(this);
}
void ZarchiveParser::ParensContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitParens(this);
}
//----------------- OrContext ------------------------------------------------------------------

std::vector<ZarchiveParser::BooleanQueryContext *> ZarchiveParser::OrContext::booleanQuery() {
  return getRuleContexts<ZarchiveParser::BooleanQueryContext>();
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::OrContext::booleanQuery(size_t i) {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(i);
}

std::vector<tree::TerminalNode *> ZarchiveParser::OrContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::OrContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}

tree::TerminalNode* ZarchiveParser::OrContext::OR() {
  return getToken(ZarchiveParser::OR, 0);
}

ZarchiveParser::OrContext::OrContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::OrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterOr(this);
}
void ZarchiveParser::OrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitOr(this);
}
//----------------- AdjContext ------------------------------------------------------------------

ZarchiveParser::ScopedAdjacencyContext* ZarchiveParser::AdjContext::scopedAdjacency() {
  return getRuleContext<ZarchiveParser::ScopedAdjacencyContext>(0);
}

ZarchiveParser::AdjContext::AdjContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::AdjContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAdj(this);
}
void ZarchiveParser::AdjContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAdj(this);
}
//----------------- AndContext ------------------------------------------------------------------

std::vector<ZarchiveParser::BooleanQueryContext *> ZarchiveParser::AndContext::booleanQuery() {
  return getRuleContexts<ZarchiveParser::BooleanQueryContext>();
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::AndContext::booleanQuery(size_t i) {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(i);
}

std::vector<tree::TerminalNode *> ZarchiveParser::AndContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::AndContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}

tree::TerminalNode* ZarchiveParser::AndContext::AND() {
  return getToken(ZarchiveParser::AND, 0);
}

ZarchiveParser::AndContext::AndContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::AndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAnd(this);
}
void ZarchiveParser::AndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAnd(this);
}
//----------------- MetadataFnArgsStringContext ------------------------------------------------------------------

tree::TerminalNode* ZarchiveParser::MetadataFnArgsStringContext::METADATA_FN_ARGS_STRING() {
  return getToken(ZarchiveParser::METADATA_FN_ARGS_STRING, 0);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsStringContext::LITERAL() {
  return getToken(ZarchiveParser::LITERAL, 0);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsStringContext::RPAREN() {
  return getToken(ZarchiveParser::RPAREN, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::MetadataFnArgsStringContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::MetadataFnArgsStringContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}

ZarchiveParser::MetadataFnArgsStringContext::MetadataFnArgsStringContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::MetadataFnArgsStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMetadataFnArgsString(this);
}
void ZarchiveParser::MetadataFnArgsStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMetadataFnArgsString(this);
}
//----------------- ImpliedAndContext ------------------------------------------------------------------

std::vector<ZarchiveParser::BooleanQueryContext *> ZarchiveParser::ImpliedAndContext::booleanQuery() {
  return getRuleContexts<ZarchiveParser::BooleanQueryContext>();
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::ImpliedAndContext::booleanQuery(size_t i) {
  return getRuleContext<ZarchiveParser::BooleanQueryContext>(i);
}

tree::TerminalNode* ZarchiveParser::ImpliedAndContext::WS() {
  return getToken(ZarchiveParser::WS, 0);
}

ZarchiveParser::ImpliedAndContext::ImpliedAndContext(BooleanQueryContext *ctx) { copyFrom(ctx); }

void ZarchiveParser::ImpliedAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterImpliedAnd(this);
}
void ZarchiveParser::ImpliedAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitImpliedAnd(this);
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::booleanQuery() {
   return booleanQuery(0);
}

ZarchiveParser::BooleanQueryContext* ZarchiveParser::booleanQuery(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  ZarchiveParser::BooleanQueryContext *_localctx = _tracker.createInstance<BooleanQueryContext>(_ctx, parentState);
  ZarchiveParser::BooleanQueryContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 2;
  enterRecursionRule(_localctx, 2, ZarchiveParser::RuleBooleanQuery, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(68);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case ZarchiveParser::METADATA_FN_ARGS_STRING: {
        _localctx = _tracker.createInstance<MetadataFnArgsStringContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(24);
        match(ZarchiveParser::METADATA_FN_ARGS_STRING);
        setState(28);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == ZarchiveParser::WS) {
          setState(25);
          match(ZarchiveParser::WS);
          setState(30);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(31);
        match(ZarchiveParser::LITERAL);
        setState(35);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == ZarchiveParser::WS) {
          setState(32);
          match(ZarchiveParser::WS);
          setState(37);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(38);
        match(ZarchiveParser::RPAREN);
        break;
      }

      case ZarchiveParser::METADATA_FN_ARGS_WHOLE_NUMBER: {
        _localctx = _tracker.createInstance<MetadataFnArgsWholeNumberContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(39);
        match(ZarchiveParser::METADATA_FN_ARGS_WHOLE_NUMBER);
        setState(43);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == ZarchiveParser::WS) {
          setState(40);
          match(ZarchiveParser::WS);
          setState(45);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(46);
        match(ZarchiveParser::WHOLE_NUMBER);
        setState(50);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == ZarchiveParser::WS) {
          setState(47);
          match(ZarchiveParser::WS);
          setState(52);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(53);
        match(ZarchiveParser::RPAREN);
        break;
      }

      case ZarchiveParser::LITERALLY:
      case ZarchiveParser::LANCHOR:
      case ZarchiveParser::FIELD_SPECIFICATION:
      case ZarchiveParser::UNQUOTED_WORD_OR_PUNCT:
      case ZarchiveParser::QUOTE:
      case ZarchiveParser::TILDE: {
        _localctx = _tracker.createInstance<AdjContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(54);
        scopedAdjacency();
        break;
      }

      case ZarchiveParser::NOT: {
        _localctx = _tracker.createInstance<NotContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(55);
        match(ZarchiveParser::NOT);
        setState(56);
        match(ZarchiveParser::WS);
        setState(57);
        booleanQuery(5);
        break;
      }

      case ZarchiveParser::LPAREN: {
        _localctx = _tracker.createInstance<ParensContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(58);
        match(ZarchiveParser::LPAREN);
        setState(60);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == ZarchiveParser::WS) {
          setState(59);
          match(ZarchiveParser::WS);
        }
        setState(62);
        booleanQuery(0);
        setState(64);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == ZarchiveParser::WS) {
          setState(63);
          match(ZarchiveParser::WS);
        }
        setState(66);
        match(ZarchiveParser::RPAREN);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(85);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(83);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ImpliedAndContext>(_tracker.createInstance<BooleanQueryContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleBooleanQuery);
          setState(70);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(71);
          match(ZarchiveParser::WS);
          setState(72);
          booleanQuery(5);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<AndContext>(_tracker.createInstance<BooleanQueryContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleBooleanQuery);
          setState(73);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(74);
          match(ZarchiveParser::WS);
          setState(75);
          match(ZarchiveParser::AND);
          setState(76);
          match(ZarchiveParser::WS);
          setState(77);
          booleanQuery(4);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<OrContext>(_tracker.createInstance<BooleanQueryContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleBooleanQuery);
          setState(78);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(79);
          match(ZarchiveParser::WS);
          setState(80);
          match(ZarchiveParser::OR);
          setState(81);
          match(ZarchiveParser::WS);
          setState(82);
          booleanQuery(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(87);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- ScopedAdjacencyContext ------------------------------------------------------------------

ZarchiveParser::ScopedAdjacencyContext::ScopedAdjacencyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

ZarchiveParser::AdjacentWordsContext* ZarchiveParser::ScopedAdjacencyContext::adjacentWords() {
  return getRuleContext<ZarchiveParser::AdjacentWordsContext>(0);
}

ZarchiveParser::QuotedAdjacencyContext* ZarchiveParser::ScopedAdjacencyContext::quotedAdjacency() {
  return getRuleContext<ZarchiveParser::QuotedAdjacencyContext>(0);
}

ZarchiveParser::TildedAdjacencyContext* ZarchiveParser::ScopedAdjacencyContext::tildedAdjacency() {
  return getRuleContext<ZarchiveParser::TildedAdjacencyContext>(0);
}

ZarchiveParser::LiterallyContext* ZarchiveParser::ScopedAdjacencyContext::literally() {
  return getRuleContext<ZarchiveParser::LiterallyContext>(0);
}

tree::TerminalNode* ZarchiveParser::ScopedAdjacencyContext::FIELD_SPECIFICATION() {
  return getToken(ZarchiveParser::FIELD_SPECIFICATION, 0);
}

tree::TerminalNode* ZarchiveParser::ScopedAdjacencyContext::LANCHOR() {
  return getToken(ZarchiveParser::LANCHOR, 0);
}

tree::TerminalNode* ZarchiveParser::ScopedAdjacencyContext::RANCHOR() {
  return getToken(ZarchiveParser::RANCHOR, 0);
}

tree::TerminalNode* ZarchiveParser::ScopedAdjacencyContext::WS() {
  return getToken(ZarchiveParser::WS, 0);
}


size_t ZarchiveParser::ScopedAdjacencyContext::getRuleIndex() const {
  return ZarchiveParser::RuleScopedAdjacency;
}

void ZarchiveParser::ScopedAdjacencyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterScopedAdjacency(this);
}

void ZarchiveParser::ScopedAdjacencyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitScopedAdjacency(this);
}

ZarchiveParser::ScopedAdjacencyContext* ZarchiveParser::scopedAdjacency() {
  ScopedAdjacencyContext *_localctx = _tracker.createInstance<ScopedAdjacencyContext>(_ctx, getState());
  enterRule(_localctx, 4, ZarchiveParser::RuleScopedAdjacency);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(92);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::FIELD_SPECIFICATION) {
      setState(88);
      match(ZarchiveParser::FIELD_SPECIFICATION);
      setState(90);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == ZarchiveParser::WS) {
        setState(89);
        match(ZarchiveParser::WS);
      }
    }
    setState(95);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::LANCHOR) {
      setState(94);
      match(ZarchiveParser::LANCHOR);
    }
    setState(101);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case ZarchiveParser::UNQUOTED_WORD_OR_PUNCT: {
        setState(97);
        adjacentWords();
        break;
      }

      case ZarchiveParser::QUOTE: {
        setState(98);
        quotedAdjacency();
        break;
      }

      case ZarchiveParser::TILDE: {
        setState(99);
        tildedAdjacency();
        break;
      }

      case ZarchiveParser::LITERALLY: {
        setState(100);
        literally();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(104);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      setState(103);
      match(ZarchiveParser::RANCHOR);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AdjacentWordsContext ------------------------------------------------------------------

ZarchiveParser::AdjacentWordsContext::AdjacentWordsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> ZarchiveParser::AdjacentWordsContext::UNQUOTED_WORD_OR_PUNCT() {
  return getTokens(ZarchiveParser::UNQUOTED_WORD_OR_PUNCT);
}

tree::TerminalNode* ZarchiveParser::AdjacentWordsContext::UNQUOTED_WORD_OR_PUNCT(size_t i) {
  return getToken(ZarchiveParser::UNQUOTED_WORD_OR_PUNCT, i);
}


size_t ZarchiveParser::AdjacentWordsContext::getRuleIndex() const {
  return ZarchiveParser::RuleAdjacentWords;
}

void ZarchiveParser::AdjacentWordsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAdjacentWords(this);
}

void ZarchiveParser::AdjacentWordsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAdjacentWords(this);
}

ZarchiveParser::AdjacentWordsContext* ZarchiveParser::adjacentWords() {
  AdjacentWordsContext *_localctx = _tracker.createInstance<AdjacentWordsContext>(_ctx, getState());
  enterRule(_localctx, 6, ZarchiveParser::RuleAdjacentWords);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(107); 
    _errHandler->sync(this);
    alt = 1;
    do {
      switch (alt) {
        case 1: {
              setState(106);
              match(ZarchiveParser::UNQUOTED_WORD_OR_PUNCT);
              break;
            }

      default:
        throw NoViableAltException(this);
      }
      setState(109); 
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 16, _ctx);
    } while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QuotedAdjacencyContext ------------------------------------------------------------------

ZarchiveParser::QuotedAdjacencyContext::QuotedAdjacencyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ZarchiveParser::QuotedAdjacencyContext::QUOTE() {
  return getToken(ZarchiveParser::QUOTE, 0);
}

tree::TerminalNode* ZarchiveParser::QuotedAdjacencyContext::ENDQUOTE() {
  return getToken(ZarchiveParser::ENDQUOTE, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::QuotedAdjacencyContext::QUOTED_WS() {
  return getTokens(ZarchiveParser::QUOTED_WS);
}

tree::TerminalNode* ZarchiveParser::QuotedAdjacencyContext::QUOTED_WS(size_t i) {
  return getToken(ZarchiveParser::QUOTED_WS, i);
}

std::vector<tree::TerminalNode *> ZarchiveParser::QuotedAdjacencyContext::QUOTED_WORD_OR_PUNCT() {
  return getTokens(ZarchiveParser::QUOTED_WORD_OR_PUNCT);
}

tree::TerminalNode* ZarchiveParser::QuotedAdjacencyContext::QUOTED_WORD_OR_PUNCT(size_t i) {
  return getToken(ZarchiveParser::QUOTED_WORD_OR_PUNCT, i);
}


size_t ZarchiveParser::QuotedAdjacencyContext::getRuleIndex() const {
  return ZarchiveParser::RuleQuotedAdjacency;
}

void ZarchiveParser::QuotedAdjacencyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQuotedAdjacency(this);
}

void ZarchiveParser::QuotedAdjacencyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQuotedAdjacency(this);
}

ZarchiveParser::QuotedAdjacencyContext* ZarchiveParser::quotedAdjacency() {
  QuotedAdjacencyContext *_localctx = _tracker.createInstance<QuotedAdjacencyContext>(_ctx, getState());
  enterRule(_localctx, 8, ZarchiveParser::RuleQuotedAdjacency);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(111);
    match(ZarchiveParser::QUOTE);
    setState(113);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::QUOTED_WS) {
      setState(112);
      match(ZarchiveParser::QUOTED_WS);
    }
    setState(121);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ZarchiveParser::QUOTED_WORD_OR_PUNCT) {
      setState(115);
      match(ZarchiveParser::QUOTED_WORD_OR_PUNCT);
      setState(117);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == ZarchiveParser::QUOTED_WS) {
        setState(116);
        match(ZarchiveParser::QUOTED_WS);
      }
      setState(123);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(124);
    match(ZarchiveParser::ENDQUOTE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TildedAdjacencyContext ------------------------------------------------------------------

ZarchiveParser::TildedAdjacencyContext::TildedAdjacencyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ZarchiveParser::TildedAdjacencyContext::TILDE() {
  return getToken(ZarchiveParser::TILDE, 0);
}

tree::TerminalNode* ZarchiveParser::TildedAdjacencyContext::ENDTILDE() {
  return getToken(ZarchiveParser::ENDTILDE, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::TildedAdjacencyContext::TILDED_WS() {
  return getTokens(ZarchiveParser::TILDED_WS);
}

tree::TerminalNode* ZarchiveParser::TildedAdjacencyContext::TILDED_WS(size_t i) {
  return getToken(ZarchiveParser::TILDED_WS, i);
}

std::vector<tree::TerminalNode *> ZarchiveParser::TildedAdjacencyContext::TILDED_WORD_OR_PUNCT() {
  return getTokens(ZarchiveParser::TILDED_WORD_OR_PUNCT);
}

tree::TerminalNode* ZarchiveParser::TildedAdjacencyContext::TILDED_WORD_OR_PUNCT(size_t i) {
  return getToken(ZarchiveParser::TILDED_WORD_OR_PUNCT, i);
}


size_t ZarchiveParser::TildedAdjacencyContext::getRuleIndex() const {
  return ZarchiveParser::RuleTildedAdjacency;
}

void ZarchiveParser::TildedAdjacencyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTildedAdjacency(this);
}

void ZarchiveParser::TildedAdjacencyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTildedAdjacency(this);
}

ZarchiveParser::TildedAdjacencyContext* ZarchiveParser::tildedAdjacency() {
  TildedAdjacencyContext *_localctx = _tracker.createInstance<TildedAdjacencyContext>(_ctx, getState());
  enterRule(_localctx, 10, ZarchiveParser::RuleTildedAdjacency);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(126);
    match(ZarchiveParser::TILDE);
    setState(128);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == ZarchiveParser::TILDED_WS) {
      setState(127);
      match(ZarchiveParser::TILDED_WS);
    }
    setState(136);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ZarchiveParser::TILDED_WORD_OR_PUNCT) {
      setState(130);
      match(ZarchiveParser::TILDED_WORD_OR_PUNCT);
      setState(132);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == ZarchiveParser::TILDED_WS) {
        setState(131);
        match(ZarchiveParser::TILDED_WS);
      }
      setState(138);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(139);
    match(ZarchiveParser::ENDTILDE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LiterallyContext ------------------------------------------------------------------

ZarchiveParser::LiterallyContext::LiterallyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* ZarchiveParser::LiterallyContext::LITERALLY() {
  return getToken(ZarchiveParser::LITERALLY, 0);
}

tree::TerminalNode* ZarchiveParser::LiterallyContext::LITERAL() {
  return getToken(ZarchiveParser::LITERAL, 0);
}

tree::TerminalNode* ZarchiveParser::LiterallyContext::RPAREN() {
  return getToken(ZarchiveParser::RPAREN, 0);
}

std::vector<tree::TerminalNode *> ZarchiveParser::LiterallyContext::WS() {
  return getTokens(ZarchiveParser::WS);
}

tree::TerminalNode* ZarchiveParser::LiterallyContext::WS(size_t i) {
  return getToken(ZarchiveParser::WS, i);
}


size_t ZarchiveParser::LiterallyContext::getRuleIndex() const {
  return ZarchiveParser::RuleLiterally;
}

void ZarchiveParser::LiterallyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLiterally(this);
}

void ZarchiveParser::LiterallyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<ZarchiveParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLiterally(this);
}

ZarchiveParser::LiterallyContext* ZarchiveParser::literally() {
  LiterallyContext *_localctx = _tracker.createInstance<LiterallyContext>(_ctx, getState());
  enterRule(_localctx, 12, ZarchiveParser::RuleLiterally);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(141);
    match(ZarchiveParser::LITERALLY);
    setState(145);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ZarchiveParser::WS) {
      setState(142);
      match(ZarchiveParser::WS);
      setState(147);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(148);
    match(ZarchiveParser::LITERAL);
    setState(152);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == ZarchiveParser::WS) {
      setState(149);
      match(ZarchiveParser::WS);
      setState(154);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(155);
    match(ZarchiveParser::RPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool ZarchiveParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 1: return booleanQuerySempred(antlrcpp::downCast<BooleanQueryContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool ZarchiveParser::booleanQuerySempred(BooleanQueryContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 4);
    case 1: return precpred(_ctx, 3);
    case 2: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

void ZarchiveParser::initialize() {
  ::antlr4::internal::call_once(zarchiveparserParserOnceFlag, zarchiveparserParserInitialize);
}

