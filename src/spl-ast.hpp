#ifndef SPL_AST_HPP
#define SPL_AST_HPP

#include "spl-ir.hpp"
#include <cassert>
#include <cstdio>
#include <forward_list>
#include <iostream>
#include <list>
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

struct IrVarDesc {
    std::string var;
    bool is_addr = false;
    std::forward_list<std::shared_ptr<Patchable>> truelist, falselist;
    std::shared_ptr<SplIrLabelInstruction> label;
};

struct SplValExp : public SplVal {
    std::shared_ptr<SplExpExactType> type;
    bool is_lvalue;
    IrVarDesc ir_var;
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

struct SplValArgs : public SplVal {
    std::vector<std::shared_ptr<SplExpExactType>>
        arg_types; // arguments are stored by reverse order
    SplValArgs(const std::vector<std::shared_ptr<SplExpExactType>> &arg_types)
        : SplVal{"Args"}, arg_types(arg_types) {}
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

    template <typename T> T &val() const { return static_cast<T &>(*value); }
};

enum SplExpType {
    SPL_EXP_INIT = 0,
    SPL_EXP_INT,
    SPL_EXP_FLOAT,
    SPL_EXP_CHAR,
    SPL_EXP_STRUCT,
};

struct SplExpExactType {
    const SplExpType exp_type;
    const std::string struct_name; // if exp_type == SplExpType::SPL_EXP_STRUCT
    int array_idx{-1};
    const std::shared_ptr<std::vector<int>> dims; // if is_array()
    int size;
    int primitive_size; // raw size

    explicit SplExpExactType(SplExpType exp_type)
        : exp_type(exp_type), size{exp_type == SPL_EXP_CHAR ? 1 : 4},
          primitive_size{size} {}
    SplExpExactType(SplExpType exp_type, const std::string &struct_name,
                    int size)
        : exp_type(exp_type), struct_name(struct_name), size(size),
          primitive_size(size) {}
    SplExpExactType(SplExpType exp_type, std::shared_ptr<std::vector<int>> dims,
                    int size, int primitive_size, const int array_idx = 0)
        : exp_type(exp_type), dims(dims), array_idx(array_idx),
          size(size), primitive_size{primitive_size} {}
    SplExpExactType(SplExpType exp_type, const std::string &struct_name,
                    std::shared_ptr<std::vector<int>> dims, int size,
                    int primitive_size, const int array_idx = 0)
        : exp_type(exp_type), struct_name(struct_name), dims(dims),
          array_idx(array_idx), size(size), primitive_size{primitive_size} {}

    SplExpExactType(const SplExpExactType &) = default;
    bool is_array() const { return array_idx != -1; }
    void step_array_idx() {
        ++array_idx;
        if (array_idx == dims->size()) {
            array_idx = -1;
        }
    }
    int get_arr_size_for_current_index() {
        int res = primitive_size;
        for (int i = array_idx + 1; i < dims->size(); i++) {
            res *= dims->at(i);
        }
        return res;
    }
    bool operator==(const SplExpExactType &rhs) const {
        if (exp_type != rhs.exp_type) {
            return false;
        }
        if (exp_type == SplExpType::SPL_EXP_STRUCT &&
            struct_name != rhs.struct_name) {
            return false;
        }
        if (is_array() != rhs.is_array()) {
            return false;
        }
        if (is_array() &&
            dims->size() - array_idx != rhs.dims->size() - rhs.array_idx) {
            return false;
        }
        return true;
    }
    bool operator!=(const SplExpExactType &rhs) const {
        return !(*this == rhs);
    }
    bool is_array_or_struct() const {
        return is_array() || exp_type == SPL_EXP_STRUCT;
    }
};

enum SplSymbolType { SPL_SYM_VAR, SPL_SYM_STRUCT, SPL_SYM_FUNC };

class SplSymbol {
  public:
    const std::string name;
    const SplSymbolType sym_type;
    explicit SplSymbol(const std::string &name, const SplSymbolType type)
        : name(name), sym_type(type) {}
    virtual void print() = 0;
};

class SplSymbolTable
    : public std::unordered_map<std::string, std::shared_ptr<SplSymbol>> {
  public:
    enum SplSymbolInstallResult {
        SPL_SYM_INSTALL_OK = 0,
        SPL_SYM_INSTALL_TYPE_CONFLICT,
        SPL_SYM_INSTALL_REDEF_FUNC,
        SPL_SYM_INSTALL_REDEF_STRUCT,
        SPL_SYM_INSTALL_REDEF_VAR,
    };
    static int install_symbol(SplSymbolTable &st,
                              std::shared_ptr<SplSymbol> sym) {
        auto it = st.find(sym->name);
        if (it != st.end()) {
            if (it->second->sym_type == sym->sym_type) {
                switch (sym->sym_type) {
                case SPL_SYM_VAR:
                    return SPL_SYM_INSTALL_REDEF_VAR;
                case SPL_SYM_STRUCT:
                    return SPL_SYM_INSTALL_REDEF_STRUCT;
                case SPL_SYM_FUNC:
                    return SPL_SYM_INSTALL_REDEF_FUNC;
                }
            } else {
                return SPL_SYM_INSTALL_TYPE_CONFLICT;
            }
        }
        st.emplace(sym->name, sym);
        return SPL_SYM_INSTALL_OK;
    }
    int install_symbol(std::shared_ptr<SplSymbol> sym) {
        return install_symbol(*this, sym);
    }
    void print() {
        std::cout << "Symbol Table:" << std::endl;
        for (auto it = this->cbegin(); it != this->cend(); ++it) {
            it->second->print();
        }
    }
};

class SplScope {
  private:
    std::vector<SplSymbolTable> tables_{1};

