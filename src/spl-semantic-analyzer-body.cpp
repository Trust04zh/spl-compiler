#include <exception>
#include <string>

#include "spl-ast.hpp"
#include "spl-parser-module.cpp"
#include "spl-semantic-error.hpp"

extern SplAstNode *prog;
extern bool hasError;

extern VariableSymbolTable symbols_var;
extern StructSymbolTable symbols_struct;
extern FunctionSymbolTable symbols_func;

SplExpExactType *latest_specifier_exact_type = nullptr;

void uninstall_specifier() {
    if (latest_specifier_exact_type != nullptr) {
        delete latest_specifier_exact_type;
        latest_specifier_exact_type = nullptr;
    }
}
void install_specifier(SplExpExactType *type) {
    uninstall_specifier();
    latest_specifier_exact_type = new SplExpExactType(*type);
}

SplFunctionSymbol *latest_function = nullptr;
void install_function() {
    assert(latest_function == nullptr);
    latest_function = new SplFunctionSymbol();
}
void uninstall_function() {
    // do not delete latest_function, since it is moved to symbols_func
    latest_function = nullptr;
}

// postorder traverse the AST and do semantic analysis
void traverse(SplAstNode *current, SplAstNode *parent);

void spl_semantic_analysis() {
    traverse(prog, nullptr);
    return;
}

