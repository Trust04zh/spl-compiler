#include "spl-ast.hpp"
#include "spl-enum.hpp"
#include "spl-ir.hpp"
#include "spl-semantic-analyzer-module.cpp"
#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <unordered_map>

extern SplAstNode *prog;

static std::stringstream out;

static std::unordered_map<std::string, std::string> spl_var_name_2_ir_var_name;
static SplIrModule ir_module;

void generate_ir();
void traverse_ir(SplAstNode *now);

void collect_ir_by_postorder(SplAstNode *root);
void collect_ir_by_postorder(SplAstNode *now, SplAstNode *root);
void collect_ir_by_postorder_recurse(SplAstNode *now, SplAstNode *root);

void handle_fundec(SplAstNode *now);
void handle_args(SplAstNode *now);
std::string &handle_vardec(SplAstNode *now);
void handle_dec(SplAstNode *now);

void traverse_exp(SplAstNode *now);
void traverse_comp(SplAstNode *now);
void handle_stmt(SplAstNode *now);

std::shared_ptr<SplIrOperand> deref(SplAstNode *now,
                                    SplIrInstructionList &insert_pos);
std::shared_ptr<SplIrOperand> deref(SplAstNode *now);

void opt_ir();

void generate_ir() {
    traverse_ir(prog);
    collect_ir_by_postorder(prog);
    ir_module.fill_ir(prog->ir);
    opt_ir();  // uncomment this line to enable optimization
    for (auto &ir : ir_module.ir) {
        ir->print(out);
    }
    std::cout << out.str() << std::endl;
}

std::shared_ptr<SplIrOperand> deref(SplAstNode *now,
                                    SplIrInstructionList &insert_pos) {
    auto &ir_var = now->attr.val<SplValExp>().ir_var;
    auto op_var = ir_module.get_operand_by_name(ir_var.var);
    if (!ir_var.is_addr)
        return op_var;
    auto op_tmp = ir_module.tmp_counter->next();
    insert_pos.emplace_back(
        std::make_shared<SplIrAssignDerefSrcInstruction>(op_tmp, op_var));
    return op_tmp;
}

std::shared_ptr<SplIrOperand> deref(SplAstNode *now) {
    return deref(now, now->ir);
}

void handle_fundec(SplAstNode *now) {
    auto &func_name = now->children[0]->attr.val<SplValId>().val_id;
    auto func_symbol = std::static_pointer_cast<SplFunctionSymbol>(
        *(symbols.lookup(func_name)));
    now->ir.emplace_back(std::make_shared<SplIrFunctionInstruction>(
        ir_module.get_or_make_function_operand_by_name(func_name)));
    for (auto &param : func_symbol->params) {
        auto op_param = ir_module.var_counter->next();
        spl_var_name_2_ir_var_name[param->name] = op_param->repr;
        now->ir.emplace_back(std::make_shared<SplIrParamInstruction>(op_param));
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
            traverse_exp(exp);
            args = args->children[2];
        }
        auto &exp = args->children[0];
        traverse_exp(exp);
    }

    // set args
    SplIrInstructionList args_ir_tmp;
    {
        auto args = now->children[2];
        while (args->children.size() == 3) {
            auto op_arg = deref(args->children[0], now->ir);
            args_ir_tmp.emplace_front(
                std::make_shared<SplIrArgInstruction>(op_arg));
            args = args->children[2];
        }
        auto op_arg = deref(args->children[0], now->ir);
        args_ir_tmp.emplace_front(
            std::make_shared<SplIrArgInstruction>(op_arg));
    }

    now->ir.splice(now->ir.end(), args_ir_tmp);
}

std::string &handle_vardec(SplAstNode *now) {
    SplAstNode *tmp = now;              // VARDEC
    while (tmp->children.size() == 4) { // is array
        tmp = tmp->children[0];
    }
    auto &id = tmp->children[0];
    auto &name = id->attr.val<SplValId>().val_id;
    auto op_var = ir_module.var_counter->next();
    spl_var_name_2_ir_var_name[name] = op_var->repr;
    auto symbol =
        std::static_pointer_cast<SplVariableSymbol>(*(symbols.lookup(name)));
    if (symbol->var_type->is_array_or_struct()) {
        auto op_tmp = ir_module.tmp_counter->next();
        now->ir.emplace_back(std::make_shared<SplIrDecInstruction>(
            op_tmp, symbol->var_type->size));

        now->ir.emplace_back(
            std::make_shared<SplIrAssignAddressInstruction>(op_var, op_tmp));
    }
    return name;
}

