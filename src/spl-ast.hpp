#ifndef SPL_AST_HPP
#define SPL_AST_HPP

#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// iterators keeps valid after unordered_map being modified, except for iterator
// to the modified element

struct SplLoc;
struct SplAttr;

struct SplExpExactType;
struct SplTypeArray;
struct SplVariableSymbol;
struct SplStructSymbol;
struct SplFunctionSymbol;

#if !defined YYLTYPE && !defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1
#endif

// the lexer (parser) allocates fills splval for listed terminals only
struct SplVal {
    std::string debug_type;
    explicit SplVal(const std::string &type) : debug_type(type) {}
};

struct SplValValue : public SplVal {
    std::variant<int, float, char> value;
#if defined(SPL_PARSER_STANDALONE)
    std::string raw;
    SplValValue(const std::variant<int, float, char> &value,
                const std::string &raw)
        : SplVal{"VALUE"}, value(value), raw(raw) {}
#else
    SplValValue(const std::variant<int, float, char> &value)
        : SplVal{"VALUE"}, value(value) {}
#endif
};

struct SplValId : public SplVal {
    std::string val_id;
    explicit SplValId(const std::string &val_id)
        : SplVal{"ID"}, val_id(val_id) {}
};

struct SplValType : public SplVal {
    std::string val_type;
    explicit SplValType(const std::string &val_type)
        : SplVal{"TYPE"}, val_type(val_type) {}
};

struct SplValExp : public SplVal {
    std::shared_ptr<SplExpExactType> type;
    bool is_lvalue;
    SplValExp(const std::shared_ptr<SplExpExactType> &type, bool is_lvalue)
        : SplVal{"Exp"}, type(type), is_lvalue(is_lvalue) {}
};

struct SplValSpec : public SplVal {
    std::shared_ptr<SplExpExactType> type;
    explicit SplValSpec(const std::shared_ptr<SplExpExactType> &type)
        : SplVal{"Specifier"}, type(type) {}
};

struct SplValVarDec : public SplVal {
    std::string name;
    std::shared_ptr<SplExpExactType> type;
    SplValVarDec(const std::string &name,
                 const std::shared_ptr<SplExpExactType> &type)
        : SplVal{"VarDec"}, name(name), type(type) {}
};

struct SplValStructSpec : public SplVal {
    std::string struct_name;
    SplValStructSpec(const std::string &name)
        : SplVal{"StructSpecifier"}, struct_name(name) {}
};

// union SplVal {
//     struct {
// #if defined(SPL_PARSER_STANDALONE)
//         char *raw;
// #endif
//         int value;
//     } val_int;
//     struct {
// #if defined(SPL_PARSER_STANDALONE)
//         char *raw;
// #endif
//         float value;
//     } val_float;
//     struct {
// #if defined(SPL_PARSER_STANDALONE)
//         char *raw;
// #endif
//         char value;
//     } val_char;
//     char *val_id;
//     char *val_type; // val for terminal "type"
//     struct {
//         std::shared_ptr<SplExpExactType> type;
//         int is_lvalue;
//     } val_exp;
//     struct {
//         std::shared_ptr<SplExpExactType> type;
//     } val_specifier;
//     struct {
//         std::string name;
//         std::shared_ptr<SplExpExactType> type;
//     } val_vardec;
// };

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

// an imitation struct of YYLTYPE
struct SplLoc {
    int first_line;
    int first_column;
    int last_line;
    int last_column;

    explicit SplLoc(YYLTYPE *yylloc)
        : first_line{yylloc->first_line}, first_column{yylloc->first_column},
          last_line{yylloc->last_line}, last_column{yylloc->last_column} {}
};

struct SplAttr {
    SplAstNodeType type;
    std::unique_ptr<SplVal> value;

    SplAttr(SplAstNodeType type, std::unique_ptr<SplVal> &&value)
        : type(type), value(std::move(value)) {}
    SplAttr(SplAttr &&rhs) : type(rhs.type) { value.swap(rhs.value); }

    template <typename T> T &val() { return static_cast<T &>(*value); }
};

enum SplExpType {
    SPL_EXP_INIT,
    SPL_EXP_INT,
    SPL_EXP_FLOAT,
    SPL_EXP_CHAR,
    SPL_EXP_STRUCT,
};

struct SplTypeArray {
    SplExpType primitive_type;
    std::vector<int> *dimensions;
};

struct SplExpExactType {
    const SplExpType exp_type;
    const std::string struct_name; // if isStruct()
    const bool is_array{false};
    const std::vector<int> dimensions; // if is_array == true

