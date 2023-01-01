#include "spl-semantic-analyzer-module.cpp"
#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <unordered_map>

extern SplAstNode *prog;

static std::stringstream out;

static std::unordered_map<std::string, std::string> spl_var_name_2_ir_var_name;
static SplIrAutoIncrementHelper var_counter("v");

static SplIrAutoIncrementHelper tmp_counter("t");

static SplIrAutoIncrementHelper label_counter("label");

SplIrOperand::OperandType ir_name_2_operand_type(const std::string &name) {
    if (name[0] == '#') {
        return SplIrOperand::OperandType::L_VALUE_CONSTANT;
    } else if (name[0] == 'v') {
        return SplIrOperand::OperandType::R_VALUE_VARIABLE;
    } else if (name[0] == 't') {
        return SplIrOperand::OperandType::R_VALUE_TEMPORARY;
    }
    throw std::runtime_error("unknown operand type");
}

void generate_ir();
void traverse_ir(SplAstNode *now);

void collect_ir_by_postorder(SplAstNode *root);
void collect_ir_by_postorder(SplAstNode *now, SplAstNode *root);
void collect_ir_by_postorder_recurse(SplAstNode *now, SplAstNode *root);

void handle_exp(SplAstNode *now);
void traverse_exp(SplAstNode *now);

void handle_fundec(SplAstNode *now);

void handle_args(SplAstNode *now);

void handle_dec(SplAstNode *now);

void generate_ir() {
    traverse_ir(prog);
    collect_ir_by_postorder(prog);
    for (auto &ir : prog->ir) {
        ir->print(out);
    }
    std::cout << "----------------------" << std::endl;
    std::cout << out.str() << std::endl;
}

void handle_fundec(SplAstNode *now) {
    auto &func_name = now->children[0]->attr.val<SplValId>().val_id;
    auto func_symbol = std::static_pointer_cast<SplFunctionSymbol>(
        *(symbols.lookup(func_name)));
    now->ir.emplace_back(std::make_unique<SplIrFunctionInstruction>(
        SplIrOperand(SplIrOperand::FUNCTION, func_symbol->name)));
    for (auto &param : func_symbol->params) {
        spl_var_name_2_ir_var_name[param->name] = var_counter.next();
        now->ir.emplace_back(std::make_unique<SplIrParamInstruction>(
            SplIrOperand(SplIrOperand::R_VALUE_VARIABLE,
                         spl_var_name_2_ir_var_name[param->name])));
    }
}

void handle_args(SplAstNode *now) {
    // Exp -> ID LP Args RP
    // Note that the passed in node is Exp
    {
        auto args = now->children[2];
        while (args->children.size() == 3) {
            // Args -> Exp COMMA Args
            auto &exp = args->children[0];
            handle_exp(exp);
            args = args->children[2];
        }
        auto &exp = args->children[0];
        handle_exp(exp);
    }

    collect_ir_by_postorder(now);

    // set args
    {
        auto args = now->children[2];
        while (args->children.size() == 3) {
            auto &exp = args->children[0];
            auto &ir_var = exp->attr.val<SplValExp>().ir_var;
            now->ir.emplace_back(std::make_unique<SplIrArgInstruction>(
                SplIrOperand(ir_name_2_operand_type(ir_var), ir_var)));
            args = args->children[2];
        }
        auto &exp = args->children[0];
        auto &ir_var = exp->attr.val<SplValExp>().ir_var;
        now->ir.emplace_back(std::make_unique<SplIrArgInstruction>(
            SplIrOperand(ir_name_2_operand_type(ir_var), ir_var)));
    }

    // reverse ir
    std::reverse(now->ir.begin(), now->ir.end());
}

void handle_exp(SplAstNode *now) {
    traverse_exp(now);
    collect_ir_by_postorder(now);
}

