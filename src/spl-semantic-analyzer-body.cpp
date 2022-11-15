#include "spl-parser-module.cpp"
#include "spl-ast.hpp"
#include "semantic-error.hpp"

#include <string>

extern SplAstNode *prog;
extern bool hasError;

extern VariableSymbolTable symbols_var;
extern StructSymbolTable symbols_struct;
extern FunctionSymbolTable symbols_func;

inline SplVal*& getSplNodeValuePtr(SplAstNode *node) {
    SplVal *&ptr = node->attr.value_p;
    if (ptr == nullptr) {
        ptr = new SplVal();
    }
    return ptr;
}

SplExpExactType *latest_specifier_exact_type = nullptr;
void uninstall_specifier() {
    if (latest_specifier_exact_type != nullptr) {
        delete latest_specifier_exact_type;
        latest_specifier_exact_type = nullptr;
    }
}
void install_specifier(SplExpExactType* type) {
    uninstall_specifier();
    latest_specifier_exact_type = new SplExpExactType();
    SplExpExactType::dup(type, latest_specifier_exact_type);
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
    switch (current->attr.type) {  // rules at end of production
        case SplAstNodeType::SPL_SPECIFIER : {
            SplAstNode *node = current->children[0];
            if (node->attr.type == SplAstNodeType::SPL_TYPE) {
                // Specifier -> TYPE
                std::string type_str(getSplNodeValuePtr(node)->val_type);
                SplExpExactType *&type = getSplNodeValuePtr(current)->val_specifier.type;
                type = new SplExpExactType();
                if (type_str == "int") {
                    type->exp_type = SPL_EXP_INT;
                } else if (type_str == "float") {
                    type->exp_type = SPL_EXP_FLOAT;
                } else if (type_str == "char") {
                    type->exp_type = SPL_EXP_CHAR;
                } else {
                    assert(false);
                }
                install_specifier(type);
            } else if (node->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER) {
                // Specifier -> StructSpecifier
                // TODO: handle struct specifier
            } else {
                assert(false);
            }
            break;
        } case SplAstNodeType::SPL_STRUCTSPECIFIER : {
            if (current->children.size() == 5) {
                // StructSpecifier -> STRUCT ID LC DefList RC
                // TODO: install struct symbol
            } else if (current->children.size() == 2) {
                // StructSpecifier -> STRUCT ID
                std::string name(getSplNodeValuePtr(current->children[1])->val_id);
                if (symbols_struct.find(name) != symbols_struct.cend()) {
                // TODO: construct struct specifier
                } else {
                // TODO: declare struct / undeclared usage
                }
            } else {
                assert(false);
            }
            break;
        } case SplAstNodeType::SPL_VARDEC : {
            if (current->children.size() == 1) {
                // VarDec -> ID
                auto &value = getSplNodeValuePtr(current)->val_vardec;
                value.name = new std::string(getSplNodeValuePtr(current->children[0])->val_id);
                value.type = new SplExpExactType();
                SplExpExactType::dup(latest_specifier_exact_type, value.type);
            } else if (current->children.size() == 4) {
                // VarDec -> VarDec LB INT RB
                auto &value = getSplNodeValuePtr(current)->val_vardec;
                auto &value_prev = getSplNodeValuePtr(current->children[0])->val_vardec;
                value.name = new std::string(*value_prev.name);
                value.type = new SplExpExactType();
                value.type->exp_type = value_prev.type->exp_type;
                value.type->is_array = true;
                std::vector<int> *&dims = value.type->dimensions;
                dims = new std::vector<int>();
                if (value_prev.type->is_array) {
                    std::vector<int> *&dims_prev = value_prev.type->dimensions;
                    dims->insert(dims->begin(), dims_prev->begin(), dims_prev->end());
                }
                dims->push_back(getSplNodeValuePtr(current->children[2])->val_int.value);
            } else {
                assert(false);
            }
            if (parent != nullptr && parent->attr.type != SplAstNodeType::SPL_VARDEC) {
                // ExtDecList -> *VarDec*
                // ExtDecList -> *VarDec* COMMA ExtDecList
                // ParamDec -> Specifier *VarDec*
                // Dec -> *VarDec*
                // Dec -> *VarDec* ASSIGNOP Exp
                // install variable symbol
                auto &v_vardec = getSplNodeValuePtr(current)->val_vardec;
                SplVariableSymbol *symbol = new SplVariableSymbol(*v_vardec.name, v_vardec.type);
                VariableSymbolTable::install_symbol(symbols_var, symbol);
            }
            break;
        // FunDec
        } case SplAstNodeType::SPL_FUNDEC : {
            FunctionSymbolTable::install_symbol(symbols_func, latest_function);
            uninstall_function();
            break;
        case SplAstNodeType::SPL_PARAMDEC :
            // ParamDec -> Specifier VarDec
            // install function param for funcition symbol
            auto it = symbols_var.find(*getSplNodeValuePtr(current->children[1])->val_vardec.name);
            latest_function->params.push_back(it);
            // uninstall specifier
            uninstall_specifier();
            break;
        } case SplAstNodeType::SPL_DEC : {
            if (current->children.size() == 3) {
                // Dev -> VarDec ASSIGN Exp
                // TODO: initialize variable
                // TODO: check type compatibility during assignment
            }
            break;
        } case SplAstNodeType::SPL_ID : {
            if (parent != nullptr && parent->attr.type == SplAstNodeType::SPL_FUNDEC) {
                // FunDec -> ID LP VarList RP
                // FunDec -> ID LP RP
                install_function();
                latest_function->return_type = new SplExpExactType();
                SplExpExactType::dup(latest_specifier_exact_type, latest_function->return_type);
                latest_function->name = std::string(getSplNodeValuePtr(current)->val_id);
            }
            break;
        } case SplAstNodeType::SPL_SEMI : {
            // uninstall specifier
            uninstall_specifier();
            break;
        } case SplAstNodeType::SPL_EXP : {
            if (current->children.size() == 1) {
                switch (current->children[0]->attr.type) {
                    case SplAstNodeType::SPL_ID : {
                        // Exp -> ID
                        std::string id(getSplNodeValuePtr(current->children[0])->val_id);
                        auto it = symbols_var.find(id);
                        if (it != symbols_var.end()) {
                            auto &v_exp = getSplNodeValuePtr(current)->val_exp;
                            v_exp.type = new SplExpExactType();
                            SplExpExactType::dup(it->second->type, v_exp.type);
                            v_exp.is_lvalue = true;
                        } else {
                            // TODO: add error report
                            assert(false);
                        }
                        break;
                    } case SplAstNodeType::SPL_INT : {
                        // Exp -> INT
                        auto &v_exp = getSplNodeValuePtr(current)->val_exp;
                        v_exp.type = new SplExpExactType();
                        v_exp.type->exp_type = SPL_EXP_INT;
                        v_exp.is_lvalue = false;
                        break;
                    } case SplAstNodeType::SPL_FLOAT : {
                        // Exp -> FLOAT
                        auto &v_exp = getSplNodeValuePtr(current)->val_exp;
                        v_exp.type = new SplExpExactType();
                        v_exp.type->exp_type = SPL_EXP_FLOAT;
                        v_exp.is_lvalue = false;
                        break;
                    } case SplAstNodeType::SPL_CHAR : {
                        // Exp -> CHAR
                        auto &v_exp = getSplNodeValuePtr(current)->val_exp;
                        v_exp.type = new SplExpExactType();
                        v_exp.type->exp_type = SPL_EXP_CHAR;
                        v_exp.is_lvalue = false;
                        break;
                    }
                }
            }
        } default: {
            break;
        }
    }
    if (parent == nullptr) {
        return;
    }
    switch (parent->attr.type) {
        case SplAstNodeType::SPL_VARDEC :
            break;
        default:
            break;
    }
}