#include "spl-semantic-analyzer-module.cpp"
#include "spl-ast.hpp"
#include "spl-enum.hpp"
#include "spl-ir.hpp"
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
        return SplIrOperand::OperandType::R_VALUE_CONSTANT;
    } else if (name[0] == 'v') {
        return SplIrOperand::OperandType::L_VALUE_VARIABLE;
    } else if (name[0] == 't') {
        return SplIrOperand::OperandType::L_VALUE_TEMPORARY;
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
    now->ir.emplace_back(std::make_shared<SplIrFunctionInstruction>(
        SplIrOperand(SplIrOperand::FUNCTION, func_symbol->name)));
    for (auto &param : func_symbol->params) {
        spl_var_name_2_ir_var_name[param->name] = var_counter.next();
        now->ir.emplace_back(std::make_shared<SplIrParamInstruction>(
            SplIrOperand(SplIrOperand::L_VALUE_VARIABLE,
                         spl_var_name_2_ir_var_name[param->name])));
    }
}

std::string deref(SplAstNode *now,
                  std::list<std::shared_ptr<SplIrInstruction>> &insert_pos) {
    auto &ir_var = now->attr.val<SplValExp>().ir_var;
    if (!ir_var.is_addr)
        return ir_var.var;
    std::string tmp = tmp_counter.next();
    insert_pos.emplace_back(std::make_shared<SplIrAssignDerefSrcInstruction>(
        SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, tmp),
        SplIrOperand(ir_name_2_operand_type(ir_var.var), ir_var.var)));
    return tmp;
}

std::string deref(SplAstNode *now) { return deref(now, now->ir); }

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
    std::list<std::shared_ptr<SplIrInstruction>> args_ir_tmp;
    {
        auto args = now->children[2];
        while (args->children.size() == 3) {
            std::string ir_var = deref(args->children[0], now->ir);
            args_ir_tmp.emplace_front(std::make_shared<SplIrArgInstruction>(
                SplIrOperand(ir_name_2_operand_type(ir_var), ir_var)));
            args = args->children[2];
        }
        std::string ir_var = deref(args->children[0], now->ir);
        args_ir_tmp.emplace_front(std::make_shared<SplIrArgInstruction>(
            SplIrOperand(ir_name_2_operand_type(ir_var), ir_var)));
    }

    now->ir.splice(now->ir.end(), args_ir_tmp);
}

void handle_exp(SplAstNode *now) {
    traverse_exp(now);
    collect_ir_by_postorder(now);
}