void handle_dec(SplAstNode *now) {
    auto &name = handle_vardec(now->children[0]); // vardec
    if (now->children.size() == 3) {
        // VarDec ASSIGN Exp
        traverse_exp(now->children[2]);
        auto &exp_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        if (exp_ir_var.is_addr) { // val = addr
            now->ir.emplace_back(
                std::make_shared<SplIrAssignDerefSrcInstruction>(
                    ir_module.get_operand_by_name(
                        spl_var_name_2_ir_var_name[name]),
                    ir_module.get_operand_by_name(exp_ir_var.var)));
        } else { // val = val
            now->ir.emplace_back(std::make_shared<SplIrAssignInstruction>(
                ir_module.get_operand_by_name(spl_var_name_2_ir_var_name[name]),
                ir_module.get_operand_by_name(exp_ir_var.var)));
        }
    }
}

void collect_ir_by_postorder(SplAstNode *root) {
    collect_ir_by_postorder(root, root);
}

void collect_ir_by_postorder(SplAstNode *now, SplAstNode *root) {
    // collect ir to root node by post order
    SplIrInstructionList tmp;
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

void traverse_exp(SplAstNode *now) {
    // Caution: deref before use exp
    // Caution: deref before use exp
    // Caution: deref before use exp
    if (now->attr.type == SplAstNodeType::SPL_AND ||
        now->attr.type == SplAstNodeType::SPL_OR) {

        auto label = std::make_shared<SplIrLabelInstruction>(
            ir_module.label_counter->next());
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
            ir_module.get_or_make_constant_operand_by_name(
                now->attr.val<SplValExp>().ir_var.var);
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
            auto op_tmp = ir_module.tmp_counter->next();
            auto op_child = deref(now->children[1]);
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            now->ir.emplace_back(std::make_shared<SplIrAssignMinusInstruction>(
                op_tmp, ir_module.get_or_make_constant_operand_by_name("#0"),
                op_child));
            break;
        }
        }
    } else if (now->children.size() == 3) {
        switch (now->children[1]->attr.type) {
        case SplAstNodeType::SPL_ASSIGN: {
            // Exp -> Exp ASSIGN Exp
            auto &dst_ir_var = now->children[0]->attr.val<SplValExp>().ir_var;
            auto op_dst = ir_module.get_operand_by_name(dst_ir_var.var);
            auto &src_ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
            auto op_src = ir_module.get_operand_by_name(src_ir_var.var);
            now->attr.val<SplValExp>().ir_var = dst_ir_var;

            if (!dst_ir_var.is_addr &&
                src_ir_var.is_addr) { // var = addr, optimization
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignDerefSrcInstruction>(op_dst,
                                                                     op_src));
            } else if (dst_ir_var.is_addr) { // addr = var
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignDerefDstInstruction>(op_dst,
                                                                     op_src));
            } else { // var = var
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignInstruction>(op_dst, op_src));
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
            auto op_lhs = deref(now->children[0]),
                 op_rhs = deref(now->children[2]);
            auto if_ins = std::make_shared<SplIrIfGotoInstruction>(
                op_lhs, op_rhs, now->children[1]->attr.type);
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
            auto op_tmp = ir_module.tmp_counter->next(),
                 op_lhs = deref(now->children[0]),
                 op_rhs = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                op_tmp, op_lhs, op_rhs));
            break;
        }
        case SplAstNodeType::SPL_MINUS: {
            // Exp -> Exp MINUS Exp
            auto op_tmp = ir_module.tmp_counter->next(),
                 op_lhs = deref(now->children[0]),
                 op_rhs = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            now->ir.emplace_back(std::make_shared<SplIrAssignMinusInstruction>(
                op_tmp, op_lhs, op_rhs));
            break;
        }
        case SplAstNodeType::SPL_MUL: {
            // Exp -> Exp MUL Exp
            auto op_tmp = ir_module.tmp_counter->next(),
                 op_lhs = deref(now->children[0]),
                 op_rhs = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            now->ir.emplace_back(std::make_shared<SplIrAssignMulInstruction>(
                op_tmp, op_lhs, op_rhs));
            break;
        }
        case SplAstNodeType::SPL_DIV: {
            // Exp -> Exp DIV Exp
            auto op_tmp = ir_module.tmp_counter->next(),
                 op_lhs = deref(now->children[0]),
                 op_rhs = deref(now->children[2]);
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            now->ir.emplace_back(std::make_shared<SplIrAssignDivInstruction>(
                op_tmp, op_lhs, op_rhs));
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

            auto op_tmp = ir_module.tmp_counter->next();
            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                op_tmp, ir_module.get_operand_by_name(base_addr_var),
                ir_module.get_or_make_constant_operand_by_name(
                    "#" + std::to_string(offset))));
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr, true};
            break;
        }
        case SplAstNodeType::SPL_LP: {
            // Exp -> ID LP RP
            // call without args
            auto op_tmp = ir_module.tmp_counter->next();
            now->attr.val<SplValExp>().ir_var = {op_tmp->repr};
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "read") {
                now->ir.emplace_back(
                    std::make_shared<SplIrReadInstruction>(op_tmp));
            } else {
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignCallInstruction>(
                        op_tmp, ir_module.get_or_make_function_operand_by_name(
                                    func_name, false)));
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
            std::shared_ptr<SplIrOperand> op;
            std::string func_name =
                now->children[0]->attr.val<SplValId>().val_id;
            if (func_name == "write") {
                auto arg_ins = std::static_pointer_cast<SplIrArgInstruction>(
                    now->ir.back());
                op = arg_ins->arg;
                now->ir.pop_back();
                now->ir.emplace_back(
                    std::make_shared<SplIrWriteInstruction>(op));
            } else {
                op = ir_module.tmp_counter->next();
                now->ir.emplace_back(
                    std::make_shared<SplIrAssignCallInstruction>(
                        op, ir_module.get_or_make_function_operand_by_name(
                                func_name, false)));
            }
            now->attr.val<SplValExp>().ir_var = {op->repr};
            break;
        }
        case SplAstNodeType::SPL_LB: {
            // Exp -> Exp LB Exp RB
            auto arr_size = now->children[0]
                                ->attr.val<SplValExp>()
                                .type->get_arr_size_for_current_index();
            std::string &base_addr_var =
                now->children[0]->attr.val<SplValExp>().ir_var.var;
            auto op_base_addr = ir_module.get_operand_by_name(base_addr_var);
            auto op_accesser = deref(now->children[2]);

            if (op_accesser->repr == "#0") {
                now->attr.val<SplValExp>().ir_var = {base_addr_var, true};
                break;
            }
            auto op_mul = ir_module.tmp_counter->next();
            auto op_add = ir_module.tmp_counter->next();
            now->ir.emplace_back(std::make_shared<SplIrAssignMulInstruction>(
                op_mul, op_accesser,
                ir_module.get_or_make_constant_operand_by_name(
                    "#" + std::to_string(arr_size))));

            now->ir.emplace_back(std::make_shared<SplIrAssignAddInstruction>(
                op_add, op_base_addr, op_mul));
            now->attr.val<SplValExp>().ir_var = {op_add->repr, true};
            break;
        }
        }
    }
}

