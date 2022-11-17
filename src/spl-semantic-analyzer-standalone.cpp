#define SPL_SEMANTIC_ANALYZER_VERBOSE
// uncomment this line to enable verbose output in semantic analyzer
#include "spl-ast.hpp"
#include "spl-semantic-analyzer-body.cpp"
#include "spl-semantic-error.hpp"
#include <cstdio>

SplAstNode *prog = nullptr;
bool hasError = false;

// global symbol table for variables
VariableSymbolTable symbols_var;
// global symbol table for structs
StructSymbolTable symbols_struct;
// global symbol table for functions
FunctionSymbolTable symbols_func;

int main(int argc, char **argv) {
    char *file_path;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAIL;
    } else if (argc == 2) {
        file_path = argv[1];
        if (!(yyin = fopen(file_path, "r"))) {
            perror(argv[1]);
            return EXIT_FAIL;
        }

        // apply parser
        yyrestart(yyin);
        // yydebug = 1;  // uncomment this line to enable bison debug output
        yyparse();

        // apply semantic analyzer on prog
        spl_semantic_analysis();

#if defined(SPL_SEMANTIC_ANALYZER_VERBOSE)
        symbols_var.print();
        symbols_func.print();
#endif

        delete prog;

        if (!hasError) { // if there is no error, print nothing
            return EXIT_OK;
        } else {
            return EXIT_FAIL;
        }
    } else {
        fputs("Too many arguments! Expected: 2.\n", stderr);
        return EXIT_FAIL;
    }
}