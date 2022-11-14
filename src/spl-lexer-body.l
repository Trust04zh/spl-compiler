%{
    #include <cstdio>
    #include <cstdlib>
    #include <cstring>
    #include <sstream>
    #include <fstream>

    extern "C" int yylex();

    extern bool hasError;

    #define EXIT_OK 0
    #define EXIT_FAIL 1
    
    #if defined(SPL_LEXER_STANDALONE) || defined(SPL_LEXER_VERBOSE)
        #define SPL_LEXER_DEBUG_PRINT(...) \
            printf( __VA_ARGS__ );
    #else
        #define SPL_LEXER_DEBUG_PRINT(...)
    #endif

    #define SPL_LEXER_ERROR_PRINT(...) \
        printf( __VA_ARGS__ );

    #if defined(SPL_LEXER_STANDALONE)
        struct {
            int first_line;
            int first_column;
            int last_line;
            int last_column;
        } yylloc;
    #endif
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
%option noyywrap


letter [a-zA-Z]
letter_underline ({letter}|_)
digit [0-9]
digit_nonzero [1-9]
hexdigit [0-9a-fA-F]
hexdigit_nonzero [1-9a-fA-F]
hex_prefix (0(x|X))

eol \n
whitespace [ \t\r]


include         "#include"{whitespace}*"<"

/* A int constant starting with "0" is considered to be oct form, which is not supported by spl. */
err_int_dec     0{digit}+
err_int_hex     {hex_prefix}0{hexdigit}+
int             ((0|{digit_nonzero}{digit}*))|({hex_prefix}(0|{hexdigit_nonzero}{hexdigit}*))
float           {digit}+\.{digit}+
/* [\x20-\x7e] indicates printable characters */
/* ref: https://en.cppreference.com/w/cpp/string/byte/isprint */
char            '(([\x20-\x7e])|(\\x{hexdigit}{2}))'
err_id_digit    {digit}({letter_underline}|{digit})*
id              {letter_underline}({letter_underline}|{digit})*
type            "int"|"float"|"char"
struct          "struct"
if              "if"
else            "else"
while           "while"
for             "for"
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


%x SINGLE_LINE_COMMENT
%x MULTI_LINE_COMMENT

%%

<*>{eol} {
    yycolumn = 1;
    if (YYSTATE == SINGLE_LINE_COMMENT) {
        BEGIN(INITIAL);
    }
}

<SINGLE_LINE_COMMENT><<EOF>> {
    BEGIN(INITIAL);
}
<SINGLE_LINE_COMMENT>. {  }

<INITIAL>"*/" {
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: unexpected \"*/\"\n", yylineno);
    exit(EXIT_FAIL);
}
<MULTI_LINE_COMMENT>"*/" {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: \"*/\"\n", yylineno, yycolumn - yyleng);
    BEGIN(INITIAL);
}
<MULTI_LINE_COMMENT>. {  }
<MULTI_LINE_COMMENT><<EOF>>  {
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: unterminated comment\n", yylineno);
    exit(EXIT_FAIL);
}

{whitespace} {  }

"//" { 
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: \"//\"\n", yylineno, yycolumn - yyleng);
    BEGIN(SINGLE_LINE_COMMENT); 
}
"/*" {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: \"/*\"\n", yylineno, yycolumn - yyleng);
    BEGIN(MULTI_LINE_COMMENT);
}

{include} {
    std::stringstream f;
    char ch;
    bool flag;
    while (ch = yyinput(), ch >= 0x21 && ch <= 0x7e) {
        if (ch == '>') {
            flag = true;
            break;
        }
        f << ch;
        // putchar(ch);
    }
    if (!flag) {
        printf("Error type A: no matching >\n");
    } else {
        std::ifstream in(f.str());
        if (!in.is_open()) {
            printf("Error type A: failed to open: %s\n", f.str().c_str());
        } else {
            std::stringstream buf;
            buf << in.rdbuf();
            std::string tmp(buf.str());
            for (auto it = tmp.crbegin(); it != tmp.crend(); it++) {
                if (*it == '\n') {
                    yylineno += 1;
                }
                unput(*it);
            }
        }
    }
}

