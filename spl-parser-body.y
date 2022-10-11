%{
    #include "spl-lexer-module.c"
    void yyerror(const char *);
%}

%locations

%token INT FLOAT CHAR
%token ID
%token TYPE
%token STRUCT
%token IF ELSE WHILE
%token RETURN
%token DOT SEMI COMMA
%token ASSIGN
%token LT LE GT GE NE EQ
%token PLUS MINUS MUL DIV
%token AND OR NOT
%token LP RP LB RB LC RC

%union {
  int type_int;
  float type_float;
  char type_char;
  char* type_id;
}

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
      ExtDefList
    ;
ExtDefList:
      ExtDef ExtDefList
    | /* empty */
    ;
ExtDef:
      Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;
ExtDecList:
      VarDec
    | VarDec COMMA ExtDecList
    ;

/* specifier */
Specifier:
      TYPE
    | StructSpecifier
    ;
StructSpecifier:
      STRUCT ID LC DefList RC
    | STRUCT ID
    ;

/* declarator */
VarDec:
      ID
    | VarDec LB INT RB
    ;
FunDec:
      ID LP VarList RP
    | ID LP RP
    ;
VarList :
      ParamDec COMMA VarList
    | ParamDec
    ;
ParamDec:
      Specifier VarDec

/* statement */
CompSt:
      LC DefList StmtList RC
    ;
StmtList:
      Stmt StmtList
    | /* empty */
    ;
Stmt:
      Exp SEMI
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt  %prec IF_WITHOUT_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
    ;

/* local definition */
DefList:
      Def DefList
    | /* empty */
    ;
Def:
      Specifier DecList SEMI
    ;
DecList:
      Dec
    | Dec COMMA DecList
    ;
Dec:
      VarDec
    | VarDec ASSIGN Exp

/* expression */
Exp:
      Exp ASSIGN Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp LT Exp
    | Exp LE Exp
    | Exp GT Exp
    | Exp GE Exp
    | Exp EQ Exp
    | Exp NE Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp MUL Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
    | CHAR
    ;
Args:
      Exp COMMA Args
    | Exp
    ;

%%

void yyerror(const char *s) {
  printf("Syntax error\n");
  exit(1);
}