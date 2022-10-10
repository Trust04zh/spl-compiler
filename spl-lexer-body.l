%option yylineno

%{
    #if !defined(SPL_LEXER_STANDALONE)
        #include "spl-parser-body.h"
    #endif
    #define EXIT_OK 0
    #define EXIT_FAIL 1
%}

letter [a-zA-Z]
letter_underline ({letter}|_)
digit [0-9]
digit_nonzero [1-9]
hexdigit [0-9a-fA-F]
hexdigit_nonzero [1-9a-fA-F]
hex_prefix (0(x|X))
float_suffix (f|F)

whitespace [ \t\r\n]

/* A int constant starting with "0" is considered to be oct form, which is not supported by spl. */
/* Here I consider the sign ("+" or "-") to be an unary operator, instead of a part of the constant */
int             ((0|{digit_nonzero}{digit}*))|({hex_prefix}(0|{hexdigit_nonzero}{hexdigit}*))
float           (({digit}*\.{digit}+)|({digit}+\.{digit}*)){float_suffix}*
/* for example "'ab'", will not be considered to be valid char token */
/* [\x20-\x7f] indicates printable characters */
char            '(([\x20-\x7f])|(\\x{hexdigit}{2}))'
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
/* spl does not support "++" and "--", they would be considered to be two consecutive "+" or "-" operators */
/* FIXME: conflict with syntax definition? */
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

{int} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: int (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return INT;
    #endif
}
{float} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: float (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return FLOAT;
    #endif
}
{char} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: char (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return CHAR;
    #endif
}
{type} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: type (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return TYPE;
    #endif
}
{struct} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: struct (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return STRUCT;
    #endif
}
{if} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: if (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return IF;
    #endif
}
{else} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: else (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return ELSE;
    #endif
}
{while} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: while (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return WHILE;
    #endif
}
{return} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: return (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return RETURN;
    #endif
}
{id} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: id (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return ID;
    #endif
}
{dot} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: dot (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return DOT;
    #endif
}
{semi} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: semi (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return SEMI;
    #endif
}
{comma} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: comma (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return COMMA;
    #endif
}
{assign} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: assign (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return ASSIGN;
    #endif
}
{lt} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lt (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return LT;
    #endif
}
{le} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: le (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return LE;
    #endif
}
{gt} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: gt (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return GT;
    #endif
}
{ge} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: ge (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return GE;
    #endif
}
{ne} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: ne (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return NE;
    #endif
}
{eq} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: eq (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return EQ;
    #endif
}
{plus} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: plus (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return PLUS;
    #endif
}
{minus} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: minus (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return MINUS;
    #endif
}
{mul} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: mul (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return MUL;
    #endif
}
{div} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: div (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return DIV;
    #endif
}
{and} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: and (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return AND;
    #endif
}
{or} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: or (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return OR;
    #endif
}
{not} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: not (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return NOT;
    #endif
}
{lp} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lp (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return LP;
    #endif
}
{rp} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rp (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return RP;
    #endif
}
{lb} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lb (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return LB;
    #endif
}
{rb} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rb (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return RB;
    #endif
}
{lc} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lc (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return LC;
    #endif
}
{rc} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rc (%d) : %s\n", yylineno, yyleng, yytext);
    #endif
    #if !defined(SPL_LEXER_STANDALONE)
        return RC;
    #endif
}


{whitespace} {  }
. { printf("unrecognized character (hex): %x\n", *yytext); exit(1); }

%%