{err_int_dec} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: decimal int constant cannot start with 0 (except for 0) : %s\n", yylineno, yycolumn - yyleng, yytext);
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: decimal int constant cannot start with 0 (except for 0) : %s\n", yylineno, yytext);
    hasError = true;
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TERMINAL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ILLEGAL_INT_CONST", tmp_splattr, tmp_splloc);
        return INT;
    #endif
}
{err_int_hex} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: hexadecimal int constant cannot start with 0 in digit part (except for 0x0) : %s\n", yylineno, yycolumn - yyleng, yytext); 
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: hexadecimal int constant cannot start with 0 in digit part (except for 0x0) : %s\n", yylineno, yytext); 
    hasError = true;
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TERMINAL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ILLEGAL_INT_CONST", tmp_splattr, tmp_splloc);
        return INT;
    #endif
}
{int} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: int        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_INT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            tmp_splval.val_int.raw = strdup(yytext);
        #endif
        if (strlen(yytext) >= 2 && (yytext[1] == 'x' || yytext[1] == 'X')) { 
            tmp_splval.val_int.value = strtol(yytext, nullptr, 16);
        } else {
            tmp_splval.val_int.value = atoi(yytext);
        }
        tmp_splattr.value_p = &tmp_splval;
        yylval = new SplAstNode("INT", tmp_splattr, tmp_splloc);
        return INT;
    #endif
}
{float} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: float      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_FLOAT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            tmp_splval.val_float.raw = strdup(yytext);
        #endif
        tmp_splval.val_float.value = (float)atof(yytext);
        tmp_splattr.value_p = &tmp_splval;
        yylval = new SplAstNode("FLOAT", tmp_splattr, tmp_splloc);
        return FLOAT;
    #endif
}
{char} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: char       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_CHAR, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            tmp_splval.val_char.raw = strdup(yytext);
        #endif
        if (yytext[1] == '\\') { 
            char *hex_str = strndup(yytext + 3, 2);
            tmp_splval.val_char.value = strtol(hex_str, nullptr, 16);
            free(hex_str);
        } else {
            tmp_splval.val_char.value = yytext[1];
        }
        tmp_splattr.value_p = &tmp_splval;
        yylval = new SplAstNode("CHAR", tmp_splattr, tmp_splloc);
        return CHAR;
    #endif
}
{type} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: type       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TYPE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        tmp_splval.val_type = strdup(yytext);
        tmp_splattr.value_p = &tmp_splval;
        yylval = new SplAstNode("TYPE", tmp_splattr, tmp_splloc);
        return TYPE;
    #endif
}
{struct} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: struct     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_STRUCT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("STRUCT", tmp_splattr, tmp_splloc);
        return STRUCT;
    #endif
}
{if} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: if         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_IF, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("IF", tmp_splattr, tmp_splloc);
        return IF;
    #endif
}
{else} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: else       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_ELSE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ELSE", tmp_splattr, tmp_splloc);
        return ELSE;
    #endif
}
{while} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: while      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_WHILE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("WHILE", tmp_splattr, tmp_splloc);
        return WHILE;
    #endif
}
{for} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: for      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TERMINAL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("FOR", tmp_splattr, tmp_splloc);
        return FOR;
    #endif
}
{return} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: return     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_RETURN, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("RETURN", tmp_splattr, tmp_splloc);
        return RETURN;
    #endif
}
{err_id_digit} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: identifier cannot start with digit : %s\n", yylineno, yycolumn - yyleng, yytext); 
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: identifier cannot start with digit : %s\n", yylineno, yytext); 
    hasError = true;
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TERMINAL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ILLEGAL_ID", tmp_splattr, tmp_splloc);
        return ID;
    #endif
}
{id} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: id         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_ID, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        tmp_splval.val_id = strdup(yytext);
        tmp_splattr.value_p = &tmp_splval;
        yylval = new SplAstNode("ID", tmp_splattr, tmp_splloc);
        return ID;
    #endif
}
{dot} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: dot        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_DOT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("DOT", tmp_splattr, tmp_splloc);
        return DOT;
    #endif
}
{semi} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: semi       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_SEMI, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("SEMI", tmp_splattr, tmp_splloc);
        return SEMI;
    #endif
}
{comma} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: comma      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_COMMA, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("COMMA", tmp_splattr, tmp_splloc);
        return COMMA;
    #endif
}
{assign} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: assign     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_ASSIGN, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ASSIGN", tmp_splattr, tmp_splloc);
        return ASSIGN;
    #endif
}
{lt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_LT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("LT", tmp_splattr, tmp_splloc);
        return LT;
    #endif
}
{le} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: le         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_LE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("LE", tmp_splattr, tmp_splloc);
        return LE;
    #endif
}
{gt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: gt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_GT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("GT", tmp_splattr, tmp_splloc);
        return GT;
    #endif
}
{ge} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ge         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_GE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("GE", tmp_splattr, tmp_splloc);
        return GE;
    #endif
}
{ne} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ne         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_NE, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("NE", tmp_splattr, tmp_splloc);
        return NE;
    #endif
}
{eq} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: eq         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_EQ, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("EQ", tmp_splattr, tmp_splloc);
        return EQ;
    #endif
}
{plus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: plus       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_PLUS, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("PLUS", tmp_splattr, tmp_splloc);
        return PLUS;
    #endif
}
{minus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: minus      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_MINUS, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("MINUS", tmp_splattr, tmp_splloc);
        return MINUS;
    #endif
}
{mul} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: mul        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_MUL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("MUL", tmp_splattr, tmp_splloc);
        return MUL;
    #endif
}
{div} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: div        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_DIV, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("DIV", tmp_splattr, tmp_splloc);
        return DIV;
    #endif
}
{and} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: and        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_AND, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("AND", tmp_splattr, tmp_splloc);
        return AND;
    #endif
}
{or} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: or         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_OR, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("OR", tmp_splattr, tmp_splloc);
        return OR;
    #endif
}
{not} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: not        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_NOT, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("NOT", tmp_splattr, tmp_splloc);
        return NOT;
    #endif
}
{lp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_LP, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("LP", tmp_splattr, tmp_splloc);
        return LP;
    #endif
}
{rp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_RP, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("RP", tmp_splattr, tmp_splloc);
        return RP;
    #endif
}
{lb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_LB, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("LB", tmp_splattr, tmp_splloc);
        return LB;
    #endif
}
{rb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_RB, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("RB", tmp_splattr, tmp_splloc);
        return RB;
    #endif
}
{lc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_LC, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("LC", tmp_splattr, tmp_splloc);
        return LC;
    #endif
}
{rc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_RC, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("RC", tmp_splattr, tmp_splloc);
        return RC;
    #endif
}

. {
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: unknown lexeme: %s", yylineno, yytext);
    hasError = true;
    char ch;
    // keeps reading printable characters (except for blank)
    while (ch = yyinput(), ch >= 0x21 && ch <= 0x7e) {
        if (ch == ';') {
            unput(';');
            break;
        }
        putchar(ch);
    }
    putchar('\n');
    #if !defined(SPL_LEXER_STANDALONE)
        tmp_splattr = {SPL_TERMINAL, nullptr};
        yyltype_to_splloc(&yylloc, &tmp_splloc);
        yylval = new SplAstNode("ILLEGAL_ID", tmp_splattr, tmp_splloc);
        return ID;
    #endif
}

%%


