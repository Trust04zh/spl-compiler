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

        // pre-install read and write function
        std::shared_ptr<SplFunctionSymbol> dummy_read =
            std::make_shared<SplFunctionSymbol>(
                "read",
                std::make_shared<SplExpExactType>(SplExpType::SPL_EXP_INT));
        std::shared_ptr<SplFunctionSymbol> dummy_write =
            std::make_shared<SplFunctionSymbol>(
                "write",
                std::make_shared<SplExpExactType>(SplExpType::SPL_EXP_INT));
        std::shared_ptr<SplVariableSymbol> dummy_write_param =
            std::make_shared<SplVariableSymbol>(
                "0param",
                std::make_shared<SplExpExactType>(SplExpType::SPL_EXP_INT));
        dummy_write->params.push_back(dummy_write_param);
        symbols.install_symbol(dummy_read);
        symbols.install_symbol(dummy_write);

        // apply semantic analyzer on prog
        spl_semantic_analysis();

        generate_ir();

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