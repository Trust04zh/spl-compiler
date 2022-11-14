#ifndef SPL_AST_HPP
#define SPL_AST_HPP

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cassert>

// iterators keeps valid after unordered_map being modified, except for iterator to the modified element

struct SplLoc;
struct SplAttr;
union SplVal;

struct SplExpExactType;
struct SplTypeArray;
struct SplVariableSymbol;
struct SplStructSymbol;
struct SplFunctionSymbol;

enum SplAstNodeType {
    SPL_EMPTY,  // empty node (in case of empty production in syntax analysis)
    SPL_INT, SPL_FLOAT, SPL_CHAR, SPL_TYPE, SPL_ID,  // special terminals
    SPL_TERMINAL,  // other terminals
    SPL_EXP, SPL_SPECIFIER, SPL_VARDEC, // special non-terminals
    SPL_NONTERMINAL,  // other nonterminals
};

// an imitation struct of YYLTYPE
typedef struct SplLoc SplLoc;
struct SplLoc{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};

typedef struct SplAttr SplAttr;
struct SplAttr {
    SplAstNodeType type;
    SplVal *value_p;
};

enum SplExpType {
    SPL_EXP_INT, SPL_EXP_FLOAT, SPL_EXP_CHAR,
    SPL_EXP_STRUCT,
};

struct SplTypeArray {
    SplExpType primitive_type;
    std::vector<int> *dimensions;
};

struct SplExpExactType {
    SplExpType exp_type;
    std::string *struct_name;  // if exp_type == SplExpType::SPL_EXP_STRUCT
    bool is_array;
    std::vector<int> *dimensions; // if is_array == true
    static void dup(const SplExpExactType *src, SplExpExactType *dst) {
        dst->exp_type = src->exp_type;
        if (src->exp_type == SplExpType::SPL_EXP_STRUCT) {
            *(dst->struct_name) = *(src->struct_name);  // copy std::string
        }
        if (src->is_array) {
            dst->dimensions = new std::vector<int>(*src->dimensions);
        }
        dst->is_array = src->is_array;
    }
    static SplExpExactType* create_int() {
        SplExpExactType *ret = new SplExpExactType();
        ret->exp_type = SplExpType::SPL_EXP_INT;
        ret->is_array = false;
        return ret;
    }
    static SplExpExactType* create_float() {
        SplExpExactType *ret = new SplExpExactType();
        ret->exp_type = SplExpType::SPL_EXP_FLOAT;
        ret->is_array = false;
        return ret;
    }
    static SplExpExactType* create_char() {
        SplExpExactType *ret = new SplExpExactType();
        ret->exp_type = SplExpType::SPL_EXP_CHAR;
        ret->is_array = false;
        return ret;
    }
    static bool equal(const SplExpExactType &lhs, const SplExpExactType &rhs) {
        if (lhs.exp_type != rhs.exp_type) {
            return false;
        }
        if (lhs.exp_type == SplExpType::SPL_EXP_STRUCT) {
            if (*(lhs.struct_name) != *(rhs.struct_name)) {
                return false;
            }
        }
        if (lhs.is_array != rhs.is_array) {
            return false;
        }
        if (lhs.is_array) {
            if (lhs.dimensions->size() != rhs.dimensions->size()) {
                return false;
            }
        }
        return true;
    }
};

struct SplVariableSymbol {
    std::string name;
    SplExpExactType *type;
    SplVariableSymbol(const std::string &name, const SplExpExactType *type) {
        this->name = name;
        this->type = new SplExpExactType();
        SplExpExactType::dup(type, this->type);
    }
};
class VariableSymbolTable: public std::unordered_map<std::string, SplVariableSymbol*> {
public:
    static void install_symbol(VariableSymbolTable &st, SplVariableSymbol *sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            fprintf(stderr, "Error: variable %s has been defined\n", sym->name.c_str());
            exit(1);
        }
        st.insert({sym->name, sym});
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
                    std::cout << "struct " << *(it->second->type->struct_name);
                    break;
            }
            if (it->second->type->is_array) {
                std::cout << " array";
                for (auto dim: *(it->second->type->dimensions)) {
                    std::cout << "[" << dim << "]";
                }
            }
            std::cout << std::endl;
        }
    }
};


struct SplStructSymbol {
    std::string name;
    VariableSymbolTable members;
};
class StructSymbolTable: public std::unordered_map<std::string, SplStructSymbol*> {
public:
    static void install_symbol(StructSymbolTable &st, SplStructSymbol *sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            fprintf(stderr, "Error: structure %s has been defined\n", sym->name.c_str());
            exit(1);
        }
        st.insert({sym->name, sym});
    }
};

