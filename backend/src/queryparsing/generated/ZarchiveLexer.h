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


// Generated from ZarchiveLexer.g4 by ANTLR 4.11.1

#pragma once


#include "antlr4-runtime.h"


namespace z2kplus::backend::queryparsing::generated {


class  ZarchiveLexer : public antlr4::Lexer {
public:
  enum {
    METADATA_FN_ARGS_STRING = 1, METADATA_FN_ARGS_WHOLE_NUMBER = 2, LITERALLY = 3, 
    AND = 4, OR = 5, NOT = 6, LPAREN = 7, RPAREN = 8, LANCHOR = 9, RANCHOR = 10, 
    FIELD_SPECIFICATION = 11, UNQUOTED_WORD_OR_PUNCT = 12, QUOTE = 13, TILDE = 14, 
    WS = 15, QUOTED_WORD_OR_PUNCT = 16, QUOTED_WS = 17, ENDQUOTE = 18, TILDED_WORD_OR_PUNCT = 19, 
    TILDED_WS = 20, ENDTILDE = 21, LITERAL = 22, WHOLE_NUMBER = 23
  };

  enum {
    QUOTE_MODE = 1, TILDE_MODE = 2, LITERAL_MODE = 3, WHOLE_NUMBER_MODE = 4
  };

  explicit ZarchiveLexer(antlr4::CharStream *input);

  ~ZarchiveLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace z2kplus::backend::queryparsing::generated
