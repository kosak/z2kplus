parser grammar ZarchiveParser;
options {tokenVocab = ZarchiveLexer; }

query: WS? booleanQuery WS? EOF ;

booleanQuery:
  // e.g. hasreaction("bug")
  METADATA_FN_ARGS_STRING WS* LITERAL WS* RPAREN # metadataFnArgsString |
  // e.g. zgramid(12345)
  METADATA_FN_ARGS_WHOLE_NUMBER WS* WHOLE_NUMBER WS* RPAREN # metadataFnArgsWholeNumber |
  // e.g. sender: kosak
  scopedAdjacency #adj |
  // e.g. not sender: kosak
  NOT WS booleanQuery #not |
  // Whitespace between booleanQueries gets you implied AND
  // e.g. sender:kosak instance:love  // AND(sender:kosak, instance: love)
  booleanQuery WS booleanQuery #impliedAnd |
  // e.g. kosak and not cinnabon
  booleanQuery WS AND WS booleanQuery #and |
  // e.g. kosak or cinnabon
  booleanQuery WS OR WS booleanQuery #or |
  // e.g. (sender:kosak and instance:C++) and not likedby("agroce")
  LPAREN WS? booleanQuery WS? RPAREN #parens ;

// An adjacency group is an optional field specification followed by a bunch of words which are
// interpreted as being adjacent, either because they are enclosed in quotes or tildes, or because
// they are separated by punctuation rather than whitespace. These things are adjacent
//
// Don't   // A single word thanks to apostrophe handling
// kosak++  // ADJ(kosak,+,+)
// hurly-burly  // ADJ(hurly,-,burly)
// corey-louis.kosak  // ADJ(corey,-,louis,.,kosak)
// star!trek   // ADJ(star,!,trek)
// star\*trek  // ADJ(star,*,trek)  (* escaped otherwise it's a glob character)
// "corey louis kosak"  // ADJ(corey, louis, kosak)
// ~corey louis kosak~  // NEAR(corey, louis, kosak)
// sender: kosak  // field specification is "sender:"
// sender: ^kosak$  // field specification is "sender:", front anchor, back anchor
// instance: ^"This pain no name"$  // front and back anchor, ADJ(this,pain,no,name)
//
// Counterexamples:
// sender: ^corey kosak$  // no, this will parse as IMPLIED-AND(sender:^corey,kosak$)

// optional fieldspec like sender: or instance,body:
// optional left anchor ^
// one of singleWord, quotedAdjacency, tildedAdjency, literally
// optional right anchor "
//
// Loaded-up example: instance,body:^Isn't$
scopedAdjacency: (FIELD_SPECIFICATION WS?)? LANCHOR?
  (adjacentWords | quotedAdjacency | tildedAdjacency | literally)
  RANCHOR? ;

// An adjacent sequence of 1 or more UNQUOTED_WORD_OR_PUNCT, with no whitespace separating them.
// Examples:
//   hello  // 1 word
//   +  // 1 word
//   hurly-burly   // NEAR(1, hurly, -, burly)
//   Don't   // 1 word (due to special apostrophe handling)
//   C++  // NEAR(1, C, +, +)
adjacentWords: UNQUOTED_WORD_OR_PUNCT+ ;

// A quoted sequence of words. The words inside the quotes are interpreted as adjacent, and now,
// thanks to the quotes, whitespace is allowed.
// Examples:
//   "hello"  // just the word hello  
//   "hello there"  // NEAR(1, hello, there)
//   "isn't this nice"   // NEAR(1, isn't, this, nice)
//   "isn't this, nice"  // NEAR(1, isn't, this, [COMMA], nice)
//   "I like \"pie\""  // NEAR(1, I, like, ", pie, ")  // if you need quotes
//   "I like ~pie~"  // NEAR(1, I, like, ~, pie, ~)  // tildes have no special meaning inside quotes
quotedAdjacency: QUOTE QUOTED_WS? (QUOTED_WORD_OR_PUNCT QUOTED_WS?)* ENDQUOTE ;

// Similar to quotes except nearness criterion is looser.
// Examples:
//   ~hello~  // just the word hello  
//   ~hello there~  // NEAR(3, hello, there)
//   ~isn't this nice~   // NEAR(3, isn't, this, nice)
//   ~isn't this, nice~  // NEAR(3, isn't, this, [COMMA], nice)
//   ~I like \~pie\~~  // NEAR(3, I, like, ~, pie, ~)  // if you need tildes
//   ~I like "pie"~  // NEAR(3, I, like, ", pie, ")  // quotes have no special meaning inside tildes
tildedAdjacency: TILDE TILDED_WS? (TILDED_WORD_OR_PUNCT TILDED_WS?)* ENDTILDE ;

// A special operator, mostly of use internally, which breaks up a string into adjacent words the
// same way the zgram indexer would. It is used as a convenience for the frontend, who would like
// to automatically construct queries without worrying about special cases. The caller's only job
// is to escape " and \.
//
// One difference (perhaps the only difference) between this and quoted strings is the looseness
// criterion.
// instance:^"cinnabon"$  // would match ċïṅṅäḅöṅ
// instance:^literally("cinnabon")$  // would not
// Of course, a hack that would accomplish the same thing would be for the caller to force exactness
// by escaping every lower case character. But I would rather just give the caller an explicit means
// to get the "literally" concept without burdening them with special-case hacks (which could change
// over time).
literally: LITERALLY WS* LITERAL WS* RPAREN ;
