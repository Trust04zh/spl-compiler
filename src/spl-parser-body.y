%{
  #include <cassert>

  #include "spl-parser-body.hpp"

  extern SplAstNode *prog;
  extern bool hasError;

  void yyltype_to_splloc(YYLTYPE *yylloc, SplLoc *splloc) {
    splloc->first_line = yylloc->first_line;
    splloc->first_column = yylloc->first_column;
    splloc->last_line = yylloc->last_line;
    splloc->last_column = yylloc->last_column;
  }

  //  The new node will copy the content of tmp_splval (if passed as argument) to a new SplVal.
  // SplVal tmp_splval;

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
        $$ = new SplAstNode("Program", {SPL_PROGRAM, nullptr}, SplLoc(&@$), $1);
        prog = $$;
      }
    | error  {
        printf("Error type B at Line %d: unrecoverable syntax error\n", yylloc.first_line);
        exit(EXIT_FAIL);
      }
    ;
ExtDefList:
      ExtDef ExtDefList  { $$ = new SplAstNode("ExtDefList", {SPL_EXTDEFLIST, nullptr}, SplLoc(&@$), $1, $2); }
    | /* empty */  { $$ = new SplAstNode("ExtDefList", {SPL_EXTDEFLIST, nullptr}, SplLoc(&@$)); }
    ;
ExtDef:
      Specifier ExtDecList SEMI  { $$ = new SplAstNode("ExtDef", {SPL_EXTDEF, nullptr}, SplLoc(&@$), $1, $2, $3); }
    /* "int;" ? */
    | StructSpecifier SEMI  { $$ = new SplAstNode("ExtDef", {SPL_EXTDEF, nullptr}, SplLoc(&@$), $1, $2); }
    /* function definition */
    | Specifier FunDec CompSt  { $$ = new SplAstNode("ExtDef", {SPL_EXTDEF, nullptr}, SplLoc(&@$), $1, $2, $3); }
    ;
/* Can declare but cannot define global variables */
ExtDecList:
      VarDec  { $$ = new SplAstNode("ExtDecList", {SPL_EXTDECLIST, nullptr}, SplLoc(&@$), $1); }
    | VarDec COMMA ExtDecList  { $$ = new SplAstNode("ExtDecList", {SPL_EXTDECLIST, nullptr}, SplLoc(&@$), $1, $2, $3); }
    ;

/* specifier */
Specifier:
      TYPE  { $$ = new SplAstNode("Specifier", {SPL_SPECIFIER, nullptr}, SplLoc(&@$), $1); }
    | StructSpecifier  { $$ = new SplAstNode("Specifier", {SPL_SPECIFIER, nullptr}, SplLoc(&@$), $1); }
    ;
StructSpecifier:
    /* struct definition */
      STRUCT ID LC DefList RC  { $$ = new SplAstNode("StructSpecifier", {SPL_STRUCTSPECIFIER, nullptr}, SplLoc(&@$), $1, $2, $3, $4, $5); }
    /* may be struct declaration */
    | STRUCT ID  { $$ = new SplAstNode("StructSpecifier", {SPL_STRUCTSPECIFIER, nullptr}, SplLoc(&@$), $1, $2); }
    ;

/* declarator */
VarDec:
      ID  { $$ = new SplAstNode("VarDec", {SPL_VARDEC, nullptr}, SplLoc(&@$), $1); }
    | VarDec LB INT RB  { $$ = new SplAstNode("VarDec", {SPL_VARDEC, nullptr}, SplLoc(&@$), $1, $2, $3, $4); }
    ;
FunDec:
      ID LP VarList RP  { $$ = new SplAstNode("FunDec", {SPL_FUNDEC, nullptr}, SplLoc(&@$), $1, $2, $3, $4); }
    | ID LP RP  { $$ = new SplAstNode("FunDec", {SPL_FUNDEC, nullptr}, SplLoc(&@$), $1, $2, $3); }
    ;
