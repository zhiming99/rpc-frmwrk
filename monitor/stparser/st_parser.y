%{
#include <stdio.h>
#include <stdlib.h>
%}

%token TOK_PROGRAM TOK_VAR TOK_END_VAR TOK_IF TOK_THEN TOK_ELSE TOK_END_IF CASE OF END_CASE TOK_FOR TOK_TO TOK_DO TOK_END_FOR TOK_WHILE TOK_END_WHILE TOK_REPEAT TOK_UNTIL TOK_END_REPEAT
%token TOK_TON TOK_TON_VALUE TOK_STRING TOK_WSTRING TOK_INT TOK_REAL TOK_LREAL TOK_BOOL TOK_TRUE TOK_FALSE TOK_TIME TOK_TYPED_LITERAL
%token TOK_ID TOK_NUMBER TOK_ASSIGN TOK_SEMICOLON TOK_COLON TOK_COMMA TOK_ARRAY
%token TOK_PLUS TOK_MINUS TOK_MULTIPLY TOK_DIVIDE TOK_MOD TOK_NOT TOK_AND TOK_OR TOK_XOR TOK_DATE TOK_TIME_OF_DAY TOK_DATE_TIME

%token TOK_EQUAL TOK_POWER TOK_LBRACKET TOK_RBRACKET TOK_LBRACE TOK_RBRACE TOK_LPAREN TOK_RPAREN TOK_LE TOK_GT TOK_NEQU TOK_NLE TOK_NGT
               
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
    assignment_statement TOK_SEMICOLON
    | if_statement TOK_SEMICOLON
    | for_statement TOK_SEMICOLON
    | while_statement TOK_SEMICOLON
    | repeat_statement TOK_SEMICOLON
    | function_block TOK_SEMICOLON
    | function TOK_SEMICOLON
    | function_call_statement TOK_SEMICOLON
    ;

/* Rule for Assignments: Only allows memory locations on the LHS */
assignment_statement:
      l_value TOK_ASSIGN full_expression {
          // Wasm logic to store the result of full_expression into l_value
          emit_assignment($1, $3);
      }
    ;

/* Rule for Standalone Calls: Used for functions/methods that return void or whose return is ignored */
function_call_statement:
      TOK_ID TOK_LPAREN arg_list TOK_RPAREN {
          // Wasm logic to call the function
          // If it's a DCS Utility, we push the g_pUtils handle first
          printf("%s",$1);
      }
    ;

/* L-Value rule: Strictly limited to writable memory locations */
l_value:
      TOK_ID { /* simple variable */ }

    | l_value TOK_LBRACKET full_expression TOK_RBRACKET { /* array element */ }
    | l_value '.' TOK_ID { /* struct field or bit access */ }
    ; 

full_expression:
    and_expression
    | full_expression TOK_OR and_expression
    | full_expression TOK_XOR and_expression
    ;

and_expression:
    comparison_expression
    | and_expression TOK_AND comparison_expression
    ;

/* 3. COMPARISON (=, <>, <, >, <=, >=) */
comparison_expression:
      arithmetic_expr
    | arithmetic_expr TOK_EQUAL  arithmetic_expr { printf($1.type == TYPE_INT ? "i32.eq\n" : "f32.eq\n"); $$.type = TYPE_BOOL; }
    | arithmetic_expr TOK_NEQU arithmetic_expr { printf($1.type == TYPE_INT ? "i32.ne\n" : "f32.ne\n"); $$.type = TYPE_BOOL; }

    | arithmetic_expr TOK_LE   arithmetic_expr 
    | arithmetic_expr TOK_GT   arithmetic_expr
    | arithmetic_expr TOK_NLE  arithmetic_expr
    | arithmetic_expr TOK_NGT  arithmetic_expr
    ;

