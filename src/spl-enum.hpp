#ifndef SPL_ENUM_HPP
#define SPL_ENUM_HPP

enum SplAstNodeType {
    SPL_DUMMY, // dummy node
    SPL_EMPTY, // empty node (in case of empty production in syntax analysis)
    SPL_INT,
    SPL_FLOAT,
    SPL_CHAR,
    SPL_TYPE,
    SPL_STRUCT,
    SPL_IF,
    SPL_ELSE,
    SPL_WHILE,
    SPL_RETURN,
    SPL_ID,
    SPL_DOT,
    SPL_SEMI,
    SPL_COMMA,
    SPL_ASSIGN,
    SPL_LT,
    SPL_LE,
    SPL_GT,
    SPL_GE,
    SPL_NE,
    SPL_EQ,
    SPL_PLUS,
    SPL_MINUS,
    SPL_MUL,
    SPL_DIV,
    SPL_AND,
    SPL_OR,
    SPL_NOT,
    SPL_LP,
    SPL_RP,
    SPL_LB,
    SPL_RB,
    SPL_LC,
    SPL_RC,       // terminals
    SPL_TERMINAL, // other terminals
    SPL_PROGRAM,
    SPL_EXTDEFLIST,
    SPL_EXTDEF,
    SPL_EXTDECLIST,
    SPL_SPECIFIER,
    SPL_STRUCTSPECIFIER,
    SPL_VARDEC,
    SPL_FUNDEC,
    SPL_VARLIST,
    SPL_PARAMDEC,
    SPL_COMPST,
    SPL_STMTLIST,
    SPL_STMT,
    SPL_DEFLIST,
    SPL_DEF,
    SPL_DECLIST,
    SPL_DEC,
    SPL_EXP,
    SPL_ARGS,        // nonterminals
    SPL_NONTERMINAL, // other nonterminals
};

#endif // SPL_ENUM_HPP
