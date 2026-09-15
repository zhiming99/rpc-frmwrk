/*
 * stlexer.g4 - ANTLR4 Lexer for Structured Text Language (IEC 61131-3)
 * Converted from stlexer.l
 */

//Lexer grammar
lexer grammar stlexer;

// Keywords - case insensitive
// Type keywords
TOK_INT       : 'INT';
TOK_UINT      : 'UINT';
TOK_DINT      : 'DINT';
TOK_UDINT     : 'UDINT';
TOK_LINT      : 'LINT';
TOK_ULINT     : 'ULINT';
TOK_SINT      : 'SINT';
TOK_USINT     : 'USINT';
TOK_BOOL      : 'BOOL';
TOK_BYTE      : 'BYTE';
TOK_WORD      : 'WORD';
TOK_DWORD     : 'DWORD';
TOK_LDWORD    : 'LDWORD';
TOK_REAL      : 'REAL';
TOK_LREAL     : 'LREAL';

// Program organization
TOK_PROGRAM         : 'PROGRAM';
TOK_FUNCTION_BLOCK  : 'FUNCTION_BLOCK';
TOK_FUNCTION        : 'FUNCTION';
TOK_END_FUNCTION_BLOCK : 'END_FUNCTION_BLOCK';
TOK_END_FUNCTION    : 'END_FUNCTION';
TOK_END_PROGRAM     : 'END_PROGRAM';
TOK_INTERFACE       : 'INTERFACE';
TOK_END_INTERFACE   : 'END_INTERFACE';
TOK_CLASS           : 'CLASS';
TOK_END_CLASS       : 'END_CLASS';
TOK_PROPERTY        : 'PROPERTY';
TOK_END_PROPERTY    : 'END_PROPERTY';

// Variable declaration keywords
TOK_VAR             : 'VAR';
TOK_VAR_GLOBAL      : 'VAR_GLOBAL';
TOK_VAR_INPUT       : 'VAR_INPUT';
TOK_VAR_OUTPUT      : 'VAR_OUTPUT';
TOK_VAR_IN_OUT      : 'VAR_IN_OUT';
TOK_VAR_TEMP        : 'VAR_TEMP';
TOK_VAR_EXTERNAL    : 'VAR_EXTERNAL';
TOK_VAR_CONFIG      : 'VAR_CONFIG';
TOK_VAR_ACCESS      : 'VAR_ACCESS';
TOK_VAR_STAT        : 'VAR_STAT';
TOK_CONSTANT        : 'CONSTANT';
TOK_END_VAR         : 'END_VAR';

// Control flow keywords
TOK_IF              : 'IF';
TOK_THEN            : 'THEN';
TOK_ELSE            : 'ELSE';
TOK_ELSIF           : 'ELSIF';
TOK_END_IF          : 'END_IF';
TOK_CASE            : 'CASE';
TOK_OF              : 'OF';
TOK_END_CASE        : 'END_CASE';
TOK_FOR             : 'FOR';
TOK_TO              : 'TO';
TOK_BY              : 'BY';
TOK_DO              : 'DO';
TOK_END_FOR         : 'END_FOR';
TOK_WHILE           : 'WHILE';
TOK_END_WHILE       : 'END_WHILE';
TOK_REPEAT          : 'REPEAT';
TOK_UNTIL           : 'UNTIL';
TOK_END_REPEAT      : 'END_REPEAT';

// Type definitions
TOK_STRUCT          : 'STRUCT';
TOK_END_STRUCT      : 'END_STRUCT';
TOK_ARRAY           : 'ARRAY';
TOK_TYPE            : 'TYPE';
TOK_END_TYPE        : 'END_TYPE';
TOK_STRING_TYPE     : 'STRING';
TOK_WSTRING_TYPE    : 'WSTRING';
TOK_USTRING_TYPE    : 'USTRING';
TOK_CHAR_TYPE       : 'CHAR';
TOK_WCHAR_TYPE      : 'WCHAR';
TOK_UCHAR_TYPE      : 'UCHAR';
TOK_TIME_TYPE       : 'TIME';

// OOP keywords
TOK_EXTENDS         : 'EXTENDS';
TOK_IMPLEMENTS      : 'IMPLEMENTS';
TOK_ABSTRACT        : 'ABSTRACT';
TOK_FINAL           : 'FINAL';
TOK_OVERRIDE        : 'OVERRIDE';
TOK_SUPER           : 'SUPER';
TOK_THIS            : 'THIS';
TOK_PROPERTY_GET    : 'PROPERTY_GET';
TOK_PROPERTY_SET    : 'PROPERTY_SET';