  public:
    const auto &table() const { return tables_.back(); }

    void forward() { tables_.emplace_back(); }
    void back() {
#if defined(SPL_SEMANTIC_ANALYZER_VERBOSE)
        std::cout << "pop: " << std::endl;
        print();
#endif
        tables_.pop_back();
    }
    int install_symbol(std::shared_ptr<SplSymbol> sym) {
        return tables_.back().install_symbol(sym);
    }

    template <typename T = SplSymbol>
    std::optional<std::shared_ptr<T>> lookup(const std::string &name) {
        for (auto it = tables_.crbegin(); it != tables_.crend(); it++) {
            auto &table = *it;
            auto table_it = table.find(name);
            if (table_it != table.end()) {
                return std::static_pointer_cast<T>((*table_it).second);
            }
        }
        return std::nullopt;
    }
    void print() {
        std::cout << "Scope:" << std::endl;
        for (int i = 0; i < tables_.size(); i++) {
            std::cout << "Layer " << i << std::endl;
            for (auto it = tables_[i].cbegin(); it != tables_[i].cend(); ++it) {
                it->second->print();
            }
        }
    }
};

class SplVariableSymbol : public SplSymbol {
  public:
    std::shared_ptr<SplExpExactType> var_type;
    SplVariableSymbol(const std::string &name,
                      const std::shared_ptr<SplExpExactType> &var_type)
        : SplSymbol{name, SplSymbolType::SPL_SYM_VAR}, var_type(var_type) {}
    void print() {
        std::cout << "Variable: " << name << " type: ";
        switch (var_type->exp_type) {
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
            std::cout << "struct " << var_type->struct_name;
            break;
        }
        if (var_type->is_array()) {
            std::cout << " array";
            for (auto dim : *var_type->dims) {
                std::cout << "[" << dim << "]";
            }
        }
        std::cout << std::endl;
    }
};

class SplStructSymbol : public SplSymbol {
    std::unordered_map<std::string, int> offsets;

  public:
    int install_symbol(std::shared_ptr<SplVariableSymbol> sym) {
        int res = members.install_symbol(sym);
        if (res == SplSymbolTable::SPL_SYM_INSTALL_OK) {
            offsets[sym->name] = size;
            size += sym->var_type->size;
        }
        return res;
    }

    SplSymbolTable members;
    SplStructSymbol(const std::string &name)
        : SplSymbol{name, SplSymbolType::SPL_SYM_STRUCT} {}
    int size = 0;
    int get_offset(const std::string &name) { return offsets[name]; }
    void print() {
        std::cout << "Struct: " << name << std::endl;
        members.print();
    }
};

class SplFunctionSymbol : public SplSymbol {
  public:
    // notice that spl does not support array return type
    const std::shared_ptr<SplExpExactType> return_type;
    std::vector<std::shared_ptr<SplSymbol>> params;

    SplFunctionSymbol(const std::string &name,
                      const std::shared_ptr<SplExpExactType> return_type)
        : SplSymbol{name, SplSymbolType::SPL_SYM_FUNC},
          return_type(return_type) {}
    void print() {
        std::cout << "Function: " << name << " return type: ";
        switch (return_type->exp_type) {
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
            std::cout << "struct " << return_type->struct_name;
            break;
        }
        std::cout << std::endl;
        std::cout << "Params:" << std::endl;
        for (auto &p : params) {
            p->print();
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
    SplIrInstructionList ir;

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
            assert(attr.value != nullptr);
            printf("%s: %d", name,
                   std::get<int>(attr.val<SplValValue>().value));
            break;
        case SPL_FLOAT:
            assert(attr.value != nullptr);
            // printf(": %f", attr.value->val_float.value);
            printf("%s: %f", name,
                   std::get<float>(attr.val<SplValValue>().value));
            break;
        case SPL_CHAR:
            assert(attr.value != nullptr);
            // printf(": %c", attr.value->val_char.value);
            printf("%s: %c", name,
                   std::get<char>(attr.val<SplValValue>().value));
            break;
        case SPL_ID:
            assert(attr.value != nullptr);
            printf("%s: %s", name, attr.val<SplValId>().val_id.c_str());
            break;
        case SPL_TYPE:
            assert(attr.value != nullptr);
            printf("%s: %s", name, attr.val<SplValType>().val_type.c_str());
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