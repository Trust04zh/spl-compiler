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

  SplAttr tmp_splattr;
  //  The new node will copy the content of tmp_splval (if passed as argument) to a new SplVal.
  SplVal tmp_splval;
  SplLoc tmp_splloc;

  #include "spl-lexer-module.cpp"

  // L-attributed

  // I really have no idea how to name this variable so that it becomes not so ugly...
  // **use getter** to get this
  SplExpExactType *latest_specifier_exact_type = new SplExpExactType();
  bool specifier_installed = false;
  void install_specifier(SplExpExactType* type) {
    specifier_installed = true;
    SplExpExactType::dup(type, latest_specifier_exact_type);
  }
  void uninstall_specifier() {
    specifier_installed = false;
  }
  SplExpExactType* get_latest_specifier_exact_type() {
    assert(specifier_installed);
    return latest_specifier_exact_type;
  }

  SplFunctionSymbol *latest_function_symbol = nullptr;
  void prepare_and_install_return_type_for_function_symbol() {
    // prepare a new function symbol
    latest_function_symbol = new SplFunctionSymbol();
    // install return type for function symbol
    SplExpExactType::dup(get_latest_specifier_exact_type(), latest_function_symbol->return_type);
  }

  extern VariableSymbolTable symbols_var;
  extern StructSymbolTable symbols_struct;
  extern FunctionSymbolTable symbols_func;

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
    /* "int;" ? */
    | Specifier SEMI  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDef", tmp_splattr, tmp_splloc, $1, $2); }
    /* function definition */
    | Specifier FunDec CompSt  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("ExtDef", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
/* Can declare but cannot define global variables */
ExtDecList:
      ExtVarDec  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          $$ = new SplAstNode("ExtDecList", tmp_splattr, tmp_splloc, $1);
        }
    | ExtVarDec COMMA ExtDecList  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          $$ = new SplAstNode("ExtDecList", tmp_splattr, tmp_splloc, $1, $2, $3);
        }
    ;
ExtVarDec:
      VarDec  {
          $$ = $1;  // dummy node
          #if !defined(SPL_PARSER_STANDALONE)
            // install global variables
            SplVariableSymbol *symbol = new SplVariableSymbol(
              std::string($1->attr.value_p->val_vardec.name),
              $1->attr.value_p->val_vardec.type
            );
            VariableSymbolTable::install_symbol(symbols_var, symbol);
          #endif
        }
    ;

/* specifier */
Specifier:
      TYPE  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_SPECIFIER, nullptr};
            std::string type_str($1->attr.value_p->val_type);
            SplExpExactType *&type = tmp_splval.val_specifier.type;
            if (type_str == "int") {
              type->exp_type = SPL_EXP_INT;
            } else if (type_str == "float") {
              type->exp_type = SPL_EXP_FLOAT;
            } else if (type_str == "char") {
              type->exp_type = SPL_EXP_CHAR;
            } else {
              assert(false);
            }
            type->is_array = false;
            tmp_splattr.value_p = &tmp_splval;
            install_specifier(type);
          #endif
          $$ = new SplAstNode("Specifier", tmp_splattr, tmp_splloc, $1);
        }
    | StructSpecifier  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_SPECIFIER, nullptr};
            // TODO: install struct symbol
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("Specifier", tmp_splattr, tmp_splloc, $1);
        }
    ;
StructSpecifier:
    /* struct definition */
      STRUCT ID LC DefList RC  {
          yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("StructSpecifier", tmp_splattr, tmp_splloc, $1, $2, $3, $4, $5);
          #if !defined(SPL_PARSER_STANDALONE)
            SplStructSymbol symbol;
            symbol.name = $2->attr.value_p->val_id;
            // TODO: install struct symbol
          #endif
        }
    /* may be struct declaration */
    | STRUCT ID  {
          yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("StructSpecifier", tmp_splattr, tmp_splloc, $1, $2);
          #if !defined(SPL_PARSER_STANDALONE)
            std::string name($2->attr.value_p->val_id);
            if (symbols_struct.find(name) != symbols_struct.cend()) {
              // TODO: construct struct specifier
            } else {
              // TODO: declare struct / undeclared usage
            }
          #endif
        }
    ;

/* declarator */
VarDec:
      ID  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_VARDEC, nullptr};
            tmp_splval.val_vardec.name = $1->attr.value_p->val_id;
            tmp_splval.val_vardec.type = new SplExpExactType();
            SplExpExactType::dup(get_latest_specifier_exact_type(), tmp_splval.val_vardec.type);
            printf("[DEBUG] A %s %d\n", $1->attr.value_p->val_id, tmp_splval.val_vardec.type->is_array); // FIXME: DEBUG
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("VarDec", tmp_splattr, tmp_splloc, $1);
        }
    | VarDec LB INT RB  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_VARDEC, nullptr};
            tmp_splval.val_vardec.name = $1->attr.value_p->val_vardec.name;
            tmp_splval.val_vardec.type = new SplExpExactType();
            tmp_splval.val_vardec.type->exp_type = get_latest_specifier_exact_type()->exp_type;
            printf("[DEBUG] B %s %d\n", $1->attr.value_p->val_vardec.name, $1->attr.value_p->val_vardec.type->is_array); // FIXME: DEBUG
            tmp_splval.val_vardec.type->is_array = true;
            std::vector<int> *&dim = tmp_splval.val_vardec.type->dimensions;
            printf("[DEBUG] B %s %d\n", $1->attr.value_p->val_vardec.name, $1->attr.value_p->val_vardec.type->is_array); // FIXME: DEBUG
            dim = new std::vector<int>();
            if ($1->attr.value_p->val_vardec.type->is_array) {
              std::vector<int> *&dim_prev = $1->attr.value_p->val_vardec.type->dimensions;
              dim->insert(dim->end(), dim_prev->begin(), dim_prev->end());
            }
            dim->push_back($3->attr.value_p->val_int.value);
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("VarDec", tmp_splattr, tmp_splloc, $1, $2, $3, $4);
        }
    ;