// Access specifiers
TOK_PRIVATE         : 'PRIVATE';
TOK_PUBLIC          : 'PUBLIC';
TOK_PROTECTED       : 'PROTECTED';
TOK_INTERNAL        : 'INTERNAL';

// Reference keywords
TOK_REFERENCE       : 'REFERENCE';
TOK_REF_TO          : 'REF_TO';
TOK_REF             : 'REF';
TOK_NULL            : 'NULL';

// Configuration keywords
TOK_CONFIGURATION   : 'CONFIGURATION';
TOK_END_CONFIGURATION : 'END_CONFIGURATION';
TOK_TASK            : 'TASK';
TOK_RESOURCE        : 'RESOURCE';
TOK_END_RESOURCE    : 'END_RESOURCE';
TOK_SINGLE          : 'SINGLE';
TOK_INTERVAL        : 'INTERVAL';
TOK_PRIORITY        : 'PRIORITY';
TOK_ON              : 'ON';
TOK_WITH            : 'WITH';
TOK_RETAIN          : 'RETAIN';
TOK_NON_RETAIN      : 'NON_RETAIN';
TOK_PERSISTENT      : 'PERSISTENT';

// Other keywords
TOK_AT              : 'AT';
TOK_MOD             : 'MOD';
TOK_AND             : ('AND'|'&');
TOK_OR              : 'OR';
TOK_XOR             : 'XOR';
TOK_NOT             : 'NOT';
TOK_READ_ONLY       : 'READ_ONLY';
TOK_READ_WRITE      : 'READ_WRITE';
TOK_ATTRIBUTE       : 'ATTRIBUTE';
TOK_INFO            : 'INFO';
TOK_REGION          : 'REGION';
TOK_END_REGION      : 'END_REGION';
TOK_NAMESPACE       : 'NAMESPACE';
TOK_END_NAMESPACE   : 'END_NAMESPACE';
TOK_USING           : 'USING';
TOK_INCLUDE         : 'INCLUDE';
TOK_OVERLAP         : 'OVERLAP';
TOK_TRUE            : 'TRUE';
TOK_FALSE           : 'FALSE';
TOK_RETURN          : 'RETURN';
TOK_CONTINUE        : 'CONTINUE';
TOK_EXIT            : 'EXIT';
TOK_METHOD          : 'METHOD';
TOK_END_METHOD      : 'END_METHOD';
TOK_LWORD           : 'LWORD';

// std func name
TOK_ABS             : 'ABS';
TOK_SQRT            : 'SQRT';
TOK_LN              : 'LN';
TOK_EXP             : 'EXP';
TOK_LOG             : 'LOG';
TOK_SIN             : 'SIN';
TOK_COS             : 'COS';

// Fragment rules (helper definitions)
fragment DIGIT      : [0-9];
fragment HEX_DIGIT  : [0-9a-fA-F];
fragment LETTER     : [a-zA-Z];

// Integer literals with underscores
fragment UNSIGNED_INT : DIGIT+ ('_'? DIGIT+)*;
fragment SIGNED_INT  : ('+' | '-')? UNSIGNED_INT;

// Time literals
// T#... or TIME#...
TOK_TIME : ([tT] | 'TIME') '#'
          '-'? UNSIGNED_INT ('d' | 'h' | 'm' | 's' | 'ms')+
          ([0-9]+ ('d' | 'h' | 'm' | 's' | 'ms'))*;

// LT#... or LTIME#...
TOK_LTIME : ([lL] [tT] | 'LTIME') '#'
           '-'? UNSIGNED_INT ('d' | 'h' | 'm' | 's' | 'ms' | 'us' | 'ns')+
           ([0-9]+ ('d' | 'h' | 'm' | 's' | 'ms' | 'us' | 'ns'))*;

// Date literal D#2024-12-25
TOK_DATE : [dD] '#' DIGIT{4} '-' DIGIT{2} '-' DIGIT{2};

// Time of day TOD#14:30:05.123
TOK_TIME_OF_DAY : ([tT] [oO] [dD] | 'TIME_OF_DAY') '#'
                  DIGIT{1,2} ':' DIGIT{2} ':' DIGIT{2} ('.' DIGIT+)?;

// Date and time DT#2024-12-25-14:30:05
TOK_DATE_TIME : ([dD] [tT] | 'DATE_AND_TIME') '#'
               DIGIT{4} '-' DIGIT{2} '-' DIGIT{2} '-'
               DIGIT{1,2} ':' DIGIT{2} ':' DIGIT{2} ('.' DIGIT+)?;

