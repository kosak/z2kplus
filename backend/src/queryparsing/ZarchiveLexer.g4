lexer grammar ZarchiveLexer;

// The primary point of the lexer is to group text into entities. The entities are:
// 1. A continuous run of punctuation, for some definition of punctuation
// 2. A continuous run of letters for some definition of letter
// 3. Special function invocations
// 4. whitespace
// 5. There is also a "literal string" mode, used for arguments to the special functions
//    like hasreaction("kosak"). No splitting is done here.

// We consider "punctuation" to be all ASCII 7-bit punctuation characters, with a couple of
// exceptions. We consider letters to be all Unicode characters > 32 and != 127, excluding the above
// punctuation.
//
// Special keywords are as defined below.
//
// The quotation marks and tilde marks don't change the word-breaking rules. Instead they have two
// effects:
// 1. They disable the special treatment of the function invocations and boolean operators.
// 2. They change the higher-level intepretation, e.g.
//    corey kosak  // AND(corey, kosak)
//    "corey kosak" // ADJ(corey, kosak)
//    ~corey kosak~ // NEAR(corey, kosak)
//
// The special quoting characters " ~ \
fragment PUNCT_QUOTING: ["~\\] ;
// The special glob characters
fragment PUNCT_GLOBBING: [*?] ;
// The query grouping characters
fragment PUNCT_PARENS: [()] ;
// The anchor characters
fragment PUNCT_ANCHORS: [$^] ;
// BTW, apostrophe is a letter if it's inside a word, and punct if it's on either end
fragment PUNCT_APOSTROPHE: '\'';
// escapes for the above
fragment PUNCT_ESCAPES: '\\"' | '\\~' | '\\\\' | '\\*' | '\\?' | '\\\'' ;
// All 7-bit punctuation except for PUNCT_QUOTING, PUNCT_GLOBBING, PUNCT_PARENS, PUNCT_ANCHORS,
// and PUNCT_APOSTROPHE.
fragment PUNCT_NORMAL: [!#%&+,-./:;<=>@[_`{|}] | ']' ;
// The "F" stands for fragment. White Space Fragment.
fragment WSF: [ \t\n\r\f]+ ;

// This is equivalent to
// ~[WHITESPACE | PUNCT_QUOTING | PUNCT_PARENS | PUNCT_ANCHORS | PUNCT_APOSTROPHE | PUNCT_NORMAL]
// (which we can't say directly in antlr4)
// Notice that we consider the globbing characters to be letters for the purpose of this.
fragment LETTER: ~[ \t\n\r\f"~\\()$^'!#%&+,-./:;<=>@[_`{|}\]] ;
fragment WORD: LETTER+(PUNCT_APOSTROPHE LETTER+)* ;

// A single punctuation character or an escaped punctuation character.
// We pick up a single apostrophe here (if it's not picked up by WORD, which will be longer and thus
// be a preferable match).
fragment PUNCT: (PUNCT_NORMAL | PUNCT_APOSTROPHE | PUNCT_ESCAPES) ;
// Inside quotation marks and tildes, we give parens and anchors their normal meaning again.
fragment QPUNCT: (PUNCT | PUNCT_PARENS | PUNCT_ANCHORS);

// Support for literal mode
fragment LITERAL_NORMAL: ~["\\] ;
fragment LITERAL_ESCAPE: '\\"' | '\\\\' ;

// With those definitions out of the way, we can write our lexer.

// Metadata function taking 1 string.  A metadata keyword followed by an open paren, which changes
// lexing mode. e.g.
// hasreaction("#c++")
METADATA_FN_ARGS_STRING: ('hasreaction') '(' -> pushMode(LITERAL_MODE);

// zgramid(5000)
METADATA_FN_ARGS_WHOLE_NUMBER: ('zgramid') '(' -> pushMode(WHOLE_NUMBER_MODE);

// literally("blah"). Rationale: getting the backend to help you split words properly. All you need
// to do is escape your quotation marks. e.g. if your instance is
//   help.C++.isn't."fail"
// you can search for any instance that starts that way via
//   instance:^literally("help.C++.isn't.\"fail\"")
//
// without having to worry about how the backend would break up those words.
// TODO: explain this more here, and even decide if it even makes sense.
LITERALLY: 'literally(' -> pushMode(LITERAL_MODE);

// Boolean operators.
AND: 'and' ;
OR: 'or' ;
NOT: 'not' ;
LPAREN: '(' ;
RPAREN: ')' ;

// Anchors.
LANCHOR: '^' ;
RANCHOR: '$' ;

// Field specifications only kick in when they parse exactly. For example
// sender  // not a reserved word
// sender:  // reserved
// sender,instance  // not
// sender,instance: // is
fragment FIELD_SPECIFIER: 'sender' | 'signature' | 'instance' | 'body' ;
FIELD_SPECIFICATION: (FIELD_SPECIFIER WSF* ',' WSF*)* FIELD_SPECIFIER WSF* ':' ;

UNQUOTED_WORD_OR_PUNCT: WORD | PUNCT;

// The quotation mark introduces quoted text. The purpose of quotes is threefold:
// To specify the empty string: ""
// To specify text that would otherwise look like a field specifier: "sender:"
// To specify words that you want to be adjacent: "I like pie"
//
// The tilde acts like a quotation mark except it means "near" rather than "strictly adjacent":
// "kosak cafe" means adjacent(kosak,cafe) whereas
// ~kosak cafe~ means near(kosak,cafe)
QUOTE: '"' -> pushMode(QUOTE_MODE) ;
TILDE: '~' -> pushMode(TILDE_MODE) ;

// Whitespace. Terminology: "WS" is the token. "WSF" is the fragment.
WS: WSF ;

mode QUOTE_MODE;
// tildes are allowed inside quotation marks
QUOTED_WORD_OR_PUNCT: WORD | QPUNCT | '~';
QUOTED_WS: WSF ;
ENDQUOTE: '"' -> popMode ;

mode TILDE_MODE;
// quotation marks are allowed inside tildes
TILDED_WORD_OR_PUNCT: WORD | QPUNCT | '"';
TILDED_WS: WSF ;
ENDTILDE: '~' -> popMode ;

mode LITERAL_MODE;
LITERAL: '"' (LITERAL_NORMAL | LITERAL_ESCAPE)* '"' -> popMode ;

mode WHOLE_NUMBER_MODE;
WHOLE_NUMBER: [0-9]+ -> popMode;

// Consider having a skip rule for control characters.
// CONTROL: [whatever]+ -> skip
