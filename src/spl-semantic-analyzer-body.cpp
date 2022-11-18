#include <exception>
#include <optional>
#include <string>

#include "spl-ast.hpp"
#include "spl-parser-module.cpp"
#include "spl-semantic-error.hpp"

extern SplAstNode *prog;
extern bool hasError;

extern VariableSymbolTable symbols_var;
extern StructSymbolTable symbols_struct;
extern FunctionSymbolTable symbols_func;

std::shared_ptr<SplExpExactType> latest_specifier_exact_type = nullptr;

void uninstall_specifier() { latest_specifier_exact_type.reset(); }
void install_specifier(const std::shared_ptr<SplExpExactType> &type) {
    latest_specifier_exact_type = type;
}

std::optional<SplFunctionSymbol> latest_function = std::nullopt;
void install_function(const std::string &name,
                      const std::shared_ptr<SplExpExactType> &return_type) {
    if (latest_function != std::nullopt) {
        throw std::runtime_error("latest_function is not null when installing");
    }
    latest_function.emplace(name, return_type);
}
void uninstall_function() {
    // do not delete latest_function, since it is moved to symbols_func
    latest_function.reset();
}

std::shared_ptr<SplStructSymbol> latest_struct = nullptr;
void install_struct(const std::string &name) {
    if (latest_struct != nullptr) {
        throw std::runtime_error("latest_struct is not null when installing");
    }
    latest_struct = std::make_shared<SplStructSymbol>(name);
}
void uninstall_struct() {
    // do not delete latest_struct, since it is moved to symbols_struct
    latest_struct.reset();
}

// postorder traverse the AST and do semantic analysis
void traverse(SplAstNode *current);

void spl_semantic_analysis() {
    prog->completeParent();
    traverse(prog);
    return;
}

