#include <exception>
#include <optional>
#include <string>

#include "spl-ast.hpp"
#include "spl-parser-module.cpp"
#include "spl-semantic-error.hpp"

extern SplAstNode *prog;
extern bool hasError;

extern SplSymbolTable symbols;

// it is NOT strictly required to uninstall side effect variables when they can
// be uninstalled, and uninstallation may be skipped due to error propagation

std::shared_ptr<SplExpExactType> latest_specifier_exact_type = nullptr;
bool broken_specifier = false;
void uninstall_specifier() {
    broken_specifier = false;
    latest_specifier_exact_type.reset();
}
void install_specifier(const std::shared_ptr<SplExpExactType> &type) {
    broken_specifier = false;
    latest_specifier_exact_type = type;
}

std::shared_ptr<SplFunctionSymbol> latest_function = nullptr;
bool broken_function = false;
void uninstall_function() {
    // do not delete latest_function, since it is moved to symbols_func
    broken_function = false;
    latest_function.reset();
}
void install_function(const std::string &name,
                      const std::shared_ptr<SplExpExactType> &return_type) {
    if (latest_function != nullptr) {
        uninstall_function();
        //        throw std::runtime_error("latest_function is not null when
        //        installing");
    }
    broken_function = false;
    latest_function = std::make_shared<SplFunctionSymbol>(name, return_type);
}