    explicit SplExpExactType(SplExpType exp_type) : exp_type(exp_type) {}
    SplExpExactType(SplExpType exp_type, const std::string &struct_name)
        : exp_type(exp_type), struct_name(struct_name) {}
    SplExpExactType(SplExpType exp_type, std::vector<int> &&dimensions)
        : exp_type(exp_type), is_array(true), dimensions(dimensions) {}
    //   SplExpExactType(const SplExpExactType &) = default;
    bool isStruct() const { return exp_type == SplExpType::SPL_EXP_STRUCT; }
    bool operator==(const SplExpExactType &rhs) const {
        return !(exp_type != rhs.exp_type || is_array != rhs.is_array ||
                 (exp_type == SplExpType::SPL_EXP_STRUCT &&
                  struct_name != rhs.struct_name) ||
                 (is_array && dimensions.size() != rhs.dimensions.size()));
    }
};

struct SplVariableSymbol {
    const std::string name;
    std::shared_ptr<SplExpExactType> type;
    SplVariableSymbol(const std::string &name,
                      const std::shared_ptr<SplExpExactType> &type)
        : name(name), type(type) {}
};
class VariableSymbolTable
    : public std::unordered_map<std::string,
                                std::shared_ptr<SplVariableSymbol>> {
  public:
    static bool install_symbol(VariableSymbolTable &st,
                               std::shared_ptr<SplVariableSymbol> sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            return false;
        }
        st.emplace(sym->name, sym);
        return true;
    }
    void print() {
        std::cout << "Variable Symbol Table:" << std::endl;
        for (auto it = this->begin(); it != this->end(); ++it) {
            std::cout << it->first << ": ";
            switch (it->second->type->exp_type) {
            case SplExpType::SPL_EXP_INT:
                std::cout << "int";
                break;
            case SplExpType::SPL_EXP_FLOAT:
                std::cout << "float";
                break;
            case SplExpType::SPL_EXP_CHAR:
                std::cout << "char";
                break;
            case SplExpType::SPL_EXP_STRUCT:
                std::cout << "struct " << it->second->type->struct_name;
                break;
            }
            if (it->second->type->is_array) {
                std::cout << " array";
                for (auto dim : it->second->type->dimensions) {
                    std::cout << "[" << dim << "]";
                }
            }
            std::cout << std::endl;
        }
    }
};

struct SplStructSymbol {
    const std::string name;
    VariableSymbolTable members;
    SplStructSymbol(const std::string &name) : name(name) {}
};
class StructSymbolTable
    : public std::unordered_map<std::string, std::shared_ptr<SplStructSymbol>> {
  public:
    static void install_symbol(StructSymbolTable &st,
                               std::shared_ptr<SplStructSymbol> sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            fprintf(stderr, "Error: structure %s has been defined\n",
                    sym->name.c_str());
            exit(1);
        }
        st.emplace(sym->name, sym);
    }
    void print() {
        std::cout << "Struct Symbol Table:" << std::endl;
        for (auto it = this->begin(); it != this->end(); ++it) {
            std::cout << it->first << ": ";
            it->second->members.print();
        }
    }
};

struct SplFunctionSymbol {
    const std::string name;
    // notice that spl does not support array return type
    const std::shared_ptr<SplExpExactType> return_type;
    std::vector<VariableSymbolTable::iterator> params;

    SplFunctionSymbol(const std::string &name,
                      const std::shared_ptr<SplExpExactType> return_type)
        : name(name), return_type(return_type) {}
};

class FunctionSymbolTable
    : public std::unordered_map<std::string, SplFunctionSymbol> {
  public:
    static void install_symbol(FunctionSymbolTable &st,
                               SplFunctionSymbol &&sym) {
        if (st.find(sym.name) != st.end()) {
            throw std::runtime_error("Error: function " + sym.name +
                                     " has been defined\n");
        }
        st.emplace(sym.name, sym);
    }
    void print() {
        std::cout << "Function Symbol Table:" << std::endl;
        for (auto it = this->cbegin(); it != this->cend(); ++it) {
            std::cout << it->first << ": ";
            auto sym = it->second;
            switch (sym.return_type->exp_type) {
            case SplExpType::SPL_EXP_INT:
                std::cout << "int";
                break;
            case SplExpType::SPL_EXP_FLOAT:
                std::cout << "float";
                break;
            case SplExpType::SPL_EXP_CHAR:
                std::cout << "char";
                break;
            case SplExpType::SPL_EXP_STRUCT:
                std::cout << "struct " << it->second.return_type->struct_name;
                break;
            }
            std::cout << " (";
            for (auto param : sym.params) {
                auto &type = param->second->type;
                switch (type->exp_type) {
                case SplExpType::SPL_EXP_INT:
                    std::cout << "int";
                    break;
                case SplExpType::SPL_EXP_FLOAT:
                    std::cout << "float";
                    break;
                case SplExpType::SPL_EXP_CHAR:
                    std::cout << "char";
                    break;
                case SplExpType::SPL_EXP_STRUCT:
                    std::cout << "struct " << type->struct_name;
                    break;
                }
                if (type->is_array) {
                    std::cout << " array";
                    for (auto dim : type->dimensions) {
                        std::cout << "[" << dim << "]";
                    }
                }
                std::cout << ", ";
            }
            std::cout << ")" << std::endl;
        }
    }
};