void traverse_exp(SplAstNode *now) {
    if (now->attr.type != SplAstNodeType::SPL_EXP) {
        return;
    }

    for (auto &child : now->children) {
        traverse_exp(child);
    }

    if (now->children.size() == 1) {
        switch (now->children[0]->attr.type) {
        case SplAstNodeType::SPL_ID: {
            // Exp -> ID
            auto &name = now->children[0]->attr.val<SplValId>().val_id;
            now->attr.val<SplValExp>().ir_var =
                spl_var_name_2_ir_var_name[name];
            break;
        }
        case SplAstNodeType::SPL_INT: {
            // Exp -> INT
            int value =
                std::get<int>(now->children[0]->attr.val<SplValValue>().value);
            now->attr.val<SplValExp>().ir_var = "#" + std::to_string(value);
            break;
        }
        case SplAstNodeType::SPL_FLOAT: // Exp -> FLOAT
        case SplAstNodeType::SPL_CHAR:  // Exp -> CHAR
        default:
            throw std::runtime_error("This should not happen");
        }
    } else if (now->children.size() == 2) {
        switch (now->children[0]->attr.type) {
        case SplAstNodeType::SPL_NOT: {
            // Exp -> NOT Exp
            // TODO: boolean operation
            break;
        }
        case SplAstNodeType::SPL_MINUS: {
            // Exp -> MINUS Exp
            std::string ir_var = tmp_counter.next(),
                        &child_ir_var =
                            now->children[1]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignMinusInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(SplIrOperand::L_VALUE_CONSTANT, "#0"),
                SplIrOperand(ir_name_2_operand_type(child_ir_var),
                             child_ir_var)));
            break;
        }
        }
    } else if (now->children.size() == 3) {
        switch (now->children[1]->attr.type) {
        case SplAstNodeType::SPL_ASSIGN: {
            // Exp -> Exp ASSIGN Exp
            std::string &dst_ir_var =
                            now->children[0]->attr.val<SplValExp>().ir_var,
                        &src_ir_var =
                            now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = dst_ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignInstruction>(
                SplIrOperand(ir_name_2_operand_type(dst_ir_var), dst_ir_var),
                SplIrOperand(ir_name_2_operand_type(src_ir_var), src_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_AND: {
            // Exp -> Exp AND Exp
            // TODO: boolean operation
            break;
        }
        case SplAstNodeType::SPL_OR: {
            // Exp -> Exp OR Exp
            // TODO: boolean operation
            break;
        }
        case SplAstNodeType::SPL_LT:
        case SplAstNodeType::SPL_LE:
        case SplAstNodeType::SPL_GT:
        case SplAstNodeType::SPL_GE:
        case SplAstNodeType::SPL_EQ:
        case SplAstNodeType::SPL_NE: {
            // Exp -> Exp LT Exp
            // Exp -> Exp LE Exp
            // Exp -> Exp GT Exp
            // Exp -> Exp GE Exp
            // Exp -> Exp EQ Exp
            // Exp -> Exp NE Exp
            // TODO: IF GOTO instruction
            break;
        }
        case SplAstNodeType::SPL_PLUS: {
            // Exp -> Exp PLUS Exp
            std::string ir_var = tmp_counter.next(),
                        &lhs_ir_var =
                            now->children[0]->attr.val<SplValExp>().ir_var,
                        &rhs_ir_var =
                            now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignAddInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_MINUS: {
            // Exp -> Exp MINUS Exp
            std::string ir_var = tmp_counter.next(),
                        &lhs_ir_var =
                            now->children[0]->attr.val<SplValExp>().ir_var,
                        &rhs_ir_var =
                            now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignMinusInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_MUL: {
            // Exp -> Exp MUL Exp
            std::string ir_var = tmp_counter.next(),
                        &lhs_ir_var =
                            now->children[0]->attr.val<SplValExp>().ir_var,
                        &rhs_ir_var =
                            now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignMulInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_DIV: {
            // Exp -> Exp DIV Exp
            std::string ir_var = tmp_counter.next(),
                        &lhs_ir_var =
                            now->children[0]->attr.val<SplValExp>().ir_var,
                        &rhs_ir_var =
                            now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            now->ir.emplace_back(std::make_unique<SplIrAssignDivInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_EXP: {
            // Exp -> LP Exp RP
            std::string &ir_var =
                now->children[1]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = ir_var;
            break;
        }
        case SplAstNodeType::SPL_DOT: {
            // Exp -> Exp DOT ID
            // TODO: access struct member
            break;
        }
        case SplAstNodeType::SPL_LP: {
            // Exp -> ID LP RP
            // call without args
            std::string ir_var = tmp_counter.next();
            now->attr.val<SplValExp>().ir_var = ir_var;
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "read") {
                now->ir.emplace_back(std::make_unique<SplIrReadInstruction>(
                    SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var)));
            } else {
                now->ir.emplace_back(std::make_unique<SplIrCallInstruction>(
                    SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                    SplIrOperand(SplIrOperand::FUNCTION, func_name)));
            }
            break;
        }
        }
    } else if (now->children.size() == 4) {
        switch (now->children[1]->attr.type) {
        case SplAstNodeType::SPL_LP: {
            // Exp -> ID LP Args RP
            // call with args
            handle_args(now);
            std::string ir_var = tmp_counter.next();
            now->attr.val<SplValExp>().ir_var = ir_var;
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "write") {
                auto &ins = now->ir.back();
                std::unique_ptr<SplIrArgInstruction> arg_ins(
                    static_cast<SplIrArgInstruction *>(ins.release()));
                ir_var = arg_ins->arg.repr;
                now->ir.pop_back();
                now->ir.emplace_back(std::make_unique<SplIrWriteInstruction>(
                    SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var)));
            } else {
                now->ir.emplace_back(std::make_unique<SplIrCallInstruction>(
                    SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                    SplIrOperand(SplIrOperand::FUNCTION, func_name)));
            }
            break;
        }
        case SplAstNodeType::SPL_LB: {
            // Exp -> Exp LB Exp RB
            // TODO: array access
            break;
        }
        }
    }
}

void collect_ir_by_postorder(SplAstNode *root) {
    collect_ir_by_postorder(root, root);
}

void collect_ir_by_postorder(SplAstNode *now, SplAstNode *root) {
    // collect ir to root node by post order
    std::list<std::unique_ptr<SplIrInstruction>> tmp;
    tmp.insert(tmp.end(), std::make_move_iterator(root->ir.begin()),
               std::make_move_iterator(root->ir.end()));
    root->ir.clear();
    collect_ir_by_postorder_recurse(now, root);
    root->ir.insert(root->ir.end(), std::make_move_iterator(tmp.begin()),
                    std::make_move_iterator(tmp.end()));
    tmp.clear();
}

void collect_ir_by_postorder_recurse(SplAstNode *now, SplAstNode *root) {
    for (auto &child : now->children) {
        collect_ir_by_postorder_recurse(child, root);
        if (child->ir.size() > 0) {
            root->ir.insert(root->ir.end(),
                            std::make_move_iterator(child->ir.begin()),
                            std::make_move_iterator(child->ir.end()));
            child->ir.clear();
        }
    }
}

void handle_dec(SplAstNode *now) {
    SplAstNode *tmp = now->children[0]; // VARDEC
    while (tmp->children.size() == 4) { // is array
        tmp = tmp->children[0];
    }
    auto &id = tmp->children[0];
    auto &name = id->attr.val<SplValId>().val_id;
    spl_var_name_2_ir_var_name[name] = var_counter.next();
    auto symbol =
        std::static_pointer_cast<SplVariableSymbol>(*(symbols.lookup(name)));
    if (symbol->var_type->is_array_or_struct()) {
        now->ir.emplace_back(std::make_unique<SplIrDecInstruction>(
            SplIrOperand(SplIrOperand::R_VALUE_VARIABLE,
                         spl_var_name_2_ir_var_name[name]),
            symbol->var_type->size));
    }
    if (now->children.size() == 3) {
        // VarDec ASSIGN Exp
        handle_exp(now->children[2]);
        collect_ir_by_postorder(now);
        now->ir.emplace_back(std::make_unique<SplIrAssignInstruction>(
            SplIrOperand(SplIrOperand::R_VALUE_VARIABLE,
                         spl_var_name_2_ir_var_name[name]),
            SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY,
                         now->children[2]->attr.val<SplValExp>().ir_var)));
        // FIXME: do optimization here
    }
}

void traverse_comp(SplAstNode *now);

void handle_stmt(SplAstNode *now) {
    switch (now->children[0]->attr.type) {
    case SPL_EXP: {
        handle_exp(now->children[0]);
        return;
    }
    case SPL_COMPST: {
        traverse_comp(now->children[0]);
        return;
    }
    case SPL_RETURN: {
        handle_exp(now->children[1]);
        out << "RETURN ret" << std::endl; // TODO: May optimize in handle_exp
        return;
    }
    case SPL_IF: {
        handle_exp(now->children[2]);
        return;
    }
    case SPL_WHILE: {
        handle_exp(now->children[2]);
        return;
    }
    }
}

void traverse_comp(SplAstNode *now) {
    switch (now->attr.type) {
    case SPL_COMPST:
    case SPL_DEFLIST:
    case SPL_DEF:
    case SPL_DECLIST:
    case SPL_STMTLIST: {
        break;
    }
    case SPL_SPECIFIER:
    case SPL_SEMI:
    case SPL_LC:
    case SPL_RC: {
        return;
    }
    case SPL_STMT: {
        handle_stmt(now);
        return;
    }
    case SPL_DEC: {
        handle_dec(now);
        return;
    }
    default: {
        std::cout << "comp: " << now->name << std::endl;
    }
    }
    for (auto &c : now->children) {
        traverse_comp(c);
    }
}

void traverse_ir(SplAstNode *now) {
    // Handle everything outside COMPST.

    auto parent = now->parent;

    switch (now->attr.type) {
    case SPL_PROGRAM:
    case SPL_EXTDEFLIST:
    case SPL_EXTDEF: {
        break;
    }
    case SPL_SPECIFIER:
    case SPL_STRUCTSPECIFIER:
    case SPL_SEMI:
    case SPL_EXTDECLIST: {
        return;
    }
    case SPL_FUNDEC: {
        handle_fundec(now);
        return;
    }
    case SPL_COMPST: {
        traverse_comp(now);
        return;
    }
    default: {
        std::cout << "out: " << now->name << std::endl;
    }
    }

    for (auto &c : now->children) {
        traverse_ir(c);
    }
}