void handle_stmt(SplAstNode *now) {
    switch (now->children[0]->attr.type) {
    case SPL_EXP: {
        traverse_exp(now->children[0]);
        return;
    }
    case SPL_COMPST: {
        traverse_comp(now->children[0]);
        return;
    }
    case SPL_RETURN: {
        traverse_exp(now->children[1]);
        auto ret = deref(now->children[1]);
        now->ir.emplace_back(std::make_shared<SplIrReturnInstruction>(ret));
        return;
    }
    case SPL_IF: {
        // IF LP Exp RP Stmt (ELSE Stmt)
        traverse_exp(now->children[2]);
        auto label = ir_module.label_counter->next();
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(label));
        auto &ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        for (auto &v : ir_var.truelist) {
            v->patch(label);
        }

        label = ir_module.label_counter->next();
        handle_stmt(now->children[4]);

        if (now->children.size() == 7) { // ELSE Stmt
            handle_stmt(now->children[6]);
            auto end_else_label_operand = ir_module.label_counter->next();
            now->children[6]->ir.emplace_back(
                std::make_shared<SplIrLabelInstruction>(
                    end_else_label_operand));

            now->children[4]->ir.emplace_back(
                std::make_shared<SplIrGotoInstruction>(end_else_label_operand));
        }
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(label));
        for (auto &v : ir_var.falselist) {
            v->patch(label);
        }

        return;
    }
    case SPL_WHILE: {
        // WHILE LP Exp RP Stmt
        auto exp_begin_label = ir_module.label_counter->next();
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(exp_begin_label));

        traverse_exp(now->children[2]);
        auto label = ir_module.label_counter->next();
        now->children[2]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(label));
        auto &ir_var = now->children[2]->attr.val<SplValExp>().ir_var;
        for (auto &v : ir_var.truelist) {
            v->patch(label);
        }

        label = ir_module.label_counter->next();
        handle_stmt(now->children[4]);
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrGotoInstruction>(exp_begin_label));
        now->children[4]->ir.emplace_back(
            std::make_shared<SplIrLabelInstruction>(label));
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

