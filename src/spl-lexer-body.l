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
        yylval = new SplAstNode("ILLEGAL_INT_CONST", {SPL_TERMINAL, nullptr}, SplLoc(&yylloc));
        return INT;
    #endif
}
{err_int_hex} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: hexadecimal int constant cannot start with 0 in digit part (except for 0x0) : %s\n", yylineno, yycolumn - yyleng, yytext); 
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: hexadecimal int constant cannot start with 0 in digit part (except for 0x0) : %s\n", yylineno, yytext); 
    hasError = true;
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("ILLEGAL_INT_CONST", {SPL_TERMINAL, nullptr}, SplLoc(&yylloc));
        return INT;
    #endif
}
{int} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: int        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        int value;
        if (strlen(yytext) >= 2 && (yytext[1] == 'x' || yytext[1] == 'X')) {
            value = strtol(yytext, nullptr, 16);
        } else {
            value = atoi(yytext);
        }
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            yylval = new SplAstNode("INT", {SPL_INT, std::make_unique<SplValValue>(value, strdup(yytext))}, SplLoc(&yylloc));
        #else
            yylval = new SplAstNode("INT", {SPL_INT, std::make_unique<SplValValue>(value)}, SplLoc(&yylloc));
        #endif
        return INT;
    #endif
}
{float} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: float      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            yylval = new SplAstNode("FLOAT", {SPL_FLOAT, std::make_unique<SplValValue>((float)atof(yytext), strdup(yytext))}, SplLoc(&yylloc));
        #else
            yylval = new SplAstNode("FLOAT", {SPL_FLOAT, std::make_unique<SplValValue>((float)atof(yytext))}, SplLoc(&yylloc));
        #endif
        return FLOAT;
    #endif
}
{char} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: char       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        char value;
        if (yytext[1] == '\\') { 
            char *hex_str = strndup(yytext + 3, 2);
            value = strtol(hex_str, nullptr, 16);
            free(hex_str);
        } else {
            value = yytext[1];
        }
        #if !defined(SPL_LEXER_STANDALONE) && defined(SPL_PARSER_STANDALONE)
            yylval = new SplAstNode("CHAR", {SPL_CHAR, std::make_unique<SplValValue>(value, strdup(yytext))}, SplLoc(&yylloc));
        #else
            yylval = new SplAstNode("CHAR", {SPL_CHAR, std::make_unique<SplValValue>(value)}, SplLoc(&yylloc));
        #endif
        return CHAR;
    #endif
}
{type} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: type       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("TYPE", {SPL_TYPE, std::make_unique<SplValType>(strdup(yytext))}, SplLoc(&yylloc));
        return TYPE;
    #endif
}
{struct} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: struct     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("STRUCT", {SPL_STRUCT, nullptr}, SplLoc(&yylloc));
        return STRUCT;
    #endif
}
{if} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: if         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("IF", {SPL_IF, nullptr}, SplLoc(&yylloc));
        return IF;
    #endif
}
{else} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: else       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("ELSE", {SPL_ELSE, nullptr}, SplLoc(&yylloc));
        return ELSE;
    #endif
}
{while} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: while      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("WHILE", {SPL_WHILE, nullptr}, SplLoc(&yylloc));
        return WHILE;
    #endif
}
{for} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: for      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("FOR", {SPL_TERMINAL, nullptr}, SplLoc(&yylloc));
        return FOR;
    #endif
}
{return} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: return     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("RETURN", {SPL_RETURN, nullptr}, SplLoc(&yylloc));
        return RETURN;
    #endif
}
{err_id_digit} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: identifier cannot start with digit : %s\n", yylineno, yycolumn - yyleng, yytext); 
    SPL_LEXER_ERROR_PRINT("Error type A at Line %d: identifier cannot start with digit : %s\n", yylineno, yytext); 
    hasError = true;
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("ILLEGAL_ID", {SPL_TERMINAL, nullptr}, SplLoc(&yylloc));
        return ID;
    #endif
}
{id} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: id         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("ID", {SPL_ID, std::make_unique<SplValId>(strdup(yytext))}, SplLoc(&yylloc));
        return ID;
    #endif
}
{dot} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: dot        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("DOT", {SPL_DOT, nullptr}, SplLoc(&yylloc));
        return DOT;
    #endif
}
{semi} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: semi       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("SEMI", {SPL_SEMI, nullptr}, SplLoc(&yylloc));
        return SEMI;
    #endif
}
{comma} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: comma      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("COMMA", {SPL_COMMA, nullptr}, SplLoc(&yylloc));
        return COMMA;
    #endif
}
{assign} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: assign     (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("ASSIGN", {SPL_ASSIGN, nullptr}, SplLoc(&yylloc));
        return ASSIGN;
    #endif
}
{lt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("LT", {SPL_LT, nullptr}, SplLoc(&yylloc));
        return LT;
    #endif
}
{le} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: le         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("LE", {SPL_LE, nullptr}, SplLoc(&yylloc));
        return LE;
    #endif
}
{gt} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: gt         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("GT", {SPL_GT, nullptr}, SplLoc(&yylloc));
        return GT;
    #endif
}
{ge} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ge         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("GE", {SPL_GE, nullptr}, SplLoc(&yylloc));
        return GE;
    #endif
}
{ne} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: ne         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("NE", {SPL_NE, nullptr}, SplLoc(&yylloc));
        return NE;
    #endif
}
{eq} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: eq         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("EQ", {SPL_EQ, nullptr}, SplLoc(&yylloc));
        return EQ;
    #endif
}
{plus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: plus       (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("PLUS", {SPL_PLUS, nullptr}, SplLoc(&yylloc));
        return PLUS;
    #endif
}
{minus} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: minus      (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("MINUS", {SPL_MINUS, nullptr}, SplLoc(&yylloc));
        return MINUS;
    #endif
}
{mul} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: mul        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("MUL", {SPL_MUL, nullptr}, SplLoc(&yylloc));
        return MUL;
    #endif
}
{div} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: div        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("DIV", {SPL_DIV, nullptr}, SplLoc(&yylloc));
        return DIV;
    #endif
}
{and} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: and        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("AND", {SPL_AND, nullptr}, SplLoc(&yylloc));
        return AND;
    #endif
}
{or} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: or         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("OR", {SPL_OR, nullptr}, SplLoc(&yylloc));
        return OR;
    #endif
}
{not} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: not        (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("NOT", {SPL_NOT, nullptr}, SplLoc(&yylloc));
        return NOT;
    #endif
}
{lp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("LP", {SPL_LP, nullptr}, SplLoc(&yylloc));
        return LP;
    #endif
}
{rp} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rp         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("RP", {SPL_RP, nullptr}, SplLoc(&yylloc));
        return RP;
    #endif
}
{lb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("LB", {SPL_LB, nullptr}, SplLoc(&yylloc));
        return LB;
    #endif
}
{rb} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rb         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("RB", {SPL_RB, nullptr}, SplLoc(&yylloc));
        return RB;
    #endif
}
{lc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: lc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("LC", {SPL_LC, nullptr}, SplLoc(&yylloc));
        return LC;
    #endif
}
{rc} {
    SPL_LEXER_DEBUG_PRINT("line %d\t col %d\t: rc         (len %d)\t : \"%s\"\n", yylineno, yycolumn - yyleng, yyleng, yytext);
    #if !defined(SPL_LEXER_STANDALONE)
        yylval = new SplAstNode("RC", {SPL_RC, nullptr}, SplLoc(&yylloc));
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
        yylval = new SplAstNode("ILLEGAL_ID", {SPL_TERMINAL, nullptr}, SplLoc(&yylloc));
        return ID;
    #endif
}

%%