void traverse(SplAstNode *current, SplAstNode *parent) {
    for (auto child : current->children) {
        traverse(child, current);
    }
    switch (current->attr.type) { // rules at end of production
    case SplAstNodeType::SPL_SPECIFIER: {
        SplAstNode *node = current->children[0];
        if (node->attr.type == SplAstNodeType::SPL_TYPE) {
            // Specifier -> TYPE
            std::string type_str(node->attr.value.value().val_type);
            SplExpType type;
            if (type_str == "int") {
                type = SPL_EXP_INT;
            } else if (type_str == "float") {
                type = SPL_EXP_FLOAT;
            } else if (type_str == "char") {
                type = SPL_EXP_CHAR;
            } else {
                throw std::runtime_error("unknown type: " + type_str);
            }
            auto *exact_type = new SplExpExactType(type);
            current->attr.value = SplVal{.val_specifier = {exact_type}};
            install_specifier(exact_type);
        } else if (node->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER) {
            // Specifier -> StructSpecifier
            // TODO: handle struct specifier
        } else {
            throw std::runtime_error("specifier has child of: " +
                                     std::to_string(node->attr.type));
        }
        break;
    }
    case SplAstNodeType::SPL_STRUCTSPECIFIER: {
        if (current->children.size() == 5) {
            // StructSpecifier -> STRUCT ID LC DefList RC
            // TODO: install struct symbol
        } else if (current->children.size() == 2) {
            // StructSpecifier -> STRUCT ID
            std::string name(current->children[1]->attr.value.value().val_id);
            if (symbols_struct.find(name) != symbols_struct.cend()) {
                // TODO: construct struct specifier
            } else {
                // TODO: declare struct / undeclared usage
            }
        } else {
            assert(false);
        }
        break;
    }
    case SplAstNodeType::SPL_VARDEC: {
        if (current->children.size() == 1) {
            // VarDec -> ID
            current->attr.value = SplVal{.val_vardec{
                new std::string(
                    current->children[0]->attr.value.value().val_id),
                new SplExpExactType(*latest_specifier_exact_type)}};
        } else if (current->children.size() == 4) {
            // VarDec -> VarDec LB INT RB
            auto &value_prev =
                current->children[0]->attr.value.value().val_vardec;
            current->attr.value = SplVal{
                .val_vardec{new std::string(*value_prev.name),
                            new SplExpExactType(value_prev.type->exp_type)}};
            auto &value = current->attr.value.value().val_vardec;
            value.type->is_array = true;
            std::vector<int> &dims = value.type->dimensions;
            if (value_prev.type->is_array) {
                std::vector<int> &dims_prev = value_prev.type->dimensions;
                dims.insert(dims.begin(), dims_prev.begin(), dims_prev.end());
            }
            dims.push_back(
                current->children[2]->attr.value.value().val_int.value);
        } else {
            assert(false);
        }
        if (parent != nullptr &&
            parent->attr.type != SplAstNodeType::SPL_VARDEC) {
            // ExtDecList -> *VarDec*
            // ExtDecList -> *VarDec* COMMA ExtDecList
            // ParamDec -> Specifier *VarDec*
            // Dec -> *VarDec*
            // Dec -> *VarDec* ASSIGNOP Exp
            // install variable symbol
            auto &v_vardec = current->attr.value.value().val_vardec;
            SplVariableSymbol *symbol =
                new SplVariableSymbol(*v_vardec.name, v_vardec.type);
            VariableSymbolTable::install_symbol(symbols_var, symbol);
        }
        break;
        // FunDec
    }
    case SplAstNodeType::SPL_FUNDEC: {
        FunctionSymbolTable::install_symbol(symbols_func, latest_function);
        uninstall_function();
        break;
    case SplAstNodeType::SPL_PARAMDEC:
        // ParamDec -> Specifier VarDec
        // install function param for funcition symbol
        auto it = symbols_var.find(
            *current->children[1]->attr.value.value().val_vardec.name);
        latest_function->params.push_back(it);
        // uninstall specifier
        uninstall_specifier();
        break;
    }
    case SplAstNodeType::SPL_DEC: {
        if (current->children.size() == 3) {
            // Dev -> VarDec ASSIGN Exp
            // TODO: initialize variable
            // TODO: check type compatibility during assignment
        }
        break;
    }
    case SplAstNodeType::SPL_ID: {
        if (parent != nullptr &&
            parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
            // FunDec -> ID LP VarList RP
            // FunDec -> ID LP RP
            install_function();
            latest_function->return_type =
                new SplExpExactType(*latest_specifier_exact_type);
            latest_function->name =
                std::string(current->attr.value.value().val_id);
        }
        break;
    }
    case SplAstNodeType::SPL_SEMI: {
        // uninstall specifier
        uninstall_specifier();
        break;
    }
    case SplAstNodeType::SPL_EXP: {
        if (current->children.size() == 1) {
            switch (current->children[0]->attr.type) {
            case SplAstNodeType::SPL_ID: {
                // Exp -> ID
                std::string id(current->children[0]->attr.value.value().val_id);
                auto it = symbols_var.find(id);
                if (it != symbols_var.end()) {
                    current->attr.value = SplVal{.val_exp{
                        new SplExpExactType(*(it->second->type)), true}};
                } else {
                    report_semantic_error(1, current->children[0]);
                    // FIXME: error recovery
                }
                break;
            }
            case SplAstNodeType::SPL_INT: {
                // Exp -> INT
                current->attr.value =
                    SplVal{.val_exp{new SplExpExactType(SPL_EXP_INT), false}};
                break;
            }
            case SplAstNodeType::SPL_FLOAT: {
                // Exp -> FLOAT
                current->attr.value =
                    SplVal{.val_exp{new SplExpExactType(SPL_EXP_FLOAT), false}};
                break;
            }
            case SplAstNodeType::SPL_CHAR: {
                // Exp -> CHAR
                current->attr.value =
                    SplVal{.val_exp{new SplExpExactType(SPL_EXP_CHAR), false}};
                break;
            }
            }
        } else if (current->children.size() == 2) {
            switch (current->children[0]->attr.type) {
            case SplAstNodeType::SPL_MINUS: {
                // Exp -> MINUS Exp
                // arithmetic operation
                auto &v_exp = current->attr.value.value().val_exp;
                v_exp.type = new SplExpExactType(
                    *(current->children[1]->attr.value.value().val_exp.type));
                v_exp.is_lvalue = false;
                break;
            }
            case SplAstNodeType::SPL_NOT: {
                // Exp -> NOT Exp
                // boolean operation
                auto &v_exp = current->attr.value.value().val_exp;
                v_exp.type = new SplExpExactType(
                    *(current->children[1]->attr.value.value().val_exp.type));
                v_exp.is_lvalue = false;
                break;
            }
            }
        } else if (current->children.size() == 3) {
            switch (current->children[1]->attr.type) {
            case SplAstNodeType::SPL_ASSIGN: {
                // Exp -> Exp ASSIGN Exp
                auto &v_exp_lhs =
                    current->children[0]->attr.value.value().val_exp;
                auto &v_exp_rhs =
                    current->children[2]->attr.value.value().val_exp;
                if (v_exp_lhs.is_lvalue) {
                    current->attr.value = SplVal{
                        .val_exp{new SplExpExactType(*v_exp_lhs.type), false}};
                } else {
                    report_semantic_error(6, current);
                    // FIXME: error recovery
                }
                if (v_exp_lhs.type->exp_type != v_exp_rhs.type->exp_type) {
                    report_semantic_error(5, current);
                    // FIXME: error recovery
                }
                break;
            }
            case SplAstNodeType::SPL_AND:
            case SplAstNodeType::SPL_OR:
            case SplAstNodeType::SPL_LT:
            case SplAstNodeType::SPL_LE:
            case SplAstNodeType::SPL_GT:
            case SplAstNodeType::SPL_GE:
            case SplAstNodeType::SPL_EQ:
            case SplAstNodeType::SPL_NE: {
                // Exp -> Exp AND Exp
                // Exp -> Exp OR Exp
                // Exp -> Exp LT Exp
                // Exp -> Exp LE Exp
                // Exp -> Exp GT Exp
                // Exp -> Exp GE Exp
                // Exp -> Exp EQ Exp
                // Exp -> Exp NE Exp
                // boolean operation
                current->attr.value =
                    SplVal{.val_exp{new SplExpExactType(SPL_EXP_INT), false}};
                // auto &v_exp_lhs =
                // current->children[0]->attr.value.value().val_exp;
                // auto &v_exp_rhs =
                // current->children[2]->attr.value.value().val_exp; if
                // (v_exp_lhs.type->exp_type == SPL_EXP_INT &&
                // v_exp_rhs.type->exp_type == SPL_EXP_INT) {
                // } else {
                //     report_semantic_error(21, current);
                //     // FIXME: error recovery
                // }
                break;
            }
            case SplAstNodeType::SPL_PLUS:
            case SplAstNodeType::SPL_MINUS:
            case SplAstNodeType::SPL_MUL:
            case SplAstNodeType::SPL_DIV: {
                // Exp -> Exp PLUS Exp
                // Exp -> Exp MINUS Exp
                // Exp -> Exp MUL Exp
                // Exp -> Exp DIV Exp
                // arithmetic operation
                auto &v_exp_lhs =
                    current->children[0]->attr.value.value().val_exp;
                auto &v_exp_rhs =
                    current->children[2]->attr.value.value().val_exp;
                if (v_exp_lhs.type->exp_type == v_exp_rhs.type->exp_type) {
                    current->attr.value = SplVal{.val_exp{
                        new SplExpExactType(v_exp_lhs.type->exp_type), false}};
                } else {
                    report_semantic_error(7, current);
                    // FIXME: error recovery
                }
                break;
            }
            case SplAstNodeType::SPL_EXP: {
                // Exp -> LP Exp RP
                auto &v_exp_inner =
                    current->children[1]->attr.value.value().val_exp;
                current->attr.value =
                    SplVal{.val_exp{new SplExpExactType(*v_exp_inner.type),
                                    v_exp_inner.is_lvalue}};
                break;
            }
            case SplAstNodeType::SPL_DOT: {
                // Exp -> Exp DOT ID
                // TODO: refer to struct member
            }
            }
        } else if (current->children[0]->attr.type == SplAstNodeType::SPL_ID &&
                   current->children[1]->attr.type == SplAstNodeType::SPL_LP) {
            // Exp -> ID LP Args RP
            // Exp -> ID LP RP
            // TODO: function call
        } else if (current->children[0]->attr.type == SplAstNodeType::SPL_EXP &&
                   current->children[1]->attr.type == SplAstNodeType::SPL_LB) {
            // Exp -> Exp LB Exp RB
            // TODO: array access
        } else {
            assert(false);
        }
    }
    default: {
        break;
    }
    }
    if (parent == nullptr) {
        return;
    }
    switch (parent->attr.type) {
    case SplAstNodeType::SPL_VARDEC:
        break;
    default:
        break;
    }
}