void opt_ir() {
    bool dead_basic_block_eliminated = true;
    bool copy_propagation_eliminated = true;
    bool fall_through_updated = true;
    while (dead_basic_block_eliminated || copy_propagation_eliminated ||
           fall_through_updated) {
        {
            dead_basic_block_eliminated = false;
            for (auto it_bb = ir_module.basic_blocks.begin();
                 it_bb != ir_module.basic_blocks.end(); it_bb++) {
                if ((*it_bb)->predecessors.empty() &&
                    (*(*it_bb)->head)->type != SplIrInstructionType::FUNCTION) {
#ifdef SPL_IR_GENERATOR_DEBUG
                    std::cout << "dead basic block: " << (*it_bb)->name
                              << std::endl;
#endif
                    auto it_inst_tmp = (*it_bb)->head;
                    while (it_inst_tmp != ir_module.ir.end() &&
                           (*it_inst_tmp)->parent == (*it_bb)) {
                        auto it_inst_prev = it_inst_tmp;
                        it_inst_tmp++;
                        ir_module.erase_instruction(it_inst_prev);
                    }
                    // for (auto successor : (*it_bb)->successors) {
                    //     successor->predecessors.erase(
                    //         std::find(successor->predecessors.begin(),
                    //                   successor->predecessors.end(),
                    //                   *it_bb));
                    // }
                    // ir_module.basic_blocks.erase(it_bb);
                    ir_module.rebuild_basic_blocks();
                    dead_basic_block_eliminated = true;
                    break;
                }
            }
        }
        {
            copy_propagation_eliminated = false;
            for (auto it_inst = ir_module.ir.begin();
                 it_inst != ir_module.ir.end(); it_inst++) {
                if ((*it_inst)->type == SplIrInstructionType::ASSIGN) {
                    auto inst_assign =
                        std::static_pointer_cast<SplIrAssignInstruction>(
                            *it_inst);
                    if (inst_assign->dst->is_l_value()) {
                        auto &use_list =
                            ir_module.use_lists[inst_assign->src->repr];
                        auto it_inst_prev = it_inst;
                        it_inst_prev--;
                        if (use_list.size() == 2 &&
                            std::find(use_list.begin(), use_list.end(),
                                      *it_inst_prev) != use_list.end()) {
                            bool op_is_prev_dst = false;
                            switch ((*it_inst_prev)->type) {
                            case SplIrInstructionType::ASSIGN: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_ADD: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignAddInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_MINUS: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignMinusInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_MUL: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignMulInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_DIV: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignDivInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_CALL: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignCallInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            case SplIrInstructionType::ASSIGN_ADDRESS: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrAssignAddressInstruction>(
                                    *it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            // case SplIrInstructionType::ASSIGN_DEREF_DST:
                            // case SplIrInstructionType::ASSIGN_DEREF_SRC:
                            case SplIrInstructionType::READ: {
                                auto inst_prev = std::static_pointer_cast<
                                    SplIrReadInstruction>(*it_inst_prev);
                                op_is_prev_dst =
                                    inst_prev->dst == inst_assign->src;
                                break;
                            }
                            }
                            if (op_is_prev_dst) {
                                ir_module.replace_usage(*it_inst_prev,
                                                        inst_assign->src,
                                                        inst_assign->dst);
                                ir_module.erase_instruction(it_inst);
                                ir_module.rebuild_basic_blocks();
                                copy_propagation_eliminated = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        {
            fall_through_updated = false;
            for (auto it_label = ir_module.ir.begin();
                 it_label != ir_module.ir.end(); it_label++) {
                if ((*it_label)->type == SplIrInstructionType::LABEL) {
                    auto it_goto = it_label;
                    it_goto--;
                    if ((*it_goto)->type == SplIrInstructionType::GOTO) {
                        auto it_if_goto = it_goto;
                        it_if_goto--;
                        if ((*it_if_goto)->type ==
                            SplIrInstructionType::IF_GOTO) {
                            auto inst_if_goto = std::static_pointer_cast<
                                SplIrIfGotoInstruction>(*it_if_goto);
                            auto inst_goto =
                                std::static_pointer_cast<SplIrGotoInstruction>(
                                    *it_goto);
                            auto inst_label =
                                std::static_pointer_cast<SplIrLabelInstruction>(
                                    *it_label);
                            auto &use_list =
                                ir_module.use_lists[inst_if_goto->label.value()
                                                        ->repr];
                            if (inst_if_goto->label.value() ==
                                    inst_label->label &&
                                use_list.size() == 2) {
                                ir_module.replace_usage(*it_if_goto,
                                                        inst_if_goto->label.value(),
                                                        inst_goto->label.value());
                                inst_if_goto->relop = inst_if_goto->negated_relop(inst_if_goto->relop);
                                ir_module.erase_instruction(it_goto);
                                ir_module.erase_instruction(it_label);
                                ir_module.rebuild_basic_blocks();
                                fall_through_updated = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}