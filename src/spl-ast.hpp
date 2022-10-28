#ifndef SPL_AST_HPP
#define SPL_AST_HPP

#include <cstdio>
#include <vector>
#include <cassert>

enum SplAstNodeType {
    SPL_EMPTY, SPL_INT, SPL_FLOAT, SPL_CHAR, SPL_TYPE, SPL_ID, SPL_TERMINAL, SPL_NONTERMINAL,
};

typedef union SplVal SplVal;
union SplVal{
    struct {
        char* raw;
        int value;
    } val_int;
    struct {
        char *raw;
        float value;    
    } val_float;
    struct {
        char *raw;
        char value;
    } val_char;
    char *val_id;
    char *val_type;
};

typedef struct SplAttr SplAttr;
struct SplAttr {
    SplAstNodeType type;
    SplVal *value_p;
};

// an imitation struct of YYLTYPE
typedef struct SplLoc SplLoc;
struct SplLoc{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
};

typedef struct SplAstNode SplAstNode;
struct SplAstNode{
    std::vector<SplAstNode*> children;
    char const *name;
    SplAttr attr;
    SplLoc loc;

    SplAstNode(char const *name, SplAttr attr, SplLoc loc) {
        construct_node(name, attr, loc);
    }

    template <typename... Args>
    SplAstNode(char const *name, SplAttr attr, SplLoc loc, Args... children) {
        construct_node(name, attr, loc);
        add_child(children...);
    }

    void construct_node(char const *name, SplAttr attr, SplLoc loc) {
        this->name = name;
        this->attr.type = attr.type;
        if (attr.value_p != nullptr) {
            this->attr.value_p = new SplVal();
            *(this->attr.value_p) = *(attr.value_p);
        } else {
            this->attr.value_p = nullptr;
        }
        this->loc = loc;
        // print_formatted(0, 2);
    }

    template <typename T>
    void add_child(T *child) {
        children.push_back(child);
    }

    template <typename T, typename... Args>
    void add_child(T first_child, Args... rest) {
        add_child(first_child);
        add_child(rest...);
    }

    void print_formatted(int indent, int indent_step) {
        /* iterate over children tree with identation */
        if (attr.type == SPL_EMPTY) {
            return;
        }
        for (int i = 0; i < indent; i++) {
            putchar(' ');
        }
        printf("%s", name);
        switch (attr.type) {
            case SPL_NONTERMINAL:
                printf(" (%d)", loc.first_line);
                break;
            case SPL_TERMINAL:
                break;
            case SPL_INT:
                assert(attr.value_p != nullptr);
                printf(": %d", attr.value_p->val_int.value);
                break;
            case SPL_FLOAT:
                assert(attr.value_p != nullptr);
                // printf(": %f", attr.value_p->val_float.value);
                printf(": %s", attr.value_p->val_float.raw);
                break;
            case SPL_CHAR:
                assert(attr.value_p != nullptr);
                // printf(": %c", attr.value_p->val_char.value);
                printf(": %s", attr.value_p->val_char.raw);
                break;
            case SPL_ID:
                assert(attr.value_p != nullptr);
                printf(": %s", attr.value_p->val_id);
                break;
            case SPL_TYPE:
                assert(attr.value_p != nullptr);
                printf(": %s", attr.value_p->val_type);
                break;
            default:
                assert(false);
                break;
        }
        putchar('\n');
        for (std::vector<SplAstNode*>::const_iterator it = children.cbegin(); it != children.cend(); ++it) {
            (*it)->print_formatted(indent + indent_step, indent_step);
        }
    }
};

#endif  /* SPL_AST_HPP */