void traverse_exp(SplAstNode *now) {
    // Caution: deref before use exp
    // Caution: deref before use exp
    // Caution: deref before use exp
    if (now->attr.type == SplAstNodeType::SPL_AND ||
        now->attr.type == SplAstNodeType::SPL_OR) {

        auto label = std::make_shared<SplIrLabelInstruction>(
            SplIrOperand(SplIrOperand::LABEL, label_counter.next()));
        // TODO: check correctness
        now->parent->children[2]->ir.emplace_back(label);
        now->parent->attr.val<SplValExp>().ir_var.label = label;
        return;
    } else if (now->attr.type != SplAstNodeType::SPL_EXP) {
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
            now->attr.val<SplValExp>().ir_var = {
                spl_var_name_2_ir_var_name[name]};
            break;
        }
        case SplAstNodeType::SPL_INT: {
            // Exp -> INT
            int value =
                std::get<int>(now->children[0]->attr.val<SplValValue>().value);
            now->attr.val<SplValExp>().ir_var = {"#" + std::to_string(value)};
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
            auto &ir_var = now->attr.val<SplValExp>().ir_var;
            ir_var = std::move(now->children[1]->attr.val<SplValExp>().ir_var);
            std::swap(ir_var.falselist, ir_var.truelist);
            break;
        }
        case SplAstNodeType::SPL_MINUS: {
            // Exp -> MINUS Exp
            std::string ir_var = tmp_counter.next();
            std::string child_ir_var = deref(now->children[1]);
            now->attr.val<SplValExp>().ir_var = {ir_var};
            now->ir.emplace_back(std::make_shared<SplIrAssignMinusInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(SplIrOperand::R_VALUE_CONSTANT, "#0"),
                SplIrOperand(ir_name_2_operand_type(child_ir_var),
                             child_ir_var)));
            break;
        }
        }
    } else if (now->children.size() == 3) {
        switch (now->children[1]->attr.type) {
        case SplAstNodeType::SPL_ASSIGN: {
            // Exp -> Exp ASSIGN Exp
            auto &dst_ir_var = now->children[0]->attr.val<SplValExp>().ir_var;
            auto &src_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
            now->attr.val<SplValExp>().ir_var = dst_ir_var;

            SplIrOperand dst_operand(ir_name_2_operand_type(dst_ir_var.var),
                                     dst_ir_var.var);

            if (!dst_ir_var.is_addr &&
                src_ir_var.is_addr) { // var = addr, optimization
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignDerefSrcInstruction>(
                        std::move(dst_operand),
                        SplIrOperand(ir_name_2_operand_type(src_ir_var.var),
                                     src_ir_var.var)));
                break;
            }

            std::string src = deref(now->children[2]);
            SplIrOperand src_operand(ir_name_2_operand_type(src), src);
            if (dst_ir_var.is_addr) { // addr = var
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignDerefDstInstruction>(
                        std::move(dst_operand), std::move(src_operand)));
            } else { // var = var
                now->ir.emplace_back(std::make_shared<SplIrAssignInstruction>(
                    std::move(dst_operand), std::move(src_operand)));
            }
            break;
        }
        case SplAstNodeType::SPL_AND: {
            // Exp -> Exp AND Exp
            auto &ir_var = now->attr.val<SplValExp>().ir_var;
            auto &lhs_ir_var = now->children[0]->attr.val<SplValExp>().ir_var;
            auto &rhs_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;

            // backpatch
            for (auto &ins : lhs_ir_var.truelist) {
                ins->patch(ir_var.label->label);
            }

            ir_var.truelist = std::move(rhs_ir_var.truelist);
            ir_var.falselist.splice_after(ir_var.falselist.before_begin(),
                                          lhs_ir_var.falselist);
            ir_var.falselist.splice_after(ir_var.falselist.before_begin(),
                                          rhs_ir_var.falselist);
            break;
        }
        case SplAstNodeType::SPL_OR: {
            // Exp -> Exp OR Exp
            auto &ir_var = now->attr.val<SplValExp>().ir_var;
            auto &lhs_ir_var = now->children[0]->attr.val<SplValExp>().ir_var;
            auto &rhs_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;

            // backpatch
            for (auto &ins : lhs_ir_var.falselist) {
                ins->patch(ir_var.label->label);
            }

            ir_var.falselist = std::move(rhs_ir_var.falselist);
            ir_var.truelist.splice_after(ir_var.truelist.before_begin(),
                                         lhs_ir_var.truelist);
            ir_var.truelist.splice_after(ir_var.truelist.before_begin(),
                                         rhs_ir_var.truelist);
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
            std::string lhs_ir_var = deref(now->children[0]),
                        rhs_ir_var = deref(now->children[2]);
            auto if_ins = std::make_shared<SplIrIfGotoInstruction>(
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var),
                now->children[1]->attr.type);
            auto goto_ins = std::make_shared<SplIrGotoInstruction>();
            now->ir.emplace_back(if_ins);
            now->ir.emplace_back(goto_ins);

            auto &ir_var = now->attr.val<SplValExp>().ir_var;
            ir_var.truelist.emplace_front(if_ins);
            ir_var.falselist.emplace_front(goto_ins);
            break;
        }
        case SplAstNodeType::SPL_PLUS: {
            // Exp -> Exp PLUS Exp
            std::string ir_var = tmp_counter.next(),
                        lhs_ir_var = deref(now->children[0]),
                        rhs_ir_var = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {ir_var};
            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_MINUS: {
            // Exp -> Exp MINUS Exp
            std::string ir_var = tmp_counter.next(),
                        lhs_ir_var = deref(now->children[0]),
                        rhs_ir_var = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {ir_var};
            now->ir.emplace_back(std::make_shared<SplIrAssignMinusInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_MUL: {
            // Exp -> Exp MUL Exp
            std::string ir_var = tmp_counter.next(),
                        lhs_ir_var = deref(now->children[0]),
                        rhs_ir_var = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {ir_var};
            now->ir.emplace_back(std::make_shared<SplIrAssignMulInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_DIV: {
            // Exp -> Exp DIV Exp
            std::string ir_var = tmp_counter.next(),
                        lhs_ir_var = deref(now->children[0]),
                        rhs_ir_var = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {ir_var};
            now->ir.emplace_back(std::make_shared<SplIrAssignDivInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(ir_name_2_operand_type(lhs_ir_var), lhs_ir_var),
                SplIrOperand(ir_name_2_operand_type(rhs_ir_var), rhs_ir_var)));
            break;
        }
        case SplAstNodeType::SPL_EXP: {
            // Exp -> LP Exp RP
            now->attr.val<SplValExp>().ir_var =
                std::move(now->children[1]->attr.val<SplValExp>().ir_var);
            break;
        }
        case SplAstNodeType::SPL_DOT: {
            auto struct_sym = (*symbols.lookup<SplStructSymbol>(
                now->children[0]->attr.val<SplValExp>().type->struct_name));
            auto offset = struct_sym->get_offset(
                now->children[2]->attr.val<SplValId>().val_id);

            std::string &base_addr_var =
                now->children[0]->attr.val<SplValExp>().ir_var.var;

            if (offset == 0) {
                now->attr.val<SplValExp>().ir_var = {base_addr_var, true};
                break;
            }

            std::string ir_var = tmp_counter.next();
            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, base_addr_var),
                SplIrOperand(SplIrOperand::R_VALUE_CONSTANT,
                             "#" + std::to_string(offset))));
            now->attr.val<SplValExp>().ir_var = {ir_var, true};
            break;
        }
        case SplAstNodeType::SPL_LP: {
            // Exp -> ID LP RP
            // call without args
            std::string ir_var = tmp_counter.next();
            now->attr.val<SplValExp>().ir_var = {ir_var};
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "read") {
                now->ir.emplace_back(std::make_shared<SplIrReadInstruction>(
                    SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var)));
            } else {
                now->ir.emplace_back(std::make_shared<SplIrCallInstruction>(
                    SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
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
            std::string ir_var;
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "write") {
                auto arg_ins = std::static_pointer_cast<SplIrArgInstruction>(
                    now->ir.back());
                ir_var = arg_ins->arg.repr;
                now->ir.pop_back();
                now->ir.emplace_back(std::make_shared<SplIrWriteInstruction>(
                    SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var)));
            } else {
                ir_var = tmp_counter.next();
                now->ir.emplace_back(std::make_shared<SplIrCallInstruction>(
                    SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var),
                    SplIrOperand(SplIrOperand::FUNCTION, func_name)));
            }
            now->attr.val<SplValExp>().ir_var = {ir_var};
            break;
        }
        case SplAstNodeType::SPL_LB: {
            // Exp -> Exp LB Exp RB
            auto arr_size = now->children[0]
                                ->attr.val<SplValExp>()
                                .type->get_arr_size_for_current_index();
            std::string &base_addr_var =
                now->children[0]->attr.val<SplValExp>().ir_var.var;
            std::string accesser_var = deref(now->children[2]);

            if (accesser_var == "#0") {
                now->attr.val<SplValExp>().ir_var = {base_addr_var, true};
                break;
            }
            std::string ir_var_mul = tmp_counter.next();
            std::string ir_var_add = tmp_counter.next();
            now->ir.emplace_back(std::make_shared<SplIrAssignMulInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var_mul),
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, accesser_var),
                SplIrOperand(SplIrOperand::R_VALUE_CONSTANT,
                             "#" + std::to_string(arr_size))));

            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var_add),
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, base_addr_var),
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, ir_var_mul)));
            now->attr.val<SplValExp>().ir_var = {ir_var_add, true};
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
    std::list<std::shared_ptr<SplIrInstruction>> tmp;
    tmp.splice(tmp.end(), root->ir);
    collect_ir_by_postorder_recurse(now, root);
    root->ir.splice(root->ir.end(), tmp);
}

