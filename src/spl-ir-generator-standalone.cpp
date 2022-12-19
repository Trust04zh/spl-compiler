#include "spl-ast.hpp"
#include "spl-ir-generator-body.cpp"

SplAstNode *prog = nullptr;
bool hasError = false;

// scopes
SplScope symbols;

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