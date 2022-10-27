#include "spl-parser-body.cpp"
#include "spl-ast.hpp"
#include <cstdio>

SplAstNode *prog = nullptr;

int main(int argc, char **argv){
    char *file_path;
    if(argc < 2){
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return EXIT_FAIL;
    } else if(argc == 2){
        file_path = argv[1];
        if(!(yyin = fopen(file_path, "r"))){
            perror(argv[1]);
            return EXIT_FAIL;
        }
        yyrestart(yyin);
        // yydebug = 1;  // uncomment this line to enable bison debug output
        yyparse();
        if (!hasError) {
            prog->print_formatted(0, 2);
        }
        return EXIT_OK;
    } else{
        fputs("Too many arguments! Expected: 2.\n", stderr);
        return EXIT_FAIL;
    }
}
