CC = g++
FLEX = flex
BISON = bison

SOURCE_DIR = ./src
BUILD_DIR = ./build
BINARY_DIR = ./bin

CPPFLAGS ?= -g
CPPFLAGS += -I$(SOURCE_DIR) -I$(BUILD_DIR)

# source files for lexer
SPL_LEXER_BODY_L = $(SOURCE_DIR)/spl-lexer-body.l
SPL_LEXER_MODULE_CPP = $(SOURCE_DIR)/spl-lexer-module.cpp
SPL_LEXER_STANDALONE_CPP = $(SOURCE_DIR)/spl-lexer-standalone.cpp

# target files for lexer
SPL_LEXER_BODY_CPP = $(BUILD_DIR)/spl-lexer-body.cpp
SPL_LEXER_STANDALONE_OUT = $(BUILD_DIR)/spl-lexer-standalone

# source files for parser
SPL_PARSER_BODY_Y = $(SOURCE_DIR)/spl-parser-body.y
SPL_PARSER_STANDALONE_CPP = $(SOURCE_DIR)/spl-parser-standalone.cpp
SPL_AST_HPP = $(SOURCE_DIR)/spl-ast.hpp
SPL_AST_CPP = $(SOURCE_DIR)/spl-ast.cpp

# target files for parser
SPL_PARSER_BODY_CPP = $(BUILD_DIR)/spl-parser-body.cpp
SPL_PARSER_BODY_HPP = $(BUILD_DIR)/spl-parser-body.hpp
SPL_PARSER_BODY_LOG = $(BUILD_DIR)/spl-parser-body.output
SPL_PARSER_STANDALONE_OUT = $(BUILD_DIR)/spl-parser-standalone

# binary products
SPLC = $(BINARY_DIR)/splc

.PHONY: splc
splc: $(SPLC)

$(SPL_LEXER_BODY_CPP): $(SPL_LEXER_BODY_L)
	@mkdir -p $(dir $@)
	$(FLEX) -o $(SPL_LEXER_BODY_CPP) $(SPL_LEXER_BODY_L)

$(SPL_LEXER_STANDALONE_OUT): $(SPL_LEXER_BODY_CPP) $(SPL_LEXER_STANDALONE_CPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_LEXER_STANDALONE_CPP) -lfl -o $(SPL_LEXER_STANDALONE_OUT)

$(SPL_PARSER_BODY_CPP): $(SPL_PARSER_BODY_Y) $(SPL_LEXER_MODULE_CPP) $(SPL_LEXER_BODY_CPP)
	@mkdir -p $(dir $@)
	$(BISON) -t --report=state --report-file=$(SPL_PARSER_BODY_LOG) --defines=$(SPL_PARSER_BODY_HPP) \
		-o $(SPL_PARSER_BODY_CPP) $(SPL_PARSER_BODY_Y)

$(SPL_PARSER_STANDALONE_OUT): $(SPL_PARSER_BODY_CPP) $(SPL_PARSER_STANDALONE_CPP) $(SPL_AST_HPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_PARSER_STANDALONE_CPP) -lfl -ly -o $(SPL_PARSER_STANDALONE_OUT)

$(SPLC): $(SPL_PARSER_STANDALONE_OUT)
	@mkdir -p $(dir $@)
	cp $(SPL_PARSER_STANDALONE_OUT) $(SPLC)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(BINARY_DIR)
