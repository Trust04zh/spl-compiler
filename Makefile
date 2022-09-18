CC=gcc
FLEX=flex
BISON=bison

SPL_LEXER_BODY_C=spl-lexer-body.c
SPL_LEXER_MODULE_C=spl-lexer-module.c
SPL_LEXER_STANDALONE_C=spl-lexer-standalone.c

SPL_LEXER_BODY_L=spl-lexer-body.l
SPL_LEXER_MODULE_OUT=spl-lexer-module.out
SPL_LEXER_STANDALONE_OUT=spl-lexer-standalone.out

.PHONY: spl-lexer-module spl-lexer-standalone clean
spl-lexer-module:
	$(FLEX) -o $(SPL_LEXER_BODY_C) $(SPL_LEXER_BODY_L)
	$(CC) $(SPL_LEXER_MODULE_C) -lfl -o $(SPL_LEXER_MODULE_OUT)
spl-lexer-standalone:
	$(FLEX) -o $(SPL_LEXER_BODY_C) $(SPL_LEXER_BODY_L)
	$(CC) $(SPL_LEXER_STANDALONE_C) -lfl -o $(SPL_LEXER_STANDALONE_OUT)
clean:
	rm -f $(SPL_LEXER_BODY_C) $(SPL_LEXER_MODULE_OUT) $(SPL_LEXER_STANDALONE_OUT)
