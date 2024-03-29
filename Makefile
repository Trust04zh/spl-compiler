CC = g++
FLEX = flex
BISON = bison

SOURCE_DIR = ./src
BUILD_DIR = ./build
BINARY_DIR = ./bin

CPPFLAGS ?= -g
CPPFLAGS += -I$(SOURCE_DIR) -I$(BUILD_DIR) -std=c++17

FLEXFLAGS ?=

BISONFLAGS ?= -t

# source files for utilities
SPL_ENUM_HPP = $(SOURCE_DIR)/spl-enum.hpp

SPL_AST_HPP = $(SOURCE_DIR)/spl-ast.hpp

SPL_SEMANTIC_ERROR_HPP = $(SOURCE_DIR)/spl-semantic-error.hpp

SPL_IR_HPP = $(SOURCE_DIR)/spl-ir.hpp

# source files for lexer
SPL_LEXER_BODY_L = $(SOURCE_DIR)/spl-lexer-body.l
SPL_LEXER_MODULE_CPP = $(SOURCE_DIR)/spl-lexer-module.cpp
SPL_LEXER_STANDALONE_CPP = $(SOURCE_DIR)/spl-lexer-standalone.cpp

# target files for lexer
SPL_LEXER_BODY_CPP = $(BUILD_DIR)/spl-lexer-body.cpp
SPL_LEXER_STANDALONE_OUT = $(BUILD_DIR)/spl-lexer-standalone

# source files for parser
SPL_PARSER_BODY_Y = $(SOURCE_DIR)/spl-parser-body.y
SPL_PARSER_MODULE_CPP = $(SOURCE_DIR)/spl-parser-module.cpp
SPL_PARSER_STANDALONE_CPP = $(SOURCE_DIR)/spl-parser-standalone.cpp

# target files for parser
SPL_PARSER_BODY_CPP = $(BUILD_DIR)/spl-parser-body.cpp
SPL_PARSER_BODY_HPP = $(BUILD_DIR)/spl-parser-body.hpp
SPL_PARSER_BODY_LOG = $(BUILD_DIR)/spl-parser-body.output
SPL_PARSER_STANDALONE_OUT = $(BUILD_DIR)/spl-parser-standalone

# source files for semantic analyzer
SPL_SEMANTIC_ANALYZER_BODY_CPP = $(SOURCE_DIR)/spl-semantic-analyzer-body.cpp
SPL_SEMANTIC_ANALYZER_MODULE_CPP = $(SOURCE_DIR)/spl-semantic-analyzer-module.cpp
SPL_SEMANTIC_ANALYZER_STANDALONE_CPP = $(SOURCE_DIR)/spl-semantic-analyzer-standalone.cpp

# target files for semantic analyzer
SPL_SEMANTIC_ANALYZER_STANDALONE_OUT = $(BUILD_DIR)/spl-semantic-analyzer-standalone

# source files for ir generator
SPL_IR_GENERATOR_BODY_CPP = $(SOURCE_DIR)/spl-ir-generator-body.cpp
SPL_IR_GENERATOR_STANDALONE_CPP = $(SOURCE_DIR)/spl-ir-generator-standalone.cpp

# target files for ir generator
SPL_IR_GENERATOR_STANDALONE_OUT = $(BUILD_DIR)/spl-ir-generator-standalone

# binary products
SPLC = $(BINARY_DIR)/splc

.PHONY: splc
splc: $(SPLC)

$(SPL_LEXER_BODY_CPP): $(SPL_LEXER_BODY_L)
	@mkdir -p $(dir $@)
	$(FLEX) $(FLEXFLAGS) -o $(SPL_LEXER_BODY_CPP) $(SPL_LEXER_BODY_L)

$(SPL_LEXER_STANDALONE_OUT): $(SPL_LEXER_BODY_CPP) $(SPL_LEXER_STANDALONE_CPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_LEXER_STANDALONE_CPP) -lfl -o $(SPL_LEXER_STANDALONE_OUT)

$(SPL_PARSER_BODY_CPP): $(SPL_PARSER_BODY_Y) $(SPL_LEXER_MODULE_CPP) $(SPL_LEXER_BODY_CPP) \
		$(SPL_ENUM_HPP)
	@mkdir -p $(dir $@)
	$(BISON) $(BISONFLAGS) --report=state --report-file=$(SPL_PARSER_BODY_LOG) \
		--defines=$(SPL_PARSER_BODY_HPP) -o $(SPL_PARSER_BODY_CPP) $(SPL_PARSER_BODY_Y)

$(SPL_PARSER_STANDALONE_OUT): $(SPL_PARSER_BODY_CPP) $(SPL_PARSER_STANDALONE_CPP) $(SPL_AST_HPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_PARSER_STANDALONE_CPP) -lfl -ly -o $(SPL_PARSER_STANDALONE_OUT)

$(SPL_SEMANTIC_ANALYZER_STANDALONE_OUT): $(SPL_SEMANTIC_ANALYZER_BODY_CPP) $(SPL_SEMANTIC_ANALYZER_STANDALONE_CPP) \
		$(SPL_ENUM_HPP) $(SPL_AST_HPP) $(SPL_SEMANTIC_ERROR_HPP) \
		$(SPL_PARSER_MODULE_CPP) $(SPL_PARSER_BODY_CPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_SEMANTIC_ANALYZER_STANDALONE_CPP) -o $(SPL_SEMANTIC_ANALYZER_STANDALONE_OUT)

$(SPL_IR_GENERATOR_STANDALONE_OUT): $(SPL_IR_GENERATOR_BODY_CPP) $(SPL_IR_GENERATOR_STANDALONE_CPP) \
		$(SPL_ENUM_HPP) $(SPL_AST_HPP) $(SPL_SEMANTIC_ERROR_HPP) $(SPL_IR_HPP) \
		$(SPL_SEMANTIC_ANALYZER_MODULE_CPP) $(SPL_SEMANTIC_ANALYZER_BODY_CPP) \
		$(SPL_PARSER_MODULE_CPP) $(SPL_PARSER_BODY_CPP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(SPL_IR_GENERATOR_STANDALONE_CPP) -o $(SPL_IR_GENERATOR_STANDALONE_OUT)

$(SPLC): $(SPL_IR_GENERATOR_STANDALONE_OUT)
	@mkdir -p $(dir $@)
	cp $(SPL_IR_GENERATOR_STANDALONE_OUT) $(SPLC)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR) $(BINARY_DIR)