void traverse(SplAstNode *current) {
    auto parent = current->parent;
    for (auto child : current->children) {
        traverse(child);
    }
    // bottom-up error propagation
    switch (current->attr.type) {
    case SplAstNodeType::SPL_EXTDEF:
    case SplAstNodeType::SPL_SPECIFIER: {
        for (auto child : current->children) {
            // always propagate error
            if (child->error_propagated) {
                current->error_propagated = true;
                return;
            }
        }
        break;
    }
    case SplAstNodeType::SPL_DEC: {
        for (auto child : current->children) {
            if (child->attr.type == SplAstNodeType::SPL_VARDEC) {
                // only propagate error from invalid variable declaration
                // (i.e. failed to install variable symbol)
                if (child->error_propagated) {
                    current->error_propagated = true;
                    return;
                }
            }
        }
        break;
    }
    case SplAstNodeType::SPL_EXP: {
        for (auto child : current->children) {
            if (child->attr.type == SplAstNodeType::SPL_EXP) {
                // only propagate error from invalid sub expression
                if (child->error_propagated) {
                    current->error_propagated = true;
                    return;
                }
            }
        }
        break;
    }
    }
    // bottom-up attribute inference
    switch (current->attr.type) {
    case SplAstNodeType::SPL_EXTDEF: {
        // EXTDEF -> Specifier SEMI
        if (current->children.size() == 2) {
            if (current->children[0]->attr.val<SplValSpec>().type->isStruct()) {
                throw std::runtime_error("uncaught struct declaration");
            } else {
                // invalid statement, "int;" for example
                report_semantic_error(31, current);
                current->error_propagated = true;
                return;
            }
        }
        break;
    }
    case SplAstNodeType::SPL_SPECIFIER: {
        SplAstNode *node = current->children[0];
        if (node->attr.type == SplAstNodeType::SPL_TYPE) {
            // Specifier -> TYPE
            std::string &type_str = node->attr.val<SplValType>().val_type;
            SplExpType type;
            if (type_str == "int") {
                type = SPL_EXP_INT;
            } else if (type_str == "float") {
                type = SPL_EXP_FLOAT;
            } else if (type_str == "char") {
                type = SPL_EXP_CHAR;
            } else {
                throw std::runtime_error("unknown primitive type: " + type_str);
            }
            auto exact_type = std::make_shared<SplExpExactType>(type);
            current->attr.value = std::make_unique<SplValSpec>(exact_type);
            install_specifier(exact_type);
        } else if (node->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER) {
            // Specifier -> StructSpecifier
            // handle struct specifier
            std::string &struct_name =
                current->children[0]->attr.val<SplValStructSpec>().struct_name;
            auto exact_type = std::make_shared<SplExpExactType>(
                SplExpType::SPL_EXP_STRUCT, struct_name);
            current->attr.value = std::make_unique<SplValSpec>(exact_type);
            install_specifier(exact_type);
        } else {
            throw std::runtime_error("specifier has child of: " +
                                     std::to_string(node->attr.type));
        }
        break;
    }
    case SplAstNodeType::SPL_STRUCTSPECIFIER: {
        if (current->children.size() == 5) {
            // StructSpecifier -> STRUCT ID LC DefList RC
            StructSymbolTable::install_symbol(symbols_struct, latest_struct);
            uninstall_struct();
            std::string &name =
                current->children[1]->attr.val<SplValId>().val_id;
            current->attr.value = std::make_unique<SplValStructSpec>(name);
        } else if (current->children.size() == 2) {
            // StructSpecifier -> STRUCT ID
            std::string &name =
                current->children[1]->attr.val<SplValId>().val_id;
            if (symbols_struct.find(name) != symbols_struct.cend()) {
                current->attr.value = std::make_unique<SplValStructSpec>(name);
            } else {
                if (parent != nullptr && parent->parent != nullptr &&
                    parent->parent->attr.type == SplAstNodeType::SPL_EXTDEF) {
                    // structure declaration
                    report_semantic_error(32, current);
                    current->error_propagated = true;
                    return;
                } else {
                    // undeclared usage
                    report_semantic_error(33, current);
                    current->error_propagated = true;
                    return;
                }
            }
        } else {
            assert(false);
        }
        break;
    }
    case SplAstNodeType::SPL_VARDEC: {
        if (current->children.size() == 1) {
            // VarDec -> ID
            current->attr.value = std::make_unique<SplValVarDec>(
                current->children[0]->attr.val<SplValId>().val_id,
                latest_specifier_exact_type);
        } else if (current->children.size() == 4) {
            // VarDec -> VarDec LB INT RB
            auto &value_prev = current->children[0]->attr.val<SplValVarDec>();
            std::vector<int> dims;
            if (value_prev.type->is_array) {
                dims = value_prev.type->dimensions;
            }
            dims.push_back(std::get<int>(
                current->children[2]->attr.val<SplValValue>().value));
            current->attr.value = std::make_unique<SplValVarDec>(
                value_prev.name,
                std::make_shared<SplExpExactType>(value_prev.type->exp_type,
                                                  std::move(dims)));
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
            auto &v_vardec = current->attr.val<SplValVarDec>();
            auto symbol = std::make_shared<SplVariableSymbol>(v_vardec.name,
                                                              v_vardec.type);
            if (latest_struct == nullptr) {
                // install variable symbol

                bool ret =
                    VariableSymbolTable::install_symbol(symbols_var, symbol);
                if (!ret) {
                    report_semantic_error(3, current);
                    current->error_propagated = true;
                    return;
                }
            } else {
                // add struct member
                bool ret = VariableSymbolTable::install_symbol(
                    latest_struct->members, symbol);
                // TODO: error report
            }
        }
        break;
        // FunDec
    }
    case SplAstNodeType::SPL_FUNDEC: {
        FunctionSymbolTable::install_symbol(symbols_func,
                                            *std::move(latest_function));
        uninstall_function();
        break;
    case SplAstNodeType::SPL_PARAMDEC:
        // ParamDec -> Specifier VarDec
        // install function param for funcition symbol
        auto it = symbols_var.find(
            current->children[1]->attr.val<SplValVarDec>().name);
        latest_function->params.push_back(it);
        // uninstall specifier
        uninstall_specifier();
        break;
    }
    case SplAstNodeType::SPL_DEC: {
        if (current->children.size() == 3) {
            // Dev -> VarDec ASSIGN Exp
            auto &value_prev = current->children[0]->attr.val<SplValVarDec>();
            // TODO: initialize variable, do this in codegen phase
            auto &v_vardec = current->children[0]->attr.val<SplValVarDec>();
            auto &v_exp = current->children[2]->attr.val<SplValExp>();
            if (v_vardec.type->exp_type != v_exp.type->exp_type) {
                report_semantic_error(5, current);
                current->error_propagated = true;
                return;
            }
        }
        break;
    }
    case SplAstNodeType::SPL_ID: {
        if (parent != nullptr &&
            parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
            // FunDec -> ID LP VarList RP
            // FunDec -> ID LP RP
            // install function symbol with name
            install_function(current->attr.val<SplValId>().val_id,
                             latest_specifier_exact_type);
        } else if (parent != nullptr &&
                   parent->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER &&
                   parent->children.size() == 5) {
            // StructSpecifier -> STRUCT ID LC DefList RC
            // install struct symbol with name
            install_struct(current->attr.val<SplValId>().val_id);
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
                auto it = symbols_var.find(
                    current->children[0]->attr.val<SplValId>().val_id);
                if (it != symbols_var.end()) {
                    current->attr.value =
                        std::make_unique<SplValExp>(it->second->type, true);
                } else {
                    report_semantic_error(1, current->children[0]);
                    current->error_propagated = true;
                    return;
                }
                break;
            }
            case SplAstNodeType::SPL_INT: {
                // Exp -> INT
                current->attr.value = std::make_unique<SplValExp>(
                    std::make_shared<SplExpExactType>(SPL_EXP_INT), false);
                break;
            }
            case SplAstNodeType::SPL_FLOAT: {
                // Exp -> FLOAT
                current->attr.value = std::make_unique<SplValExp>(
                    std::make_shared<SplExpExactType>(SPL_EXP_FLOAT), false);
                break;
            }
            case SplAstNodeType::SPL_CHAR: {
                // Exp -> CHAR
                current->attr.value = std::make_unique<SplValExp>(
                    std::make_shared<SplExpExactType>(SPL_EXP_CHAR), false);
                break;
            }
            }
        } else if (current->children.size() == 2) {
            switch (current->children[0]->attr.type) {
            case SplAstNodeType::SPL_NOT:
            case SplAstNodeType::SPL_MINUS: {
                // Exp -> NOT Exp
                // boolean operation
                // Exp -> MINUS Exp
                // arithmetic operation
                current->attr.value = std::make_unique<SplValExp>(
                    current->children[1]->attr.val<SplValExp>().type, false);
                break;
            }
            }
        } else if (current->children.size() == 3) {
            switch (current->children[1]->attr.type) {
            case SplAstNodeType::SPL_ASSIGN: {
                // Exp -> Exp ASSIGN Exp
                auto &v_exp_lhs = current->children[0]->attr.val<SplValExp>();
                auto &v_exp_rhs = current->children[2]->attr.val<SplValExp>();
                if (!v_exp_lhs.is_lvalue) {
                    report_semantic_error(6, current);
                    current->error_propagated = true;
                    return;
                }
                if (v_exp_lhs.type->exp_type != v_exp_rhs.type->exp_type) {
                    report_semantic_error(5, current);
                    current->error_propagated = true;
                    return;
                }
                current->attr.value =
                    std::make_unique<SplValExp>(v_exp_lhs.type, false);
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
                current->attr.value = std::make_unique<SplValExp>(
                    std::make_shared<SplExpExactType>(SPL_EXP_INT), false);
                auto &v_exp_lhs = current->children[0]->attr.val<SplValExp>();
                auto &v_exp_rhs = current->children[2]->attr.val<SplValExp>();
                if (v_exp_lhs.type->exp_type == SPL_EXP_INT &&
                    v_exp_rhs.type->exp_type == SPL_EXP_INT) {
                    current->attr.value = std::make_unique<SplValExp>(
                        std::make_shared<SplExpExactType>(SPL_EXP_INT), false);
                } else {
                    report_semantic_error(21, current);
                    current->error_propagated = true;
                    return;
                }
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
                auto &v_exp_lhs = current->children[0]->attr.val<SplValExp>();
                auto &v_exp_rhs = current->children[2]->attr.val<SplValExp>();

                if (v_exp_lhs.type->exp_type == v_exp_rhs.type->exp_type) {
                    current->attr.value =
                        std::make_unique<SplValExp>(v_exp_lhs.type, false);
                } else {
                    report_semantic_error(7, current);
                    current->error_propagated = true;
                    return;
                }
                break;
            }
            case SplAstNodeType::SPL_EXP: {
                // Exp -> LP Exp RP
                current->attr.value = std::make_unique<SplValExp>(
                    current->children[1]->attr.val<SplValExp>());
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