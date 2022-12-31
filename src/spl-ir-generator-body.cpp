#include "spl-semantic-analyzer-module.cpp"
#include <iostream>
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

void handle_exp(SplAstNode *now);
void traverse_exp(SplAstNode *now);

void handle_dec(SplAstNode *now);

void generate_ir() {
    traverse_ir(prog);
    std::cout << "----------------------" << std::endl;
    std::cout << out.str() << std::endl;
}

void insert_fundec(SplAstNode *now) {
    out << "FUNCTION " << now->children[0]->attr.val<SplValId>().val_id
        << std::endl;
    auto &name = now->children[0]->attr.val<SplValId>().val_id;
    auto func_symbol =
        std::static_pointer_cast<SplFunctionSymbol>(*(symbols.lookup(name)));
    for (auto &param : func_symbol->params) {
        out << "PARAM: " << param->name << std::endl;
    }
}

void handle_exp(SplAstNode *now) {
    // TODO: extract ir from childrens by post order
    // switch (now->attr.type) {
    // case SPL_ID: {
    //     out << target_var << " := " << now->attr.val<SplValId>().val_id <<
    //     std::endl; return;
    // }
    // case SPL_INT: {
    //     out << target_var << " := #" <<
    //     std::get<int>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // case SPL_CHAR: {
    //     out << target_var << " := #" <<
    //     std::get<char>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // case SPL_FLOAT: {
    //     out << target_var << " := #" <<
    //     std::get<float>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // default: {
    //     std::cout << "exp: " << now->name << std::endl;
    // }
    // }
}

void traverse_exp(SplAstNode *now) {
    if (now->attr.type != SPL_EXP) {
        return;
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
            // SplIrOperand dst(SplIrOperand::R_VALUE_TEMPORARY, ir_var);
            // SplIrOperand src1(SplIrOperand::L_VALUE_CONSTANT, "#0");
            // SplIrOperand src2(ir_name_2_operand_type(child_ir_var),
            //                   child_ir_var);
            // SplIrAssignMinusInstruction ins(std::move(dst), std::move(src1),
            // std::move(src2));
            auto ins_ptr = std::make_unique<SplIrAssignMinusInstruction>(
                SplIrOperand(SplIrOperand::R_VALUE_TEMPORARY, ir_var),
                SplIrOperand(SplIrOperand::L_VALUE_CONSTANT, "#0"),
                SplIrOperand(ir_name_2_operand_type(child_ir_var),
                             child_ir_var));
            now->ir.push_back(std::move(ins_ptr));
            break;
        }
        }
    }
    for (auto &child : now->children) {
        traverse_exp(child);
    }
}

void handle_dec(SplAstNode *now) {
    SplAstNode *tmp = now->children[0]; // VARDEC
    while (tmp->children.size() == 4) { // is array
        tmp = tmp->children[0];
    }
    auto &id = tmp->children[0];
    auto &name = id->attr.val<SplValId>().val_id;
    auto symbol =
        std::static_pointer_cast<SplVariableSymbol>(*(symbols.lookup(name)));
    spl_var_name_2_ir_var_name[name] = var_counter.next();
    if (symbol->var_type->is_array_or_struct()) {
        // out << "DEC " << name << " [size]" << std::endl;
        // TODO: fill size.
    }
    if (now->children.size() == 3) {
        // VarDec ASSIGN Exp
        handle_exp(now->children[2]);
        // TODO: do assignment
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
        insert_fundec(now);
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
