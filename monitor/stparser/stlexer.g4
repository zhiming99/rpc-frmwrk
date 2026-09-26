/*
 * stlexer.g4 - ANTLR4 Lexer for Structured Text Language (IEC 61131-3)
 * Converted from stlexer.l
 */

//Lexer grammar
lexer grammar stlexer;
options {
    caseInsensitive = true;
}
channels { PRAGMA_CHANNEL }

// Keywords - case insensitive
// Type keywords
TOK_INT_TYPE  : 'INT';
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
TOK_REAL_TYPE : 'REAL';
TOK_LREAL_TYPE : 'LREAL';

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
TOK_LTIME_TYPE      : 'LTIME';
TOK_DATE_TYPE       : 'DATE';
TOK_LDATE_TYPE      : 'LDATE';

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
TOK_AND             : 'AND';
TOK_AND_OP          : '&';
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
TOK_EXTERNAL        : 'EXTERNAL';

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
fragment HEX_DIGIT  : [0-9A-F];
fragment LETTER     : [A-Z];

// Integer literals with underscores
fragment UNSIGNED_INT : DIGIT+ ('_'? DIGIT+)*;
fragment SIGNED_INT  : ('+' | '-')? UNSIGNED_INT;
fragment BIT : '0' | '1';

// string literal
fragment COMMON_CHAR_VALUE
    : '$$'              // Matches a literal escaped dollar sign
    | '$' [LNPRT]  // Matches $ followed by a valid IEC string escape code (case-insensitive)
    | ~['$\r\n]         // Matches any single character except single quotes, dollar signs, or newlines
    ;

TOK_CHAR_LITERAL
    : ('S#' | 'STRING#')? ( CHAR_STR | WCHAR_STR )
    ;

TOK_WCHAR_LITERAL
    : ('W#' | 'WSTRING#')? WCHAR_STR    
    | ('W#' | 'WSTRING#') WCHAR_ALTSTR    
    ;

TOK_UCHAR_LITERAL
    : ('U#' | 'USTRING#') UCHAR_STR    
    ;

fragment CHAR_STR
    : '\'' CHAR_VALUE* '\''
    ;

fragment WCHAR_STR
    : '"' WCHAR_VALUE* '"'
    ;

fragment WCHAR_ALTSTR
    : '\'' WCHAR_ALTVALUE* '\''
    ;

fragment UCHAR_STR
    : '\'' UCHAR_VALUE* '\''
    ;

fragment CHAR_VALUE
    : COMMON_CHAR_VALUE | '$\'' | '$' HEX_DIGIT HEX_DIGIT
    ;

fragment WCHAR_VALUE
    : COMMON_CHAR_VALUE | '\'' | '$"' | '$' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;

fragment WCHAR_ALTVALUE
    : COMMON_CHAR_VALUE | '$\'' | '"' | '$' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;

fragment UCHAR_VALUE
    : COMMON_CHAR_VALUE 
    | '$\'' 
    | '"' 
    | '$' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT 
    | '${' HEX_DIGIT+ '}'
    ;

// Character literals: CHAR#'x' or 'x'
TOK_CHAR : (('C' | 'CHAR') '#')? '\'' CHAR_VALUE '\'';

// Unicode char: UCHAR#'x' or UC#'x'
TOK_UCHAR : (('UC' | 'UCHAR') '#')? '\'' UCHAR_VALUE '\'';

// Wide char: WCHAR#'x' or WC#'x'
TOK_WCHAR : ('WC' | 'WCHAR') '#'? '"' WCHAR_VALUE '"';

// Typed numeric literals: REAL#10, LREAL#-1.5, INT#10
// Real literals with type
TOK_REAL : ('REAL' | 'LREAL') '#' SIGNED_INT ( '.' UNSIGNED_INT ('E' SIGNED_INT)?)
           | UNSIGNED_INT '.' UNSIGNED_INT ('E' SIGNED_INT)?  // Real literal
           ;

TOK_INT:   '16#' HEX_DIGIT+  // Based literal 16#FF
           | SIGNED_INT        // Simple integer
           | UNSIGNED_INT 
           | '2#' BIT ('_'? BIT)*   // binary literal
           ;


// Peripheral address: %IW0:P, %PIW0, %Q*.1
TOK_ABS_ADDR_PERIPHERAL : '%' [P]? [IQM] [XBWDL]? DIGIT+ ('.' DIGIT+ | '*')? (':' [P])?;

// Multi-part access: %QW0.1
TOK_MULTPART_ACCESS : '%' [XBWD]? DIGIT+;

// Partial address: %I* %Q*
TOK_PART_ADDR : '%' [IQM] '*';

// RPCF address: @IOSensor.Name
TOK_RPCF_ADDR : '@' [IOS] [BWDQSOF] [A-Z_][A-Z0-9_]* ('.' [A-Z_][A-Z0-9_]*)* (':' [A-Z_][A-Z0-9_]*)?;

// Identifier
TOK_ID : [A-Z_] [A-Z0-9_]*;

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

PRAGMA : '{' ~[}]* '}' -> channel(PRAGMA_CHANNEL) ;

