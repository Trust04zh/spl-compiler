#include "spl-semantic-analyzer-module.cpp"
#include <iostream>
#include <sstream>
#include <unordered_map>

std::stringstream out;

std::unordered_map<std::string, std::string> spl_val_name_2_ir_var_name;

void traverse_ir(const SplAstNode *now);

void generate_ir() {
    traverse_ir(prog);
    std::cout << out.str() << std::endl;
}

bool check_skip(const SplAstNode *now) {
    return now->attr.type == SplAstNodeType::SPL_EXTDEF &&
           (now->children[1]->attr.type == SplAstNodeType::SPL_EXTDECLIST ||
            now->children[0]->attr.type == SplAstNodeType::SPL_STRUCTSPECIFIER);
}

void insert_fundec(const SplAstNode *now) {
    out << "FUNCTION " << now->name << std::endl;
    if (now->children.size() == 4) { // has varList
        now = now->children[2];      // VarList
        while (true) {
            // insert
            if (now->children.size() == 1)
                break;
        }
    }
}

void handle_exp(const SplAstNode *now) {}

void handle_def(const SplAstNode *now) {}

void handle_comp(const SplAstNode *now) {}

void handle_vardec(const SplAstNode *now) {
    // TODO: lookup in symbol table, use auto increment helper to generate ir variable
}

void traverse_ir(const SplAstNode *now) {
    if (check_skip(now))
        return;

    auto parent = now->parent;

    switch (now->attr.type) {
    case SPL_PROGRAM:
    case SPL_EXTDEFLIST:
    case SPL_EXTDEF: {
        break;
    }
    case SPL_DEFLIST: {
        handle_def(now);
        return;
    }
    case SPL_FUNDEC: {
        insert_fundec(now);
        return;
    }
    case SPL_COMPST: {
        handle_comp(now);
        return;
    }
    case SPL_SPECIFIER: {
        switch (now->parent->attr.type) {
        case SPL_EXTDEF: {
            break;
        }
        default: {
            std::cout << "SPEC " << now->parent->name << std::endl;
        }
        }
    }
    case SPL_DEF: {
        break;
    }
    case SPL_DEC: {
        break;
    }
    case SPL_VARDEC: {
        // TODO: support struct and array (requires DEC instruction)
        break;
    }
    case SPL_EXP: {
        handle_exp(now);
        return;
    }
    case SPL_ID: {
        if (parent != nullptr && parent->attr.type == SPL_VARDEC) {
            handle_vardec(now);
        }
        break;
    }
    default: {
        std::cout << now->name << std::endl;
    }
    }

    for (auto &c : now->children) {
        traverse_ir(c);
    }
}
