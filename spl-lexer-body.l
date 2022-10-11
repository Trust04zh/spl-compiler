%{
    #include <stdlib.h>

    #if !defined(SPL_LEXER_STANDALONE)
        #include "spl-parser-body.h"
    #endif
    #define EXIT_OK 0
    #define EXIT_FAIL 1
    
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        #define SPL_LEXER_DEBUG_PRINT(...) \
            printf( __VA_ARGS__ );
    #else
        #define SPL_LEXER_DEBUG_PRINT(...)
    #endif

    #define SPL_ERROR_PRINT(...) \
        printf( __VA_ARGS__ );

    int yycolumn = 1;
    #define YY_USER_ACTION \
            yylloc.first_line = yylloc.last_line = yylineno; \
            yylloc.first_column = yycolumn; \
            yylloc.last_column = yycolumn + yyleng - 1; \
            yycolumn += yyleng;
    
    // at flex 2.6.4 input() confusingly returns 0 instead of -1 (EOF) when meeting an EOF
    // reference: https://github.com/westes/flex/issues/448
    #define YYINPUT_EOF 0
    // #define YYINPUT_EOF -1
%}

%option yylineno

letter [a-zA-Z]
letter_underline ({letter}|_)
digit [0-9]
digit_nonzero [1-9]
hexdigit [0-9a-fA-F]
hexdigit_nonzero [1-9a-fA-F]
hex_prefix (0(x|X))

eol \n
whitespace [ \t\r\n]

/* A int constant starting with "0" is considered to be oct form, which is not supported by spl. */
err_int_dec     0{digit}+
err_int_hex     {hex_prefix}0{hexdigit}+
int             ((0|{digit_nonzero}{digit}*))|({hex_prefix}(0|{hexdigit_nonzero}{hexdigit}*))
float           {digit}+\.{digit}+
/* for example "'ab'", will not be considered to be valid char token */
/* [\x20-\x7f] indicates printable characters */
char            '(([\x20-\x7f])|(\\x{hexdigit}{2}))'
err_id_digit    {digit}({letter_underline}|{digit})*
id              {letter_underline}({letter_underline}|{digit})*
type            "int"|"float"|"char"
struct          "struct"
if              "if"
else            "else"
while           "while"
return          "return"
dot             "."
semi            ";"
comma           ","
assign          "="
lt              "<"
le              "<="
gt              ">"
ge              ">="
ne              "!="
eq              "=="
plus            "+"
minus           "-"
mul             "*"
div             "/"
and             "&&"
or              "||"
not             "!"
lp              "("
rp              ")"
lb              "["
rb              "]"
lc              "{"
rc              "}"

%%

{err_int_dec} {
    SPL_ERROR_PRINT("line %d\t col %d\t: decimal int constant cannot start with 0 (except for 0) : %s\n", yylineno, yycolumn - yyleng, yytext); 
    exit(EXIT_FAIL);
}
{err_int_hex} {
    SPL_ERROR_PRINT("line %d\t col %d\t: hexadecimal int constant cannot start with 0 in digit part (except for 0x0) : %s\n", yylineno, yycolumn - yyleng, yytext); 
    exit(EXIT_FAIL);
}
{int} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: int        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = (union YYSTYPE)atoi(yytext);
        return INT;
    #endif
}
{float} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: float      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = (union YYSTYPE)(float)atof(yytext);
        return FLOAT;
    #endif
}
{char} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: char       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = (union YYSTYPE)(*yytext);
        return CHAR;
    #endif
}
{type} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: type       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return TYPE;
    #endif
}
{struct} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: struct     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return STRUCT;
    #endif
}
{if} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: if         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return IF;
    #endif
}
{else} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: else       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return ELSE;
    #endif
}
{while} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: while      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return WHILE;
    #endif
}
{return} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: return     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return RETURN;
    #endif
}
{err_id_digit} {
    SPL_ERROR_PRINT("line %d\t col %d\t: identifier cannot start with digit : %s\n", yylineno, yycolumn - yyleng, yytext); 
    exit(EXIT_FAIL);
}
{id} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: id         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        char *copy = malloc(strlen(yytext) + 1); 
        strcpy(copy, copy);
        yylval = (YYSTYPE)copy;
        return ID;
    #endif
}
{dot} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: dot        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return DOT;
    #endif
}
{semi} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: semi       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return SEMI;
    #endif
}
{comma} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: comma      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return COMMA;
    #endif
}
{assign} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: assign     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return ASSIGN;
    #endif
}
{lt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return LT;
    #endif
}
{le} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: le         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return LE;
    #endif
}
{gt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: gt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return GT;
    #endif
}
{ge} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ge         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return GE;
    #endif
}
{ne} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ne         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return NE;
    #endif
}
{eq} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: eq         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return EQ;
    #endif
}
{plus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: plus       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return PLUS;
    #endif
}
{minus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: minus      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return MINUS;
    #endif
}
{mul} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: mul        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return MUL;
    #endif
}
{div} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: div        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return DIV;
    #endif
}
{and} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: and        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return AND;
    #endif
}
{or} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: or         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return OR;
    #endif
}
{not} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: not        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return NOT;
    #endif
}
{lp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return LP;
    #endif
}
{rp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return RP;
    #endif
}
{lb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return LB;
    #endif
}
{rb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return RB;
    #endif
}
{lc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return LC;
    #endif
}
{rc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        return RC;
    #endif
}


{eol} { yycolumn = 1; }
{whitespace} {  }

. { printf("line %d\t col %d\t: unrecognized character (hex): %x\n", yylineno, yycolumn - yyleng, *yytext); exit(EXIT_FAIL); }

%%