void collect_ir_by_postorder_recurse(SplAstNode *now, SplAstNode *root) {
    for (auto &child : now->children) {
        collect_ir_by_postorder_recurse(child, root);
        if (child->ir.size() > 0) {
            root->ir.splice(root->ir.end(), child->ir);
        }
    }
}

std::string &handle_vardec(SplAstNode *now) {
    SplAstNode *tmp = now;              // VARDEC
    while (tmp->children.size() == 4) { // is array
        tmp = tmp->children[0];
    }
    auto &id = tmp->children[0];
    auto &name = id->attr.val<SplValId>().val_id;
    spl_var_name_2_ir_var_name[name] = var_counter.next();
    auto symbol =
        std::static_pointer_cast<SplVariableSymbol>(*(symbols.lookup(name)));
    if (symbol->var_type->is_array_or_struct()) {
        std::string ir_var_tmp = tmp_counter.next();
        now->ir.emplace_back(std::make_shared<SplIrDecInstruction>(
            SplIrOperand(SplIrOperand::L_VALUE_VARIABLE, ir_var_tmp),
            symbol->var_type->size));

        now->ir.emplace_back(std::make_shared<SplIrAssignAddressInstruction>(
            SplIrOperand(SplIrOperand::L_VALUE_VARIABLE,
                         spl_var_name_2_ir_var_name[name]),
            SplIrOperand(SplIrOperand::L_VALUE_VARIABLE, ir_var_tmp)));
    }
    return name;
}