std::shared_ptr<SplStructSymbol> latest_struct = nullptr;
bool broken_struct = false;
bool on_struct_body = false;
void uninstall_struct() {
    // do not delete latest_struct, since it is moved to symbols_struct
    broken_struct = false;
    latest_struct.reset();
}
void install_struct(const std::string &name) {
    if (latest_struct != nullptr) {
        uninstall_struct();
        //        throw std::runtime_error("latest_struct is not null when
        //        installing");
    }
    broken_struct = false;
    latest_struct = std::make_shared<SplStructSymbol>(name);
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

    // error propagation for synthesis attributes
    for (auto child : current->children) {
        if (child->error_propagated) {
            current->error_propagated = true;
        }
    }

    // error propagation for inherited attrbutes
    // latest_specifier
    if (broken_specifier) {
        switch (current->attr.type) {
        case SplAstNodeType::SPL_VARDEC: {
            if (current->children.size() == 1) {
                // VarDec -> ID
                current->error_propagated = true;
            }
            break;
        }
        case SplAstNodeType::SPL_ID: {
            if (parent != nullptr &&
                parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
                // FunDec -> *ID* LP VarList RP
                // FunDec -> *ID* LP RP
                current->error_propagated = true;
            }
            break;
        }
        }
    }
    // latest_function
    if (broken_function) {
        switch (current->attr.type) {
        case SplAstNodeType::SPL_PARAMDEC: {
            // ParamDec -> Specifier VarDec
            current->error_propagated = true;
            break;
        }
        case SplAstNodeType::SPL_FUNDEC: {
            // FunDec -> ID LP VarList RP
            // FunDec -> ID LP RP
            current->error_propagated = true;
            break;
        }
        case SplAstNodeType::SPL_STMT: {
            if (current->children.size() == 3 &&
                current->children[0]->attr.type == SplAstNodeType::SPL_RETURN) {
                // Stmt -> RETURN Exp SEMI
                current->error_propagated = true;
            }
            break;
        }
        }
    }
    // latest_struct
    if (broken_struct) {
        switch (current->attr.type) {
        case SplAstNodeType::SPL_VARDEC: {
            if (parent != nullptr &&
                parent->attr.type != SplAstNodeType::SPL_VARDEC) {
                // ExtDecList -> *VarDec*
                // ExtDecList -> *VarDec* COMMA ExtDecList
                // ParamDec -> Specifier *VarDec*
                // Dec -> *VarDec*
                // Dec -> *VarDec* ASSIGN Exp
                if (on_struct_body) {
                    current->error_propagated = true;
                }
            }
            break;
        }
        case SplAstNodeType::SPL_STRUCTSPECIFIER: {
            if (current->children.size() == 5) {
                // StructSpecifier -> STRUCT ID LC DefList RC
                current->error_propagated = true;
            }
            break;
        }
        }
    }

    // mark broken side effect variables
    if (current->error_propagated) {
        // latest_specifier
        switch (current->attr.type) {
        case SplAstNodeType::SPL_SPECIFIER: { // install specifier
            // Specifier -> TYPE
            // Specifier -> StructSpecifier
            broken_specifier = true;
            break;
        }
        }
        // latest_function
        switch (current->attr.type) {
        case SplAstNodeType::SPL_ID: { // install function name
            if (parent != nullptr &&
                parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
                // FunDec -> *ID* LP VarList RP
                // FunDec -> *ID* LP RP
                broken_function = true;
            }
            break;
        }
        case SplAstNodeType::SPL_PARAMDEC: { // install function parameter
            // ParamDec -> Specifier VarDec
            broken_function = true;
            break;
        }
        case SplAstNodeType::SPL_FUNDEC: {
            // FunDec -> ID LP VarList RP
            // FunDec -> ID LP RP
            broken_function = true;
            break;
        }
        }
        // latest_struct
        switch (current->attr.type) {
        case SplAstNodeType::SPL_ID: { // install struct name
            if (parent != nullptr &&
                parent->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER &&
                parent->children.size() == 5) {
                broken_struct = true;
            }
            break;
        }
        case SplAstNodeType::SPL_VARDEC: { // install struct member
            if (parent != nullptr &&
                parent->attr.type != SplAstNodeType::SPL_VARDEC) {
                // ExtDecList -> *VarDec*
                // ExtDecList -> *VarDec* COMMA ExtDecList
                // ParamDec -> Specifier *VarDec*
                // Dec -> *VarDec*
                // Dec -> *VarDec* ASSIGN Exp
                if (on_struct_body) {
                    broken_struct = true;
                }
            }
            break;
        }
        }
        return;
    }

    // bottom-up attribute inference
    switch (current->attr.type) {
    case SplAstNodeType::SPL_EXTDEF: {
        // ExtDef -> Specifier SEMI
        if (current->children.size() == 2) {
            if (current->children[0]->attr.val<SplValSpec>().type->exp_type ==
                SplExpType::SPL_EXP_STRUCT) {
                // FIXME: remove this?
                // throw std::runtime_error("uncaught struct declaration");
            } else {
                // invalid statement, "int;" for example
                report_semantic_error(31, current);
                current->error_propagated = true;
                return;
            }
        } else if (current->children.size() == 3 &&
                   current->children[1]->attr.type ==
                       SplAstNodeType::SPL_FUNDEC) {
            // ExtDef -> Specifier FunDec CompSt
            uninstall_function();
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
            int ret = symbols.install_symbol(latest_struct);
            if (ret != SplSymbolTable::SPL_SYM_INSTALL_OK) {
                if (ret == SplSymbolTable::SPL_SYM_INSTALL_REDEF_STRUCT) {
                    report_semantic_error(15, current);
                } else if (ret ==
                           SplSymbolTable::SPL_SYM_INSTALL_TYPE_CONFLICT) {
                    report_semantic_error(34, current);
                } else {
                    throw std::runtime_error("bad installation");
                }
                current->error_propagated = true;
                return;
            }
            uninstall_struct();
            std::string &name =
                current->children[1]->attr.val<SplValId>().val_id;
            current->attr.value = std::make_unique<SplValStructSpec>(name);
        } else if (current->children.size() == 2) {
            // StructSpecifier -> STRUCT ID
            std::string &name =
                current->children[1]->attr.val<SplValId>().val_id;
            auto id = symbols.lookup(name);
            if (id.has_value()) {
                if (id.value()->sym_type != SPL_SYM_STRUCT) {
                    report_semantic_error(33, current);
                    current->error_propagated = true;
                    return;
                }
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
            std::shared_ptr<std::vector<int>> dims;
            if (value_prev.type->is_array()) {
                dims =
                    std::make_shared<std::vector<int>>(*value_prev.type->dims);
            } else {
                dims = std::make_shared<std::vector<int>>();
            }
            // this is the only place whera array extend size
            dims->push_back(std::get<int>(
                current->children[2]->attr.val<SplValValue>().value));
            if (value_prev.type->exp_type == SPL_EXP_STRUCT) {
                current->attr.value = std::make_unique<SplValVarDec>(
                    value_prev.name,
                    std::make_shared<SplExpExactType>(
                        SPL_EXP_STRUCT, value_prev.type->struct_name, dims));
            } else {
                current->attr.value = std::make_unique<SplValVarDec>(
                    value_prev.name,
                    std::make_shared<SplExpExactType>(value_prev.type->exp_type,
                                                      std::move(dims)));
            }
        } else {
            assert(false);
        }
        if (parent != nullptr &&
            parent->attr.type != SplAstNodeType::SPL_VARDEC) {
            // ExtDecList -> *VarDec*
            // ExtDecList -> *VarDec* COMMA ExtDecList
            // ParamDec -> Specifier *VarDec*
            // Dec -> *VarDec*
            // Dec -> *VarDec* ASSIGN Exp
            auto &v_vardec = current->attr.val<SplValVarDec>();
            auto symbol = std::make_shared<SplVariableSymbol>(v_vardec.name,
                                                              v_vardec.type);
            if (!on_struct_body) {
                // install variable symbol
                int ret = symbols.install_symbol(symbol);
                if (ret != SplSymbolTable::SPL_SYM_INSTALL_OK) {
                    if (ret == SplSymbolTable::SPL_SYM_INSTALL_REDEF_VAR) {
                        report_semantic_error(3, current);
                    } else if (ret ==
                               SplSymbolTable::SPL_SYM_INSTALL_TYPE_CONFLICT) {
                        report_semantic_error(34, current);
                    } else {
                        throw std::runtime_error("bad installation");
                    }
                    current->error_propagated = true;
                    return;
                }
            } else {
                // add struct member
                int ret = latest_struct->members.install_symbol(symbol);
                if (ret != SplSymbolTable::SPL_SYM_INSTALL_OK) {
                    if (ret == SplSymbolTable::SPL_SYM_INSTALL_REDEF_VAR) {
                        report_semantic_error(3, current);
                    } else if (ret ==
                               SplSymbolTable::SPL_SYM_INSTALL_TYPE_CONFLICT) {
                        report_semantic_error(34, current);
                    } else {
                        throw std::runtime_error("bad installation");
                    }
                    current->error_propagated = true;
                    broken_struct = true;
                    return;
                }
            }
        }
        break;
        // FunDec
    }
    case SplAstNodeType::SPL_FUNDEC: {
        // FunDec -> ID LP VarList RP
        // FunDec -> ID LP RP
        int ret = symbols.install_symbol(latest_function);
        if (ret != SplSymbolTable::SPL_SYM_INSTALL_OK) {
            if (ret == SplSymbolTable::SPL_SYM_INSTALL_REDEF_FUNC) {
                report_semantic_error(4, current);
            } else if (ret == SplSymbolTable::SPL_SYM_INSTALL_TYPE_CONFLICT) {
                report_semantic_error(34, current);
            } else {
                throw std::runtime_error("bad installation");
            }
            current->error_propagated = true;
            broken_function = true;
            return;
        }
        // uninstall_function();
        break;
    }
    case SplAstNodeType::SPL_PARAMDEC: {
        // ParamDec -> Specifier VarDec
        // install function param for funcition symbol
        auto it =
            symbols.lookup(current->children[1]->attr.val<SplValVarDec>().name);
        std::cout << it.value()->sym_type << std::endl;
        latest_function->params.push_back(it.value());
        // uninstall specifier
        uninstall_specifier();
        break;
    }
    case SplAstNodeType::SPL_STMT: {
        if (current->children.size() == 3 &&
            current->children[0]->attr.type == SplAstNodeType::SPL_RETURN) {
            // Stmt -> RETURN Exp SEMI
            auto &exp_type = current->children[1]->attr.val<SplValExp>().type;
            if (exp_type->exp_type != latest_function->return_type->exp_type) {
                report_semantic_error(8, current);
                current->error_propagated = true;
                return;
            }
        }
        break;
    }
    case SplAstNodeType::SPL_DEC: {
        if (current->children.size() == 3) {
            // Dec -> VarDec ASSIGN Exp
            auto &value_prev = current->children[0]->attr.val<SplValVarDec>();
            // TODO: initialize variable, do this in codegen phase
            auto &v_vardec = current->children[0]->attr.val<SplValVarDec>();
            auto &v_exp = current->children[2]->attr.val<SplValExp>();
            if (v_vardec.type->exp_type != v_exp.type->exp_type) {
                // gcc does not cancel the variable symbol
                report_semantic_error(5, current);
                current->error_propagated = true;
                return;
            }
        }
        break;
    }
    case SplAstNodeType::SPL_EXP: {
        if (current->children.size() == 1) {
            switch (current->children[0]->attr.type) {
            case SplAstNodeType::SPL_ID: {
                // Exp -> ID
                auto it = symbols.lookup(
                    current->children[0]->attr.val<SplValId>().val_id);
                if (it.has_value()) {
                    SplVariableSymbol &symbol =
                        static_cast<SplVariableSymbol &>(*(it.value()));
                    current->attr.value =
                        std::make_unique<SplValExp>(symbol.var_type, true);
                } else {
                    report_semantic_error(1, current);
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
                // access struct member
                auto &v_struct = current->children[0]->attr.val<SplValExp>();
                auto &v_id = current->children[2]->attr.val<SplValId>();
                if (v_struct.type->exp_type != SPL_EXP_STRUCT) {
                    report_semantic_error(13, current);
                    current->error_propagated = true;
                    return;
                }
                auto it_struct = symbols.lookup(v_struct.type->struct_name);
                if (!it_struct.has_value()) {
                    throw std::runtime_error("instance of undefined structure");
                }
                auto &sym_struct =
                    static_cast<SplStructSymbol &>(**it_struct);
                auto it_member = sym_struct.members.lookup(v_id.val_id);
                if (!it_member.has_value()) {
                    report_semantic_error(14, current);
                    current->error_propagated = true;
                    return;
                }
                auto &member =
                    static_cast<SplVariableSymbol &>(**it_member);
                current->attr.value =
                    std::make_unique<SplValExp>(member.var_type, true);
                break;
            }
            case SplAstNodeType::SPL_LP: {
                // Exp -> ID LP RP
                auto it = symbols.lookup(
                    current->children[0]->attr.val<SplValId>().val_id);
                if (!it.has_value()) {
                    report_semantic_error(2, current);
                    current->error_propagated = true;
                    return;
                }
                if (it.value()->sym_type != SplSymbolType::SPL_SYM_FUNC) {
                    report_semantic_error(11, current);
                    current->error_propagated = true;
                    return;
                }
                SplFunctionSymbol &symbol =
                    static_cast<SplFunctionSymbol &>(**it);
                if (symbol.params.size() != 0) {
                    report_semantic_error(9, current);
                    current->error_propagated = true;
                    return;
                }
                current->attr.value =
                    std::make_unique<SplValExp>(symbol.return_type, false);
                break;
            }
            }
        } else if (current->children.size() == 4) {
            switch (current->children[1]->attr.type) {
            case SplAstNodeType::SPL_LP: {
                // Exp -> ID LP Args RP
                auto it = symbols.lookup(
                    current->children[0]->attr.val<SplValId>().val_id);
                if (!it.has_value()) {
                    report_semantic_error(2, current);
                    current->error_propagated = true;
                    return;
                }
                if (it.value()->sym_type != SplSymbolType::SPL_SYM_FUNC) {
                    report_semantic_error(11, current);
                    current->error_propagated = true;
                    return;
                }
                SplFunctionSymbol &symbol =
                    static_cast<SplFunctionSymbol &>(**it);
                auto &v_args = current->children[2]->attr.val<SplValArgs>();
                auto &arg_types = v_args.arg_types;
                if (arg_types.size() != symbol.params.size()) {
                    report_semantic_error(9, current);
                    current->error_propagated = true;
                    return;
                }
                for (size_t i = 0; i < arg_types.size(); ++i) {
                    if (*arg_types[arg_types.size() - 1 - i] !=
                        *static_cast<SplVariableSymbol &>(
                             *symbol.params[i])
                             .var_type) {
                        report_semantic_error(9, current);
                        current->error_propagated = true;
                        return;
                    }
                }
                current->attr.value =
                    std::make_unique<SplValExp>(symbol.return_type, false);
                break;
            }
            case SplAstNodeType::SPL_LB: {
                // Exp -> Exp LB Exp RB
                // array access
                auto &v_exp_arr = current->children[0]->attr.val<SplValExp>();
                auto &v_exp_idx = current->children[2]->attr.val<SplValExp>();

                if (!v_exp_arr.type->is_array()) {
                    report_semantic_error(10, current);
                    current->error_propagated = true;
                    return;
                }
                if (v_exp_idx.type->exp_type != SPL_EXP_INT) {
                    report_semantic_error(12, current);
                    current->error_propagated = true;
                    return;
                }
                if (!v_exp_arr.is_lvalue) {
                    throw std::runtime_error("array access on rvalue");
                }
                current->attr.value = std::make_unique<SplValExp>(
                    v_exp_arr.type, v_exp_arr.is_lvalue);
                v_exp_arr.type->step_array_idx();
                break;
            }
            }
        }
        break;
    }
    case SplAstNodeType::SPL_ARGS: {
        if (current->children.size() == 3) {
            // Args -> Exp COMMA Args
            auto &v_exp = current->children[0]->attr.val<SplValExp>();
            auto &v_args = current->children[2]->attr.val<SplValArgs>();
            current->attr.value = std::make_unique<SplValArgs>(
                std::vector<std::shared_ptr<SplExpExactType>>(
                    v_args.arg_types));
            current->attr.val<SplValArgs>().arg_types.push_back(v_exp.type);
        } else {
            // Args -> Exp
            auto &v_exp = current->children[0]->attr.val<SplValExp>();
            current->attr.value = std::make_unique<SplValArgs>(
                std::vector<std::shared_ptr<SplExpExactType>>());
            current->attr.val<SplValArgs>().arg_types.push_back(v_exp.type);
        }
        break;
    }
    case SplAstNodeType::SPL_ID: {
        if (parent != nullptr &&
            parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
            // FunDec -> *ID* LP VarList RP
            // FunDec -> *ID* LP RP
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
    case SplAstNodeType::SPL_LC: {
        if (parent != nullptr &&
            parent->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER) {
            // StructSpecifier -> STRUCT ID *LC* DefList RC
            on_struct_body = true;
        } else {
            symbols.forward();
        }
        break;
    }
    case SplAstNodeType::SPL_RC: {
        if (parent != nullptr &&
            parent->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER) {
            // StructSpecifier -> STRUCT ID *LC* DefList RC
            on_struct_body = false;
        } else {
            symbols.back();
        }
        break;
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