/* 4. ADDITIVE (+, -) */
arithmetic_expr:
      term
    | arithmetic_expr TOK_PLUS term { printf($1.type == TYPE_INT ? "i32.add\n" : "f32.add\n"); }
    | arithmetic_expr TOK_MINUS term { printf($1.type == TYPE_INT ? "i32.sub\n" : "f32.sub\n"); }
    ;

/* 5. MULTIPLICATIVE (*, /, MOD) */
term:
      unary_expr

    | term TOK_MULTIPLY   unary_expr { printf($1.type == TYPE_INT ? "i32.mul\n" : "f32.mul\n"); }
    | term TOK_DIVIDE   unary_expr { printf($1.type == TYPE_INT ? "i32.div_s\n" : "f32.div\n"); }
    | term TOK_MOD unary_expr { printf("i32.rem_s\n"); }
    ;

/* 6. UNARY (NOT, -) */
unary_expr:
      power_expr

    | TOK_MINUS   power_expr { printf($2.type == TYPE_INT ? "i32.const 0\ni32.sub\n" : "f32.neg\n"); }
    | TOK_NOT power_expr { printf("i32.eqz\n"); }
    ;

/* 7. POWER (**) */
power_expr:
      factor
    | factor TOK_POWER factor { printf("call $math_pow\n"); }
    ;

/* 8. PRIMARY (Highest Precedence) */
factor:
      TOK_INT

    | TOK_REAL
    | TOK_STRING
    | TOK_WSTRING

    | TOK_TIME    /* T#5s */
    | TOK_DATE      /* D#2024-01-01 */
    | TOK_DATE_TIME      /* DT#2024-01-01 */
    | TOK_TIME_OF_DAY    /* TOD#2024-01-01 */
    | TOK_TRUE
    | TOK_FALSE
    /* INT#10, WORD#16#FF */
    | TOK_TYPED_LITERAL   {
        std::string s($1);
        size_t hashPos = s.find('#');
        std::string type = s.substr(0, hashPos);
        std::string valStr = s.substr(hashPos + 1);

        // Remove underscores from value string
        valStr.erase(std::remove(valStr.begin(), valStr.end(), '_'), valStr.end());

        if (type == "REAL" || type == "LREAL") {
            printf("f32.const %s\n", valStr.c_str());
            $$.type = TYPE_REAL;
        } else {
            // Handle hex if 16# is present
            int32_t finalVal;
            if (valStr.find("16#") == 0) {
                finalVal = std::stol(valStr.substr(3), nullptr, 16);
            } else {
                finalVal = std::stol(valStr);
            }
            printf("i32.const %d\n", finalVal);
            $$.type = TYPE_INT;
        }
        free($1);
    }

    | TOK_ID TOK_LPAREN arg_list TOK_RPAREN {
          /* 
             1. Identify if it's a standard function or DCS utility.
             2. For utilities, push the g_pUtils handle first.
             3. Generate the Wasm 'call' instruction.
          */
          printf("call $%s\n", $1);
    }
    | TOK_ID
    | '(' full_expression ')'
    ;

assignment:
    TOK_ID TOK_ASSIGN full_expression TOK_SEMICOLON
    ;

if_statement:
    TOK_IF full_expression TOK_THEN statements TOK_ELSE statements TOK_END_IF
    ;

for_statement:
    TOK_FOR TOK_ID TOK_ASSIGN full_expression TOK_TO full_expression TOK_DO statements TOK_END_FOR
    ;

while_statement:
    TOK_WHILE full_expression TOK_DO statements TOK_END_WHILE
    ;

repeat_statement:
    TOK_REPEAT statements TOK_UNTIL full_expression TOK_END_REPEAT
    ;

arg_list:
      param_assignment
    | arg_list ',' param_assignment
    ;

param_assignment:
      full_expression             /* Positional: MyFunc(10) */
    | TOK_ID TOK_ASSIGN full_expression  /* Formal Input: IN := True */
    | TOK_ID TOK_NLE l_value           /* Formal Output: Q => MyLamp */
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
