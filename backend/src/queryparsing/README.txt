This is how I build for C++

antlr4 -Dlanguage=Cpp -package z2kplus::backend::parsing::generated -o generated ZarchiveLexer.g4 ZarchiveParser.g4

Be extremely cautious when renaming these files. ANTLR really likes paired
parsers and lexers to be named BlahParser and BlahLexer, at least when it comes
to running grun.

This is how I test with Java
antlr4 -Dlanguage=Java -o generated ZarchiveLexer.g4 ZarchiveParser.g4
cd generated
javac *.java
grun Zarchive query -gui
[enter text here]
^D