// String literals
// Standard string: STRING#'...' or S#'...' or '...'
TOK_STRING : ([sS] | 'STRING') '#'? '\'' ('\\$' | ~[$'\r\n])* '\'';

// Wide string: WSTRING#"..." or W#"..."
TOK_WSTRING : ([wW] | 'WSTRING') '#'? '"' ('\\$' | ~[$"\r\n])* '"';

// Unicode string: USTRING#'...' or U#'...'
TOK_USTRING : ([uU] | 'USTRING') '#'? '\'' ('\\$' | ~[$'\r\n])* '\'';

// Character literals: CHAR#'x' or 'x'
TOK_CHAR : ([cC] [hH] [aA] [rR] | 'CHAR') '#'? '\'' ('\\$' | ~[$'\r\n]) '\'';

// Unicode char: UCHAR#'x' or UC#'x'
TOK_UCHAR : ([uU] [cC] [hH] [aA] [rR] | 'UCHAR') '#'? '\'' ('\\$' | ~[$'\r\n]) '\'';

// Wide char: WCHAR#'x' or WC#'x'
TOK_WCHAR : ([wW] [cC] [hH] [aA] [rR] | 'WCHAR') '#'? '"' ('\\$' | ~[$"\r\n]) '"';

// Typed numeric literals: REAL#10, LREAL#-1.5, INT#10
// Real literals with type
TOK_NUMBER : (REAL | LREAL) '#' SIGNED_INT ('.' UNSIGNED_INT)? ('E' SIGNED_INT)?
           | UNSIGNED_INT '#' HEX_DIGIT+  // Based literal 16#FF
           | SIGNED_INT                    // Simple integer
           | UNSIGNED_INT '.' UNSIGNED_INT ('E' SIGNED_INT)?  // Real literal
           | UNSIGNED_INT;  // Simple integer fallback
fragment REAL  : [rR] [eE] [aA] [lL];
fragment LREAL : [lL] [rR] [eE] [aA] [lL];

// Peripheral address: %IW0:P, %PIW0, %Q*.1
TOK_ABS_ADDR_PERIPHERAL : '%' [pP]? [iIqQmM] [xXbBwWdDlL]? DIGIT+ ('.' DIGIT+ | '*')? (':' [pP])?;

// Multi-part access: %QW0.1
TOK_MULTPART_ACCESS : '%' [xXbBwWdD]? DIGIT+;

// Partial address: %I* %Q*
TOK_PART_ADDR : '%' [iIqQmM] '*';

// RPCF address: @IOSensor.Name
TOK_RPCF_ADDR : '@' [iIoOsS] [bBwWdDqQsSoOfF] [a-zA-Z_][a-zA-Z0-9_]* ('.' [a-zA-Z_][a-zA-Z0-9_]*)* (':' [a-zA-Z_][a-zA-Z0-9_]*)?;

// Identifier
TOK_ID : [a-zA-Z_] [a-zA-Z0-9_]*;

// Operators
TOK_NEQU           : '<>';
TOK_NLE            : '>=';
TOK_OUTPUT_ASSIGN  : '=>';
TOK_LE             : '<=';
TOK_ASSIGN         : ':=';
TOK_EQUAL          : '=';
TOK_SEMICOLON      : ';';
TOK_COLON          : ':';
TOK_COMMA          : ',';
TOK_ADD            : '+';
TOK_MINUS          : '-';
TOK_POWER          : '**';
TOK_MUL            : '*';
TOK_DIV            : '/';
TOK_LBRACKET       : '[';
TOK_RBRACKET       : ']';
TOK_LBRACE         : '{';
TOK_RBRACE         : '}';
TOK_LPAREN         : '(';
TOK_RPAREN         : ')';
TOK_LT             : '<';
TOK_GT             : '>';
TOK_RANGE          : '..';
TOK_DOT            : '.';
TOK_PUNC           : '#';
TOK_CARET          : '^';
TOK_UNDERSCORE     : '_';
TOK_QASSIGN        : '?=' ;

// Comments - match entire comment including delimiters
TOK_BLOCK_COMMENT_ST : '(*' .*? '*)' -> channel(HIDDEN);
TOK_BLOCK_COMMENT_C  : '/*' .*? '*/' -> channel(HIDDEN);
TOK_LINE_COMMENT     : '//' ~[\r\n]* -> channel(HIDDEN);

// Whitespace - ignored
WS : [ \t\r\n]+ -> channel(HIDDEN);

// Error handling for unknown characters
UNKNOWN_CHAR : . -> channel(HIDDEN);
