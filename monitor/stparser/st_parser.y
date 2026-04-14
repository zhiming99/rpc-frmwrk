%{
#include <stdio.h>
#include <stdlib.h>
%}

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_END_IF CASE OF END_CASE TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_TYPED_LITERAL
%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY
%token TOK_PLUS TOK_MINUS TOK_MULTIPLY TOK_DIVIDE TOK_MOD

%token TOK_FUNCTION_BLOCK TOK_FUNCTION TOK_END_FUNCTION_BLOCK TOK_END_FUNCTION

%%

program:
    TOK_PROGRAM TOK_ID declarations statements TOK_END_VAR
    ;

declarations:
    TOK_VAR declaration_list TOK_END_VAR
    ;

declaration_list:
    declaration_list declaration
    | declaration
    ;

declaration:
    TOK_ID TOK_COLON type TOK_SEMICOLON
    ;

type:
    TOK_INT
    | TOK_REAL
    | TOK_LREAL
    | TOK_BOOL
    | TOK_STRING
    | TOK_WSTRING
    | TOK_ARRAY
    ;

statements:
    statements statement
    | statement
    ;

statement:
    assignment
    | if_statement
    | for_statement
    | while_statement
    | repeat_statement
    | function_block
    | function
    ;
expression:
    arith_expression
    ;

assignment:
    TOK_ID TOK_ASSIGN expression TOK_SEMICOLON
    ;

if_statement:
    TOK_IF expression TOK_THEN statements TOK_ELSE statements TOK_END_IF
    ;

for_statement:
    TOK_FOR TOK_ID TOK_ASSIGN expression TOK_TO expression TOK_DO statements TOK_END_FOR
    ;

while_statement:
    TOK_WHILE expression TOK_DO statements TOK_END_WHILE
    ;

repeat_statement:
    TOK_REPEAT statements TOK_UNTIL expression TOK_END_REPEAT
    ;

arith_expression:
    arith_expression TOK_PLUS term
    | arith_expression TOK_MINUS term
    | term { $$ = $1; }
    ;

term:
    term TOK_MULTIPLY factor
    | term TOK_DIVIDE factor
    | term TOK_MOD factor
    | factor { $$ = $1; }
    ;

factor:
    TOK_INT
    | TOK_REAL
    | TOK_LREAL
    | TOK_ID
    | TOK_STRING
    | TOK_TIME
    | TOK_TRUE
    | TOK_FALSE
    | TOK_TYPED_LITERAL {
        std::string s($1);
        size_t hashPos = s.find('#');
        std::string type = s.substr(0, hashPos);
        std::string valStr = s.substr(hashPos + 1);

        // Remove underscores from value string
        valStr.erase(std::remove(valStr.begin(), valStr.end(), '_'), valStr.end());

        if (type == "REAL" || type == "LREAL") {
            printf("f32.const %s\n", valStr.c_str());
            $$.type = TOK_REAL;
        } else {
            // Handle hex if 16# is present
            int32_t finalVal;
            if (valStr.find("16#") == 0) {
                finalVal = std::stol(valStr.substr(3), nullptr, 16);
            } else {
                finalVal = std::stol(valStr);
            }
            printf("i32.const %d\n", finalVal);
            $$.type = TOK_INT;
        }
        free($1);
    }
    | '(' expression ')'
    ;

function_block:
    TOK_FUNCTION_BLOCK TOK_ID declarations statements TOK_END_FUNCTION_BLOCK
    ;

function:
    TOK_FUNCTION TOK_ID declarations statements TOK_END_FUNCTION
    ;

%%

int main() {
    yyparse();
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