FunDec:
      ID LP {
          #if !defined(SPL_PARSER_STANDALONE)
            prepare_and_install_return_type_for_function_symbol();
          #endif
        } VarList RP  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            latest_function_symbol->name = std::string($1->attr.value_p->val_id);
            FunctionSymbolTable::install_symbol(symbols_func, latest_function_symbol);
            latest_function_symbol = nullptr;
          #endif
          $$ = new SplAstNode("FunDec", tmp_splattr, tmp_splloc, $1, $2, $3, $4);
        }
    | ID LP {
          #if !defined(SPL_PARSER_STANDALONE)
            prepare_and_install_return_type_for_function_symbol();
          #endif
        } RP  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            latest_function_symbol->name = std::string($1->attr.value_p->val_id);
            FunctionSymbolTable::install_symbol(symbols_func, latest_function_symbol);
            latest_function_symbol = nullptr;
          #endif
          $$ = new SplAstNode("FunDec", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
VarList:
      ParamDec COMMA VarList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarList", tmp_splattr, tmp_splloc, $1, $2, $3); }
    | ParamDec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("VarList", tmp_splattr, tmp_splloc, $1); }
    ;
ParamDec:
      Specifier VarDec  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            // install variables in function parameter definitions
            SplVariableSymbol *symbol = new SplVariableSymbol(
              std::string($2->attr.value_p->val_vardec.name),
              $2->attr.value_p->val_vardec.type
            );
            VariableSymbolTable::install_symbol(symbols_var, symbol);
            uninstall_specifier();
            // install parameter for function symbol
            latest_function_symbol->params.push_back(new SplExpExactType());
            SplExpExactType::dup(symbol->type, latest_function_symbol->params.back());
          #endif
          $$ = new SplAstNode("ParamDec", tmp_splattr, tmp_splloc, $1, $2);
        }

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
    /* A single definition statement (can have multiple definitions of variable) */
      Specifier DecList SEMI  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            // uninstall declaration type
            uninstall_specifier();
          #endif
          $$ = new SplAstNode("Def", tmp_splattr, tmp_splloc, $1, $2, $3);
        }
    | Specifier DecList error { printf("Error type B at Line %d: missing semi at the end of definition \n", yylloc.first_line); }
    ;
DecList:
    /* A single declaration */
      Dec  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("DecList", tmp_splattr, tmp_splloc, $1); }
    | Dec COMMA DecList  { yyltype_to_splloc(&@$, &tmp_splloc); tmp_splattr = {SPL_NONTERMINAL, nullptr}; $$ = new SplAstNode("DecList", tmp_splattr, tmp_splloc, $1, $2, $3); }
    ;
Dec:
    /* declaration (without value assignment) */
      VarDec  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            // install local variables
            SplVariableSymbol *symbol = new SplVariableSymbol(
              std::string($1->attr.value_p->val_vardec.name),
              $1->attr.value_p->val_vardec.type
            );
            VariableSymbolTable::install_symbol(symbols_var, symbol);
          #endif
          $$ = new SplAstNode("Dec", tmp_splattr, tmp_splloc, $1);
        }
    /* definition (with value assignment) */
    | VarDec ASSIGN Exp  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #if !defined(SPL_PARSER_STANDALONE)
            // install local variables
            SplVariableSymbol *symbol = new SplVariableSymbol(
              std::string($1->attr.value_p->val_vardec.name),
              $1->attr.value_p->val_vardec.type
            );
            VariableSymbolTable::install_symbol(symbols_var, symbol);
            // TODO: initialize variable
            // TODO: check type compatibility during assignment
          #endif
          $$ = new SplAstNode("Dec", tmp_splattr, tmp_splloc, $1, $2, $3);
        }

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
    | ID  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_EXP, nullptr};
            std::string id = std::string($1->attr.value_p->val_id);
            auto it = symbols_var.find(id);
            if (it != symbols_var.end()) {
              SplExpExactType::dup(it->second->type, tmp_splval.val_exp.type);
              tmp_splval.val_exp.is_lvalue = true;
            } else {
              // TODO: add error report
              printf("Error: %s is not defined\n", $1->attr.value_p->val_id);
              exit(1);
            }
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1);
        }
    | INT  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_EXP, nullptr};
            tmp_splval.val_exp.type = SplExpExactType::create_int();
            tmp_splval.val_exp.is_lvalue = false;
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1);
        }
    | FLOAT  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_EXP, nullptr};
            tmp_splval.val_exp.type = SplExpExactType::create_float();
            tmp_splval.val_exp.is_lvalue = false;
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1);
        }
    | CHAR  {
          yyltype_to_splloc(&@$, &tmp_splloc);
          #if defined(SPL_PARSER_STANDALONE)
            tmp_splattr = {SPL_NONTERMINAL, nullptr};
          #else
            tmp_splattr = {SPL_EXP, nullptr};
            tmp_splval.val_exp.type = SplExpExactType::create_char();
            tmp_splval.val_exp.is_lvalue = false;
            tmp_splattr.value_p = &tmp_splval;
          #endif
          $$ = new SplAstNode("Exp", tmp_splattr, tmp_splloc, $1);
        }
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