struct SplAstNode {
    std::vector<SplAstNode *> children;
    const SplAstNode *parent{nullptr};
    const char *const name;
    SplAttr attr;
    SplLoc loc;
    bool error_propagated{false};

    SplAstNode(const char *name, SplAttr &&attr, const SplLoc &loc)
        : name(name), attr(std::move(attr)), loc(loc) {}

    template <typename... Args>
    SplAstNode(const char *name, SplAttr &&attr, const SplLoc &loc,
               Args... children)
        : name(name), attr(std::move(attr)), loc(loc) {
        add_child(children...);
    }

    template <typename T> void add_child(T *child) {
        children.push_back(child);
    }

    template <typename T, typename... Args>
    void add_child(T first_child, Args... rest) {
        add_child(first_child);
        add_child(rest...);
    }

    void completeParent() {
        for (auto child : children) {
            child->parent = this;
            child->completeParent();
        }
    }

    ~SplAstNode() {
        for (auto &ch : children) {
            delete ch;
        }
    }

#if defined(SPL_PARSER_STANDALONE)
    void print_formatted(int indent, int indent_step) {
        /* iterate over children tree with identation */
        switch (attr.type) {
        case SPL_PROGRAM:
        case SPL_EXTDEFLIST:
        case SPL_EXTDEF:
        case SPL_EXTDECLIST:
        case SPL_SPECIFIER:
        case SPL_STRUCTSPECIFIER:
        case SPL_VARDEC:
        case SPL_FUNDEC:
        case SPL_VARLIST:
        case SPL_PARAMDEC:
        case SPL_COMPST:
        case SPL_STMTLIST:
        case SPL_STMT:
        case SPL_DEFLIST:
        case SPL_DEF:
        case SPL_DECLIST:
        case SPL_DEC:
        case SPL_EXP:
        case SPL_ARGS:
        case SPL_NONTERMINAL:
            if (children.empty()) {
                return;
            }
        }
        for (int i = 0; i < indent; i++) {
            putchar(' ');
        }
        switch (attr.type) {
        case SPL_PROGRAM:
        case SPL_EXTDEFLIST:
        case SPL_EXTDEF:
        case SPL_EXTDECLIST:
        case SPL_SPECIFIER:
        case SPL_STRUCTSPECIFIER:
        case SPL_VARDEC:
        case SPL_FUNDEC:
        case SPL_VARLIST:
        case SPL_PARAMDEC:
        case SPL_COMPST:
        case SPL_STMTLIST:
        case SPL_STMT:
        case SPL_DEFLIST:
        case SPL_DEF:
        case SPL_DECLIST:
        case SPL_DEC:
        case SPL_EXP:
        case SPL_ARGS:
        case SPL_NONTERMINAL:
            printf("%s (%d)", name, loc.first_line);
            break;
        case SPL_STRUCT:
        case SPL_IF:
        case SPL_ELSE:
        case SPL_WHILE:
        case SPL_RETURN:
        case SPL_DOT:
        case SPL_SEMI:
        case SPL_COMMA:
        case SPL_ASSIGN:
        case SPL_LT:
        case SPL_LE:
        case SPL_GT:
        case SPL_GE:
        case SPL_NE:
        case SPL_EQ:
        case SPL_PLUS:
        case SPL_MINUS:
        case SPL_MUL:
        case SPL_DIV:
        case SPL_AND:
        case SPL_OR:
        case SPL_NOT:
        case SPL_LP:
        case SPL_RP:
        case SPL_LB:
        case SPL_RB:
        case SPL_LC:
        case SPL_RC:
        case SPL_TERMINAL:
            printf("%s", name);
            break;
        case SPL_INT:
            assert(attr.value.has_value());
            printf("%s: %d", name, attr.value->val_int.value);
            break;
        case SPL_FLOAT:
            assert(attr.value.has_value());
            // printf(": %f", attr.value->val_float.value);
            printf("%s: %s", name, attr.value->val_float.raw);
            break;
        case SPL_CHAR:
            assert(attr.value.has_value());
            // printf(": %c", attr.value->val_char.value);
            printf("%s: %s", name, attr.value->val_char.raw);
            break;
        case SPL_ID:
            assert(attr.value.has_value());
            printf("%s: %s", name, attr.value->val_id);
            break;
        case SPL_TYPE:
            assert(attr.value.has_value());
            printf("%s: %s", name, attr.value->val_type);
            break;
        default:
            assert(false);
            break;
        }
        putchar('\n');
        for (std::vector<SplAstNode *>::const_iterator it = children.cbegin();
             it != children.cend(); ++it) {
            (*it)->print_formatted(indent + indent_step, indent_step);
        }
    }
#endif
};

#endif /* SPL_AST_HPP */