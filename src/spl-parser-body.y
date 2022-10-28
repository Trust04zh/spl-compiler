%{
  #include "spl-parser-body.hpp"

  extern SplAstNode *prog;
  extern bool hasError;

  void yyltype_to_splloc(YYLTYPE *yylloc, SplLoc *splloc) {
    splloc->first_line = yylloc->first_line;
    splloc->first_column = yylloc->first_column;
    splloc->last_line = yylloc->last_line;
    splloc->last_column = yylloc->last_column;
  } 
    
  SplAttr tmp_splattr;
  SplVal tmp_splval;
  SplLoc tmp_splloc;
    
  #include "spl-lexer-module.cpp"

  void yyerror(const char *);
%}

%code requires {
  #include "spl-ast.hpp"
}

%locations
%define api.value.type { SplAstNode* }


%token INT FLOAT CHAR
%token ID
%token TYPE
%token STRUCT
%token IF ELSE WHILE FOR
%token RETURN
%token DOT SEMI COMMA
%token ASSIGN
%token LT LE GT GE NE EQ
%token PLUS MINUS MUL DIV
%token AND OR NOT
%token LP RP LB RB LC RC

%right ASSIGN
%left OR
%left AND
%left LT LE GT GE EQ NE
%left PLUS MINUS
%left MUL DIV
/* there's no need to set presedence for unary minus (neg) due to production rules */
%right NOT /* PLUS MINUS */
%left LP RP LB RB DOT

%nonassoc IF_WITHOUT_ELSE
%nonassoc ELSE

%%

/* high-level definition */
Program:
      ExtDefList  { 
        yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Program", tmp_splattr, tmp_splloc, $1); 
        prog = $$;
      }
    | error  {
        printf("Error type B at Line %d: unrecoverable syntax error\n", yylloc.first_line);
        exit(EXIT_FAIL);
      }
    ;
ExtDefList:
      ExtDef ExtDefList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDefList", tmp_splattr, tmp_splloc, $1, $2); }
    | /* empty */  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_EMPTY, nullptr}; $$ = new SplAstNode("ExtDefList", tmp_splattr, tmp_splloc); }
    ;
ExtDef:
      Specifier ExtDecList SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDef", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Specifier SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDef", tmp_splattr, tmp_splloc, $1, $2); }
    | Specifier FunDec CompSt  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDef", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
ExtDecList:
      VarDec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDecList", tmp_splattr, tmp_splloc, $1); }
    | VarDec COMMA ExtDecList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDecList", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;

/* specifier */
Specifier:
      TYPE  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Specifier", tmp_splattr, tmp_splloc, $1); }
    | StructSpecifier  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Specifier", tmp_splattr, tmp_splloc, $1); }
    ;
StructSpecifier:
      STRUCT ID LC DefList RC  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("StructSpecifier", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5); }
    | STRUCT ID  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("StructSpecifier", tmp_splattr, tmp_splloc, $1, $2); }
    ;

/* declarator */
VarDec:
      ID  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarDec", tmp_splattr, tmp_splloc, $1); }
    | VarDec LB INT RB  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarDec", tmp_splattr, tmp_splloc, $1, $2, $3, $4); }
    ;
FunDec:
      ID LP VarList RP  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("FunDec", tmp_splattr, tmp_splloc, $1, $2, $3, $4); }
    | ID LP RP  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("FunDec", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
VarList :
      ParamDec COMMA VarList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarList", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | ParamDec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarList", tmp_splattr, tmp_splloc, $1); }
    ;
ParamDec:
      Specifier VarDec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ParamDec", tmp_splattr, tmp_splloc, $1, $2); }

/* statement */
CompSt:
      LC DefList StmtList RC  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("CompSt", tmp_splattr, tmp_splloc, $1, $2, $3, $4); }
    | LC DefList StmtList error { printf("Error type B at Line %d: missing RC\n", yylloc.first_line); }
    ;
StmtList:
      Stmt StmtList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("StmtList", tmp_splattr, tmp_splloc, $1, $2); }
    | /* empty */  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_EMPTY, nullptr}; $$ = new SplAstNode("StmtList", tmp_splattr, tmp_splloc); }
    ;
Stmt:
      Exp SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2); }
    | CompSt  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1); }
    | RETURN Exp SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | IF LP Exp RP Stmt  %prec IF_WITHOUT_ELSE  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5); }
    | FOR LP Exp SEMI Exp SEMI Exp RP Stmt { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Stmt", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5, $6, $7, $8, $9); }
    | ELSE error Stmt { printf("Error type B at Line %d: incorrect else\n", yylloc.first_line); } 
    | RETURN Exp error { printf("Error type B at Line %d: missing semi at the end of return statement\n", yylloc.first_line); }
    | Exp error { printf("Error type B at Line %d: missing semi at the end of statement of expression\n", yylloc.first_line); }
    ;

/* local definition */
DefList:
      Def DefList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("DefList", tmp_splattr, tmp_splloc, $1, $2); }
    | /* empty */  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_EMPTY, nullptr}; $$ = new SplAstNode("DefList", tmp_splattr, tmp_splloc); }
    ;
Def:
      Specifier DecList SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Def", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Specifier DecList error { printf("Error type B at Line %d: missing semi at the end of definition \n", yylloc.first_line); }
    ;
DecList:
      Dec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("DecList", tmp_splattr, tmp_splloc, $1); }
    | Dec COMMA DecList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("DecList", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
Dec:
      VarDec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Dec", tmp_splattr, tmp_splloc, $1); }
    | VarDec ASSIGN Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Dec", tmp_splattr, tmp_splloc, $1, $2, $3); }

/* expression */
Exp:
      Exp ASSIGN Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp AND Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp OR Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp LT Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp LE Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp GT Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp GE Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp EQ Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp NE Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp PLUS Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp MINUS Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp MUL Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp DIV Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | LP Exp RP  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | MINUS Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2); }
    | NOT Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2); }
    | ID LP Args RP  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3, $4); }
    | ID LP RP  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp LB Exp RB  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3, $4); }
    | Exp DOT ID  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | ID  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1); }
    | INT  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1); }
    | FLOAT  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1); }
    | CHAR  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1); }
    ;
Args:
      Exp COMMA Args  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Args", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | Exp  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("Args", tmp_splattr, tmp_splloc, $1); }
    ;

%%

void yyerror(const char *s) {
  hasError = true;
  /* exit(1); */
}