void handle_dec(SplAstNode *now) {
    auto &name = handle_vardec(now->children[0]); // vardec
    if (now->children.size() == 3) {
        // VarDec ASSIGN Exp
        handle_exp(now->children[2]);
        collect_ir_by_postorder(now);
        auto &exp_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        if (exp_ir_var.is_addr) { // val = addr
            now->ir.emplace_back(
                std::make_shared<SplIrAssignDerefSrcInstruction>(
                    SplIrOperand(SplIrOperand::L_VALUE_VARIABLE,
                                 spl_var_name_2_ir_var_name[name]),
                    SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY,
                                 exp_ir_var.var)));
        } else { // val = val
            now->ir.emplace_back(std::make_shared<SplIrAssignInstruction>(
                SplIrOperand(SplIrOperand::L_VALUE_VARIABLE,
                             spl_var_name_2_ir_var_name[name]),
                SplIrOperand(SplIrOperand::L_VALUE_TEMPORARY, exp_ir_var.var)));
        }
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
        std::string ret = deref(now->children[1]);
        now->ir.emplace_back(std::make_shared<SplIrReturnInstruction>(
            SplIrOperand(ir_name_2_operand_type(ret), ret)));
        return;
    }
    case SPL_IF: {
        // IF LP Exp RP Stmt (ELSE Stmt)
        handle_exp(now->children[2]);
        auto label = SplIrOperand(SplIrOperand::LABEL, label_counter.next());
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(SplIrOperand{label}));
        auto &ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        for (auto &v : ir_var.truelist) {
            v->patch(label);
        }

        label = SplIrOperand(SplIrOperand::LABEL, label_counter.next());
        handle_stmt(now->children[4]);
        collect_ir_by_postorder(now->children[4]);

        if (now->children.size() == 7) { // ELSE Stmt
            handle_stmt(now->children[6]);
            auto end_else_label_operand =
                SplIrOperand(SplIrOperand::LABEL, label_counter.next());
            collect_ir_by_postorder(now->children[6]);
            now->children[6]->ir.emplace_back(
                std::make_shared<SplIrLabelInstruction>(
                    SplIrOperand{end_else_label_operand}));

            now->children[4]->ir.emplace_back(
                std::make_shared<SplIrGotoInstruction>(
                    SplIrOperand{end_else_label_operand}));
        }
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(SplIrOperand{label}));
        for (auto &v : ir_var.falselist) {
            v->patch(label);
        }

        return;
    }
    case SPL_WHILE: {
        // WHILE LP Exp RP Stmt
        auto exp_begin_label =
            SplIrOperand(SplIrOperand::LABEL, label_counter.next());
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(
                SplIrOperand{exp_begin_label}));

        handle_exp(now->children[2]);
        auto label = SplIrOperand(SplIrOperand::LABEL, label_counter.next());
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(SplIrOperand{label}));
        auto &ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        for (auto &v : ir_var.truelist) {
            v->patch(label);
        }

        label = SplIrOperand(SplIrOperand::LABEL, label_counter.next());
        handle_stmt(now->children[4]);
        collect_ir_by_postorder(now->children[4]);
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrGotoInstruction>(
                SplIrOperand{exp_begin_label}));
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(SplIrOperand{label}));
        for (auto &v : ir_var.falselist) {
            v->patch(label);
        }

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
    case SPL_RC:
    case SPL_COMMA: {
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
    case SPL_EXTDEF:
    case SPL_EXTDECLIST: {
        break;
    }
    case SPL_SPECIFIER:
    case SPL_STRUCTSPECIFIER:
    case SPL_SEMI: {
        return;
    }
    case SPL_VARDEC: {
        handle_vardec(now);
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