struct SplFunctionSymbol {
    std::string name;
    SplExpExactType *return_type;  // notice that spl does not support array return type
    std::vector<SplExpExactType*> params;
    SplFunctionSymbol() {
        return_type = new SplExpExactType();
    }
};
class FunctionSymbolTable: public std::unordered_map<std::string, SplFunctionSymbol*> {
public:
    static void install_symbol(FunctionSymbolTable &st, SplFunctionSymbol *sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            fprintf(stderr, "Error: function %s has been defined\n", sym->name.c_str());
            exit(1);
        }
        st.insert({sym->name, sym});
    }
    void print() {
        std::cout << "Function Symbol Table:" << std::endl;
        for (auto it = this->cbegin(); it != this->cend(); ++it) {
            std::cout << it->first << ": ";
            switch (it->second->return_type->exp_type) {
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
                    std::cout << "struct " << *(it->second->return_type->struct_name);
                    break;
            }
            std::cout << " (";
            for (auto param: it->second->params) {
                switch (param->exp_type) {
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
                        std::cout << "struct " << *(param->struct_name);
                        break;
                }
                if (param->is_array) {
                    std::cout << " array";
                    for (auto dim: *(param->dimensions)) {
                        std::cout << "[" << dim << "]";
                    }
                }
                std::cout << ", ";
            }
            std::cout << ")" << std::endl;
        }
    }
};

typedef union SplVal SplVal;
union SplVal{
    struct {
        #if defined(SPL_PARSER_STANDALONE)
            char* raw;
        #endif
        int value;
    } val_int;
    struct {
        #if defined(SPL_PARSER_STANDALONE)
            char *raw;
        #endif
        float value;
    } val_float;
    struct {
        #if defined(SPL_PARSER_STANDALONE)
            char *raw;
        #endif
        char value;
    } val_char;
    char *val_id;
    char *val_type;  // val for terminal "type"
    struct {
        SplExpExactType *type;
        int is_lvalue;
    } val_exp;
    struct {
        SplExpExactType *type;
    } val_specifier;
    struct {
        char *name;
        SplExpExactType *type;
    } val_vardec;
};

typedef struct SplAstNode SplAstNode;
struct SplAstNode{
    std::vector<SplAstNode*> children;
    char const *name;
    SplAttr attr;
    SplLoc loc;

    SplAstNode(char const *name, SplAttr attr, SplLoc loc) {
        construct_node(name, attr, loc);
    }

    template <typename... Args>
    SplAstNode(char const *name, SplAttr attr, SplLoc loc, Args... children) {
        construct_node(name, attr, loc);
        add_child(children...);
    }

    void construct_node(char const *name, SplAttr attr, SplLoc loc) {
        this->name = name;
        this->attr.type = attr.type;
        if (attr.value_p != nullptr) {
            this->attr.value_p = new SplVal();
            *(this->attr.value_p) = *(attr.value_p);
        } else {
            this->attr.value_p = nullptr;
        }
        this->loc = loc;
    }

    template <typename T>
    void add_child(T *child) {
        children.push_back(child);
    }

    template <typename T, typename... Args>
    void add_child(T first_child, Args... rest) {
        add_child(first_child);
        add_child(rest...);
    }

#if defined(SPL_PARSER_STANDALONE)
    void print_formatted(int indent, int indent_step) {
        /* iterate over children tree with identation */
        if (attr.type == SPL_EMPTY) {
            return;
        }
        for (int i = 0; i < indent; i++) {
            putchar(' ');
        }
        printf("%s", name);
        switch (attr.type) {
            case SPL_NONTERMINAL:
                printf(" (%d)", loc.first_line);
                break;
            case SPL_TERMINAL:
                break;
            case SPL_INT:
                assert(attr.value_p != nullptr);
                printf(": %d", attr.value_p->val_int.value);
                break;
            case SPL_FLOAT:
                assert(attr.value_p != nullptr);
                // printf(": %f", attr.value_p->val_float.value);
                printf(": %s", attr.value_p->val_float.raw);
                break;
            case SPL_CHAR:
                assert(attr.value_p != nullptr);
                // printf(": %c", attr.value_p->val_char.value);
                printf(": %s", attr.value_p->val_char.raw);
                break;
            case SPL_ID:
                assert(attr.value_p != nullptr);
                printf(": %s", attr.value_p->val_id);
                break;
            case SPL_TYPE:
                assert(attr.value_p != nullptr);
                printf(": %s", attr.value_p->val_type);
                break;
            default:
                assert(false);
                break;
        }
        putchar('\n');
        for (std::vector<SplAstNode*>::const_iterator it = children.cbegin(); it != children.cend(); ++it) {
            (*it)->print_formatted(indent + indent_step, indent_step);
        }
    }
#endif

};

#endif  /* SPL_AST_HPP */