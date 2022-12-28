#include "spl-semantic-analyzer-module.cpp"
#include <iostream>
#include <sstream>
#include <unordered_map>

std::stringstream out;

std::unordered_map<std::string, std::string> spl_val_name_2_ir_var_name;

void traverse_ir(const SplAstNode *now);

void generate_ir() {
    traverse_ir(prog);
    std::cout << "----------------------" << std::endl;
    std::cout << out.str() << std::endl;
}

void insert_fundec(const SplAstNode *now) {
    out << "FUNCTION " << now->children[0]->attr.val<SplValId>().val_id
        << std::endl;
    auto &name = now->children[0]->attr.val<SplValId>().val_id;
    auto func_symbol =
        std::static_pointer_cast<SplFunctionSymbol>(*(symbols.lookup(name)));
    for (auto &param : func_symbol->params) {
        out << "PARAM: " << param->name << std::endl;
    }
}

void handle_exp(const SplAstNode *now, const std::string &target_var) {
    // switch (now->attr.type) {
    // case SPL_ID: {
    //     out << target_var << " := " << now->attr.val<SplValId>().val_id << std::endl;
    //     return;
    // }
    // case SPL_INT: {
    //     out << target_var << " := #" << std::get<int>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // case SPL_CHAR: {
    //     out << target_var << " := #" << std::get<char>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // case SPL_FLOAT: {
    //     out << target_var << " := #" << std::get<float>(now->attr.val<SplValValue>().value) << std::endl;
    //     return;
    // }
    // default: {
    //     std::cout << "exp: " << now->name << std::endl;
    // }
    // }
}

void handle_dec(const SplAstNode *now) {
    SplAstNode *tmp = now->children[0]; // VARDEC
    while (tmp->children.size() == 4) { // is array
        tmp = tmp->children[0];
    }
    auto &name = tmp->children[0]->attr.val<SplValId>().val_id;
    auto symbol =
        std::static_pointer_cast<SplVariableSymbol>(*(symbols.lookup(name)));
    if (symbol->var_type->is_array_or_struct()) {
        out << "DEC " << name << " [size]" << std::endl; // TODO: fill size.
    } else if (now->children.size() == 3) {              // VarDec ASSIGN Exp
        handle_exp(now->children[2], name);
    }
}

void traverse_comp(const SplAstNode *now);

void handle_stmt(const SplAstNode *now) {
    switch (now->children[0]->attr.type) {
    case SPL_EXP: {
        handle_exp(now->children[0], "");
        return;
    }
    case SPL_COMPST: {
        traverse_comp(now->children[0]);
        return;
    }
    case SPL_RETURN: {
        handle_exp(now->children[1], "ret");
        out << "RETURN ret" << std::endl; // TODO: May optimize in handle_exp
        return;
    }
    case SPL_IF: {
        handle_exp(now->children[2], "if");
        return;
    }
    case SPL_WHILE: {
        handle_exp(now->children[2], "while");
        return;
    }
    }
}

void traverse_comp(const SplAstNode *now) {
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

void traverse_ir(const SplAstNode *now) {
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
