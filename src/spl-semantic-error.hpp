#include <string>
using std::string;

void report_semantic_error(int type_id, const SplAstNode *const node) {
    int lineno = node->loc.first_line;
    switch (type_id) {
        case 1:
            printf("Error type 1 at Line %d: a variable is used without a definition\n", lineno);
            break;
        case 2:
            printf("Error type 2 at Line %d: a function is invoked without a definition\n", lineno);
            break;
        case 3:
            printf("Error type 3 at Line %d: a variable is redefined in the same scope\n", lineno);
            break;
        case 4:
            printf("Error type 4 at Line %d: a function is redefined (in the global scope, since we don't have nested functions)\n", lineno);
            break;
        case 5:
            printf("Error type 5 at Line %d: unmatching types appear at both sides of the assignment operator (=)\n", lineno);
            break;
        case 6:
            printf("Error type 6 at Line %d: rvalue appears on the left-hand side of the assignment operator\n", lineno);
            break;
        case 7:
            printf("Error type 7 at Line %d: unmatching operands, such as adding an integer to a structure variable\n", lineno);
            break;
        case 8:
            printf("Error type 8 at Line %d: a function's return value type mismatches the declared type\n", lineno);
            break;
        case 9:
            printf("Error type 9 at Line %d: a function's arguments mismatch the declared parameters (either types or numbers, or both)\n", lineno);
            break;
        case 10:
            printf("Error type 10 at Line %d: applying indexing operator ([...]) on non-array type variables\n", lineno);
            break;
        case 11:
            printf("Error type 11 at Line %d: applying function invocation operator (foo(...)) on non-function names\n", lineno);
            break;
        case 12:
            printf("Error type 12 at Line %d: array indexing with a non-integer type expression\n", lineno);
            break;
        case 13:
            printf("Error type 13 at Line %d: accessing members of a non-structure variable (i.e., misuse the dot operator)\n", lineno);
            break;
        case 14:
            printf("Error type 14 at Line %d: accessing an undefined structure member\n", lineno);
            break;
        case 15:
            printf("Error type 15 at Line %d: redefine the same structure type\n", lineno);
            break;
        case 21:
            printf("Error type 21 at Line %d: only int variables can do boolean operations\n", lineno);
            break;
    }
}