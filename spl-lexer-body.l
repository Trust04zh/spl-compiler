%option yylineno

%{
    #define EXIT_OK 0
    #define EXIT_FAIL 1
%}

letter [a-zA-Z]
letter_underline {letter}|_
digit [0-9]
digit_nonzero [1-9]
hexdigit [0-9a-fA-F]
hex_prefix 0(x|X)
float_suffix (f|F)

whitespace [ \t\n]

/* A int constant starting with "0" is considered to be oct form, which is not supported by spl. */
/* Here I consider the sign ("+" or "-") to be an unary operator, instead of a part of the constant */
int             ({digit_nonzero}{digit}*)|({hex_prefix}({hexdigit}+))
float           (({digit}*\.{digit}+)|({digit}+\.{digit}*)){float_suffix}*
/* for example "'ab'", will not be considered to be valid char token */
/* [\x20-\x7f] indicates printable characters */
char            '(([\x20-\x7f])|(\\x{hexdigit}{2}))'
identifier      {letter_underline}({letter_underline}|{digit})*
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
    #else
    #endif
}
{float} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: float (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{char} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: char (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{type} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: type (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{struct} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: struct (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{if} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: if (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{else} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: else (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{while} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: while (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{return} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: return (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{identifier} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: identifier (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{dot} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: dot (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{semi} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: semi (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{comma} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: comma (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{assign} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: assign (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{lt} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lt (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{le} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: le (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{gt} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: gt (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{ge} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: ge (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{ne} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: ne (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{eq} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: eq (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{plus} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: plus (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{minus} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: minus (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{mul} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: mul (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{div} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: div (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{and} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: and (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{or} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: or (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{not} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: not (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{lp} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lp (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{rp} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rp (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{lb} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lb (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{rb} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rb (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{lc} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: lc (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}
{rc} {
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        printf("line %d: rc (%d) : %s\n", yylineno, yyleng, yytext);
    #else
    #endif
}

{whitespace} {  }
.* { printf("unrecognized sequence: %s\n", yytext); exit(1); }

%%