VarList:
      ParamDec COMMA VarList  { $$ = new SplAstNode("VarList", {SPL_VARLIST, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | ParamDec  { $$ = new SplAstNode("VarList", {SPL_VARLIST, nullptr}, SplLoc(&@$), $1); }
    ;
ParamDec:
      Specifier VarDec  { $$ = new SplAstNode("ParamDec", {SPL_PARAMDEC, nullptr}, SplLoc(&@$), $1, $2); }

/* statement */
CompSt:
      LC DefList StmtList RC  { $$ = new SplAstNode("CompSt", {SPL_COMPST, nullptr}, SplLoc(&@$), $1, $2, $3, $4); }
    | LC DefList StmtList error { printf("Error type B at Line %d: missing RC\n", yylloc.first_line); }
    ;
StmtList:
      Stmt StmtList  { $$ = new SplAstNode("StmtList", {SPL_STMTLIST, nullptr}, SplLoc(&@$), $1, $2); }
    | /* empty */  { $$ = new SplAstNode("StmtList", {SPL_STMTLIST, nullptr}, SplLoc(&@$)); }
    ;
Stmt:
      Exp SEMI  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1, $2); }
    | CompSt  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1); }
    | RETURN Exp SEMI  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | IF LP Exp RP Stmt  %prec IF_WITHOUT_ELSE  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt  { $$ = new SplAstNode("Stmt", {SPL_STMT, nullptr}, SplLoc(&@$), $1, $2, $3, $4, $5); }
/*    | FOR LP Exp SEMI Exp SEMI Exp RP Stmt { $$ = new SplAstNode("Stmt", {SPL_NONTERMINAL, nullptr}, SplLoc(&@$), $1, $2, $3, $4, $5, $6, $7, $8, $9); }*/
    | ELSE error Stmt { printf("Error type B at Line %d: incorrect else\n", yylloc.first_line); }
    | RETURN Exp error { printf("Error type B at Line %d: missing semi at the end of return statement\n", yylloc.first_line); }
    | Exp error { printf("Error type B at Line %d: missing semi at the end of statement of expression\n", yylloc.first_line); }
    ;

/* local definition */
DefList:
      Def DefList  { $$ = new SplAstNode("DefList", {SPL_DEFLIST, nullptr}, SplLoc(&@$), $1, $2); }
    | /* empty */  { $$ = new SplAstNode("DefList", {SPL_DEFLIST, nullptr}, SplLoc(&@$)); }
    ;
Def:
    /* A single definition statement (can have multiple definitions of variable) */
      Specifier DecList SEMI  { $$ = new SplAstNode("Def", {SPL_DEF, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Specifier DecList error { printf("Error type B at Line %d: missing semi at the end of definition \n", yylloc.first_line); }
    ;
DecList:
    /* A single declaration */
      Dec  { $$ = new SplAstNode("DecList", {SPL_DECLIST, nullptr}, SplLoc(&@$), $1); }
    | Dec COMMA DecList  { $$ = new SplAstNode("DecList", {SPL_DECLIST, nullptr}, SplLoc(&@$), $1, $2, $3); }
    ;
Dec:
    /* declaration (without value assignment) */
      VarDec  { $$ = new SplAstNode("Dec", {SPL_DEC, nullptr}, SplLoc(&@$), $1); }
    /* definition (with value assignment) */
    | VarDec ASSIGN Exp  { $$ = new SplAstNode("Dec", {SPL_DEC, nullptr}, SplLoc(&@$), $1, $2, $3); }

/* expression */
Exp:
      Exp ASSIGN Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp AND Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp OR Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp LT Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp LE Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp GT Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp GE Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp EQ Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp NE Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp PLUS Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp MINUS Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp MUL Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp DIV Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | LP Exp RP  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | MINUS Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2); }
    | NOT Exp  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2); }
    | ID LP Args RP  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3, $4); }
    | ID LP RP  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp LB Exp RB  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3, $4); }
    | Exp DOT ID  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | ID  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1); }
    | INT  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1); }
    | FLOAT  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1); }
    | CHAR  { $$ = new SplAstNode("Exp", {SPL_EXP, nullptr}, SplLoc(&@$), $1); }
    ;
Args:
      Exp COMMA Args  { $$ = new SplAstNode("Args", {SPL_ARGS, nullptr}, SplLoc(&@$), $1, $2, $3); }
    | Exp  { $$ = new SplAstNode("Args", {SPL_ARGS, nullptr}, SplLoc(&@$), $1); }
    ;

%%

void yyerror(const char *s) {
  hasError = true;
  /* exit(1); */
}