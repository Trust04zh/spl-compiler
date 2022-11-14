#include "spl-parser-module.cpp"
#include "spl-ast.hpp"

extern SplAstNode *prog;
extern bool hasError;

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
    switch (current->attr.type) {
        case SplAstNodeType::SPL_VARDEC:
            break;
        default:
            break;
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