// Whitespace - ignored
TOK_WHITESPACE : [ \t\r\n]+ -> channel(HIDDEN);

// Error handling for unknown characters
TOK_UNKNOWN_CHAR : . -> channel(HIDDEN);

// time_literal
fragment TIME_TYPE_NAME: 'TIME' | 'LTIME';
fragment DATE_TYPE_NAME: 'DATE' | 'LDATE';
fragment TOD_TYPE_NAME : 'TIME_OF_DAY' | 'LTIME_OF_DAY' | 'TOD' | 'LTOD';
fragment DT_TYPE_NAME :  'DATE_AND_TIME' | 'LDATE_AND_TIME' | 'DT' | 'LDT';
TOK_DURATION : ( TIME_TYPE_NAME | 'T' | 'LT' ) '#' ( '+' | '-' )? INTERVAL; 
fragment FIX_POINT  : UNSIGNED_INT ( '.' UNSIGNED_INT )?;
fragment INTERVAL : DAYS;
fragment DAYS: FIX_POINT 'd' | ( UNSIGNED_INT 'd' '_'? )? HOURS;
fragment HOURS: FIX_POINT 'h' | ( UNSIGNED_INT 'h' '_'? )? MINUTES;
fragment MINUTES: FIX_POINT 'm' | ( UNSIGNED_INT 'm' '_'? )? SECONDS;
fragment SECONDS: FIX_POINT 's' | ( UNSIGNED_INT 's' '_'? )? MILLISECONDES;
fragment MILLISECONDES: FIX_POINT 'ms' | ( UNSIGNED_INT 'ms' '_'? )? MICROSECONDES;
fragment MICROSECONDES: FIX_POINT 'us' | ( UNSIGNED_INT 'us' '_'? )? NANOSECONDES;
fragment NANOSECONDES: FIX_POINT 'us';
TOK_TIME_OF_DAY: TOD_TYPE_NAME '#' DAYTIME;
fragment DAYTIME : DAY_HOUR ':' DAY_MINUTE ':' DAY_SECOND;
fragment DAY_HOUR : UNSIGNED_INT;
fragment DAY_MINUTE: UNSIGNED_INT;
fragment DAY_SECOND: FIX_POINT;
fragment DATE_LITERAL: YEAR '-' MONTH '-' DAY;
fragment YEAR: UNSIGNED_INT;
fragment MONTH: UNSIGNED_INT;
fragment DAY: UNSIGNED_INT;
TOK_DATE : ( DATE_TYPE_NAME | 'D' | 'LD' ) '#' DATE_LITERAL;
TOK_DATE_TIME: DT_TYPE_NAME '#' DATE_LITERAL '-' DAYTIME;

