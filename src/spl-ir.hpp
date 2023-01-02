#ifndef SPL_IR_HPP
#define SPL_IR_HPP

#include "spl-enum.hpp"
#include <algorithm>
#include <forward_list>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

constexpr size_t SPL_INT_SIZE = 4;

enum class SplIrOperandType {
    R_VALUE_CONSTANT,
    L_VALUE_TEMPORARY, // they are always intermediate products of
                       // expressions, do optimization on them
    L_VALUE_VARIABLE,
    FUNCTION,
    LABEL,
    BASIC_BLOCK
};

enum class SplIrInstructionType {
    LABEL,
    FUNCTION,
    ASSIGN,
    ASSIGN_ADD,
    ASSIGN_MINUS,
    ASSIGN_MUL,
    ASSIGN_DIV,
    ASSIGN_ADDRESS,
    ASSIGN_DEREF_SRC,
    ASSIGN_DEREF_DST,
    GOTO,
    IF_GOTO,
    RETURN,
    DEC,
    ARG,
    ASSIGN_CALL,
    PARAM,
    READ,
    WRITE
};

class SplIrOperand;
class SplIrInstruction;
class SplIrBasicBlock;

using SplIrOperandRefList =
    std::forward_list<std::reference_wrapper<std::shared_ptr<SplIrOperand>>>;
// using SplIrOperandRefList =
//     std::list<std::reference_wrapper<std::shared_ptr<SplIrOperand>>>;
using SplIrInstructionList = std::list<std::shared_ptr<SplIrInstruction>>;
using SplIrBasicBlockList = std::list<std::shared_ptr<SplIrBasicBlock>>;
using SplIrOperandName2Operand =
    std::unordered_map<std::string, std::shared_ptr<SplIrOperand>>;

class SplIrModule;

class Patchable;

class Patchable {
  public:
    virtual void patch(const std::shared_ptr<SplIrOperand> label) = 0;
};

class SplIrOperand {
  private:
    static SplIrOperandType ir_name_2_operand_type(const std::string &name) {
        if (name[0] == '#') {
            return SplIrOperandType::R_VALUE_CONSTANT;
        } else if (name[0] == 'v') {
            return SplIrOperandType::L_VALUE_VARIABLE;
        } else if (name[0] == 't') {
            return SplIrOperandType::L_VALUE_TEMPORARY;
        } else if (name[0] == 'l') {
            return SplIrOperandType::LABEL;
        } else if (name[0] == 'b') {
            return SplIrOperandType::BASIC_BLOCK;
        }
        throw std::runtime_error("unknown operand type for name: " + name);
    }

    SplIrOperand(SplIrOperandType type, std::string repr)
        : type(type), repr(repr) {}
    SplIrOperand(const std::string &repr)
        : type(ir_name_2_operand_type(repr)), repr(repr) {}

    friend class SplIrModule;

  public:
    SplIrOperandType type;
    std::string repr;

    /* FIXME: it looks a bit stupid that friend class
    SplIrModule use this to make_shared SplIrOperand */
    SplIrOperand(const SplIrOperand &) = default;

    bool operator==(const SplIrOperand &rhs) const {
        // use repr to identify operands
        return repr == rhs.repr;
    }

    bool is_value() const {
        return type == SplIrOperandType::R_VALUE_CONSTANT ||
               type == SplIrOperandType::L_VALUE_TEMPORARY ||
               type == SplIrOperandType::L_VALUE_VARIABLE;
    }
    bool is_l_value() const {
        return type == SplIrOperandType::L_VALUE_TEMPORARY ||
               type == SplIrOperandType::L_VALUE_VARIABLE;
    }
    bool is_l_value_variable() const {
        return type == SplIrOperandType::L_VALUE_VARIABLE;
    }
    bool is_function() const { return type == SplIrOperandType::FUNCTION; }
    bool is_label() const { return type == SplIrOperandType::LABEL; }
};

class SplIrInstruction {
  public:
    SplIrInstructionType type;
    /* subclasses should track operands manually */
    SplIrOperandRefList operands;
    std::shared_ptr<SplIrBasicBlock> parent;
    explicit SplIrInstruction(SplIrInstructionType type) : type(type) {}
    virtual void print(std::stringstream &out) = 0;
};

class SplIrLabelInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> label;
    SplIrLabelInstruction(std::shared_ptr<SplIrOperand> label)
        : label(label), SplIrInstruction(SplIrInstructionType::LABEL) {
        if (!this->label->is_label()) {
            throw std::runtime_error(
                "Label instruction must have label operand");
        }
        operands.push_front(this->label);
    }
    void print(std::stringstream &out) override {
        out << "LABEL " << label->repr << " :" << std::endl;
    }
};

class SplIrFunctionInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> func;
    SplIrFunctionInstruction(std::shared_ptr<SplIrOperand> func)
        : func(func), SplIrInstruction(SplIrInstructionType::FUNCTION) {
        if (!this->func->is_function()) {
            throw std::runtime_error(
                "Function instruction must have function operand");
        }
        operands.push_front(this->func);
    }
    void print(std::stringstream &out) override {
        out << "FUNCTION " << func->repr << " :" << std::endl;
    }
};

class SplIrAssignInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src;
    SplIrAssignInstruction(std::shared_ptr<SplIrOperand> dst,
                           std::shared_ptr<SplIrOperand> src)
        : dst(dst), src(src), SplIrInstruction(SplIrInstructionType::ASSIGN) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error(
                "Assign instruction must have l-value destination operand");
        }
        if (!this->src->is_value()) {
            throw std::runtime_error(
                "Assign instruction must have value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := " << src->repr << std::endl;
    }
};

class SplIrAssignAddInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src1, src2;
    SplIrAssignAddInstruction(std::shared_ptr<SplIrOperand> dst,
                              std::shared_ptr<SplIrOperand> src1,
                              std::shared_ptr<SplIrOperand> src2)
        : dst(dst), src1(src1), src2(src2),
          SplIrInstruction(SplIrInstructionType::ASSIGN_ADD) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error(
                "AssignAdd instruction must have l-value destination operand");
        }
        if (!this->src1->is_value() || !this->src2->is_value()) {
            throw std::runtime_error(
                "AssignAdd instruction must have value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src1);
        operands.push_front(this->src2);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := " << src1->repr << " + " << src2->repr
            << std::endl;
    }
};

class SplIrAssignMinusInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src1, src2;
    SplIrAssignMinusInstruction(std::shared_ptr<SplIrOperand> dst,
                                std::shared_ptr<SplIrOperand> src1,
                                std::shared_ptr<SplIrOperand> src2)
        : dst(dst), src1(src1), src2(src2),
          SplIrInstruction(SplIrInstructionType::ASSIGN_MINUS) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("AssignMinus instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src1->is_value() || !this->src2->is_value()) {
            throw std::runtime_error(
                "AssignMinus instruction must have value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src1);
        operands.push_front(this->src2);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := " << src1->repr << " - " << src2->repr
            << std::endl;
    }
};

class SplIrAssignMulInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src1, src2;
    SplIrAssignMulInstruction(std::shared_ptr<SplIrOperand> dst,
                              std::shared_ptr<SplIrOperand> src1,
                              std::shared_ptr<SplIrOperand> src2)
        : dst(dst), src1(src1), src2(src2),
          SplIrInstruction(SplIrInstructionType::ASSIGN_MUL) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error(
                "AssignMul instruction must have l-value destination operand");
        }
        if (!this->src1->is_value() || !this->src2->is_value()) {
            throw std::runtime_error(
                "AssignMul instruction must have value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src1);
        operands.push_front(this->src2);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := " << src1->repr << " * " << src2->repr
            << std::endl;
    }
};

class SplIrAssignDivInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src1, src2;
    SplIrAssignDivInstruction(std::shared_ptr<SplIrOperand> dst,
                              std::shared_ptr<SplIrOperand> src1,
                              std::shared_ptr<SplIrOperand> src2)
        : dst(dst), src1(src1), src2(src2),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DIV) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error(
                "AssignDiv instruction must have l-value destination operand");
        }
        if (!this->src1->is_value() || !this->src2->is_value()) {
            throw std::runtime_error(
                "AssignDiv instruction must have value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src1);
        operands.push_front(this->src2);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := " << src1->repr << " / " << src2->repr
            << std::endl;
    }
};

class SplIrAssignAddressInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src;
    SplIrAssignAddressInstruction(std::shared_ptr<SplIrOperand> dst,
                                  std::shared_ptr<SplIrOperand> src)
        : dst(dst), src(src),
          SplIrInstruction(SplIrInstructionType::ASSIGN_ADDRESS) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("AssignAddress instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src->is_l_value()) {
            throw std::runtime_error("AssignAddress instruction must have "
                                     "l-value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := &" << src->repr << std::endl;
    }
};

class SplIrAssignDerefSrcInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src;
    SplIrAssignDerefSrcInstruction(std::shared_ptr<SplIrOperand> dst,
                                   std::shared_ptr<SplIrOperand> src)
        : dst(dst), src(src),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DEREF_SRC) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src->is_l_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "l-value source operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := *" << src->repr << std::endl;
    }
};

class SplIrAssignDerefDstInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, src;
    SplIrAssignDerefDstInstruction(std::shared_ptr<SplIrOperand> dst,
                                   std::shared_ptr<SplIrOperand> src)
        : dst(dst), src(src),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DEREF_DST) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("AssignDerefDst instruction must have "
                                     "l-value destination operand");
        }
        operands.push_front(this->dst);
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << "*" << dst->repr << " := " << src->repr << std::endl;
    }
};

class SplIrGotoInstruction : public SplIrInstruction, public Patchable {
  public:
    std::optional<std::shared_ptr<SplIrOperand>> label;
    explicit SplIrGotoInstruction(std::shared_ptr<SplIrOperand> label)
        : label{label}, SplIrInstruction(SplIrInstructionType::GOTO) {
        operands.push_front(this->label.value());
    }
    SplIrGotoInstruction() : SplIrInstruction(SplIrInstructionType::GOTO) {}

    void patch(const std::shared_ptr<SplIrOperand> label) override {
        this->label = label;
        operands.push_front(this->label.value());
    }

    void print(std::stringstream &out) override {
        if (!this->label.has_value()) {
            throw std::runtime_error("Goto instruction label unfilled");
        }
        if (!this->label.value()->is_label()) {
            throw std::runtime_error(
                "Goto instruction must have label operand");
        }
        out << "GOTO " << label.value()->repr << std::endl;
    }
};

class SplIrIfGotoInstruction : public SplIrInstruction, public Patchable {
  public:
    enum Relop { EQ, NE, LT, LE, GT, GE };
    static Relop astNodeTypeToRelop(SplAstNodeType t) {
        switch (t) {
        case SplAstNodeType::SPL_LT:
            return LT;
        case SplAstNodeType::SPL_LE:
            return LE;
        case SplAstNodeType::SPL_GT:
            return GT;
        case SplAstNodeType::SPL_GE:
            return GE;
        case SplAstNodeType::SPL_EQ:
            return EQ;
        case SplAstNodeType::SPL_NE:
            return NE;
        default:
            throw std::runtime_error("unrecognized ast node type");
        }
    }

  private:
    static std::string relop_to_string(Relop relop) {
        switch (relop) {
        case EQ:
            return "==";
        case NE:
            return "!=";
        case LT:
            return "<";
        case LE:
            return "<=";
        case GT:
            return ">";
        case GE:
            return ">=";
        }
        throw std::runtime_error("Invalid relop");
    }

  public:
    std::shared_ptr<SplIrOperand> lhs, rhs;
    std::optional<std::shared_ptr<SplIrOperand>> label;
    Relop relop;
    SplIrIfGotoInstruction(std::shared_ptr<SplIrOperand> lhs,
                           std::shared_ptr<SplIrOperand> rhs,
                           const SplAstNodeType &relop)
        : lhs(lhs), rhs(rhs), relop(astNodeTypeToRelop(relop)),
          SplIrInstruction(SplIrInstructionType::IF_GOTO) {
        if (!this->lhs->is_value() || !this->rhs->is_value()) {
            throw std::runtime_error("IfGoto instruction must have value "
                                     "operands");
        }
    }
    SplIrIfGotoInstruction(std::shared_ptr<SplIrOperand> lhs,
                           std::shared_ptr<SplIrOperand> rhs, Relop relop)
        : lhs(lhs), rhs(rhs), relop(relop),
          SplIrInstruction(SplIrInstructionType::IF_GOTO) {
        if (!this->lhs->is_value() || !this->rhs->is_value()) {
            throw std::runtime_error("IfGoto instruction must have value "
                                     "operands");
        }
    }

    void patch(const std::shared_ptr<SplIrOperand> label) override {
        this->label = label;
        operands.push_front(this->label.value());
    }

    void print(std::stringstream &out) override {
        if (!this->label.has_value()) {
            throw std::runtime_error("IfGoto instruction has no label");
        }
        if (!this->label.value()->is_label()) {
            throw std::runtime_error("IfGoto instruction must have label "
                                     "operand");
        }
        out << "IF " << lhs->repr << " " << relop_to_string(relop) << " "
            << rhs->repr << " GOTO " << label.value()->repr << std::endl;
    }
};

class SplIrReturnInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> src;
    SplIrReturnInstruction(std::shared_ptr<SplIrOperand> src)
        : src(src), SplIrInstruction(SplIrInstructionType::RETURN) {
        if (!this->src->is_value()) {
            throw std::runtime_error(
                "Return instruction must have value operand");
        }
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << "RETURN " << src->repr << std::endl;
    }
};

class SplIrDecInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> variable;
    int size;
    SplIrDecInstruction(std::shared_ptr<SplIrOperand> variable, int size)
        : variable(variable), size(size),
          SplIrInstruction(SplIrInstructionType::DEC) {
        if (!this->variable->is_l_value()) {
            throw std::runtime_error("Dec instruction must have "
                                     "l-value source operand");
        }
        if (size <= 0 || size % 4 != 0) {
            throw std::runtime_error("Dec instruction must have "
                                     "positive multiple of 4 size");
        }
        operands.push_front(this->variable);
    }
    void print(std::stringstream &out) override {
        out << "DEC " << variable->repr << " " << size << std::endl;
    }
};

class SplIrArgInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> arg;
    SplIrArgInstruction(std::shared_ptr<SplIrOperand> arg)
        : arg(arg), SplIrInstruction(SplIrInstructionType::ARG) {
        if (!this->arg->is_value()) {
            throw std::runtime_error("Arg instruction must have value operand");
        }
        operands.push_front(this->arg);
    }
    void print(std::stringstream &out) override {
        out << "ARG " << arg->repr << std::endl;
    }
};

class SplIrAssignCallInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst, func;
    SplIrAssignCallInstruction(std::shared_ptr<SplIrOperand> dst,
                               std::shared_ptr<SplIrOperand> func)
        : dst(dst), func(func),
          SplIrInstruction(SplIrInstructionType::ASSIGN_CALL) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("Call instruction must have "
                                     "l-value destination operand");
        }
        if (!this->func->is_function()) {
            throw std::runtime_error("Call instruction must have "
                                     "function operand to call");
        }
        operands.push_front(this->dst);
        operands.push_front(this->func);
    }
    void print(std::stringstream &out) override {
        out << dst->repr << " := CALL " << func->repr << std::endl;
    }
};

class SplIrParamInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> param;
    SplIrParamInstruction(std::shared_ptr<SplIrOperand> param)
        : param(param), SplIrInstruction(SplIrInstructionType::PARAM) {
        if (!this->param->is_value()) {
            throw std::runtime_error(
                "Param instruction must have value operand");
        }
        operands.push_front(this->param);
    }
    void print(std::stringstream &out) override {
        out << "PARAM " << param->repr << std::endl;
    }
};

class SplIrReadInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> dst;
    SplIrReadInstruction(std::shared_ptr<SplIrOperand> dst)
        : dst(dst), SplIrInstruction(SplIrInstructionType::READ) {
        if (!this->dst->is_l_value()) {
            throw std::runtime_error("Read instruction must have "
                                     "l-value destination operand");
        }
        operands.push_front(this->dst);
    }
    void print(std::stringstream &out) override {
        out << "READ " << dst->repr << std::endl;
    }
};

class SplIrWriteInstruction : public SplIrInstruction {
  public:
    std::shared_ptr<SplIrOperand> src;
    SplIrWriteInstruction(std::shared_ptr<SplIrOperand> src)
        : src(src), SplIrInstruction(SplIrInstructionType::WRITE) {
        if (!this->src->is_value()) {
            throw std::runtime_error("Write instruction must have "
                                     "value source operand");
        }
        operands.push_front(this->src);
    }
    void print(std::stringstream &out) override {
        out << "WRITE " << src->repr << std::endl;
    }
};

class SplIrBasicBlock {
  public:
    std::string name;
    SplIrInstructionList::iterator head;
    SplIrBasicBlockList predecessors, successors;
    SplIrBasicBlock(std::string name, SplIrInstructionList::iterator head)
        : name(name), head(head) {}
};

class SplIrModule {
  private:
    SplIrOperandName2Operand operand_name_2_operand;

    class SplIrAutoIncrementHelper {
      private:
        SplIrModule &ir_module;
        std::string prefix;
        int counter;

      public:
        SplIrAutoIncrementHelper(SplIrModule &ir_module, std::string prefix,
                                 int start = 0)
            : ir_module(ir_module), prefix(prefix), counter(start) {}
        std::shared_ptr<SplIrOperand> next() {
            std::string str = prefix + std::to_string(counter);
            counter++;
            ir_module.operand_name_2_operand[str] =
                std::make_shared<SplIrOperand>(SplIrOperand(str));
            return ir_module.operand_name_2_operand[str];
        }
    };

    void build_use_list();
    void build_basic_blocks();
    void mark_head_instruction(SplIrInstructionList::iterator current);
    void register_control_flow_edge(SplIrInstructionList::iterator from,
                                    SplIrInstructionList::iterator to);

  public:
    SplIrInstructionList ir;

    std::unordered_map<std::string, SplIrInstructionList> use_lists;
    SplIrBasicBlockList basic_blocks;

    std::unique_ptr<SplIrAutoIncrementHelper> var_counter, tmp_counter,
        label_counter, bb_counter;

    explicit SplIrModule();

    std::shared_ptr<SplIrOperand> get_operand_by_name(std::string name);

    std::shared_ptr<SplIrOperand>
    get_or_make_constant_operand_by_name(std::string const_name);
    std::shared_ptr<SplIrOperand>
    get_or_make_function_operand_by_name(std::string func_name,
                                         bool make_if_not_exist);

    void fill_ir(const SplIrInstructionList &ir);

    void replace_usage(std::shared_ptr<SplIrInstruction> inst,
                       std::shared_ptr<SplIrOperand> old_op,
                       std::shared_ptr<SplIrOperand> new_op);
    void erase_instruction(SplIrInstructionList::iterator it);
    void rebuild_basic_blocks();

    SplIrInstructionList::iterator
    get_ir_itor_by_inst(std::shared_ptr<SplIrInstruction> inst);
    SplIrInstructionList::iterator
    get_ir_itor_by_func_name(std::string func_name);
    SplIrInstructionList::iterator
    get_ir_itor_by_label_name(std::string label_name);

    void debug_print_use_list() {
        // for debug
        std::stringstream ss;
        for (auto &pair : use_lists) {
            ss << "> " << pair.first << ": " << std::endl;
            for (auto inst : pair.second) {
                inst->print(ss);
            }
        }
        std::cout << ss.str() << std::endl;
    }

    void debug_print_basic_blocks() {
        // for debug
        std::stringstream ss;
        for (auto &inst : ir) {
            ss << std::setw(8) << std::left << inst->parent->name;
            std::stringstream ss_pred;
            ss_pred << "pred: ";
            for (auto &pred : inst->parent->predecessors) {
                ss_pred << pred->name << " ";
            }
            ss << std::setw(20) << std::left << ss_pred.str();
            std::stringstream ss_succ;
            ss_succ << "succ: ";
            for (auto &succ : inst->parent->successors) {
                ss_succ << succ->name << " ";
            }
            ss << std::setw(20) << std::left << ss_succ.str();
            inst->print(ss);
        }
        std::cout << ss.str() << std::endl;
    }
};

void SplIrModule::build_use_list() {
    for (auto &inst : ir) {
        // std::cout << inst->operands.size() << std::endl;
        for (auto operand : inst->operands) {
            // std::cout << operand.get()->repr << std::endl;
            /* Note that an inst may be pushed more than once */
            use_lists[operand.get()->repr].push_back(inst);
        }
    }
}

void SplIrModule::build_basic_blocks() {
    /* requires use_lists */
    // find head instructions
    for (auto it = ir.begin(); it != ir.end(); it++) {
        switch ((*it)->type) {
        case SplIrInstructionType::FUNCTION: {
            mark_head_instruction(it);
            break;
        }
        case SplIrInstructionType::GOTO: {
            std::string label_name =
                (std::static_pointer_cast<SplIrGotoInstruction>(*it))
                    ->label.value()
                    ->repr;
            auto it_label = get_ir_itor_by_label_name(label_name);
            mark_head_instruction(it_label);
            auto next = it;
            next++;
            if (next != ir.end()) {
                mark_head_instruction(next);
            }
            break;
        }
        case SplIrInstructionType::IF_GOTO: {
            std::string label_name =
                (std::static_pointer_cast<SplIrIfGotoInstruction>(*it))
                    ->label.value()
                    ->repr;
            auto it_label = get_ir_itor_by_label_name(label_name);
            mark_head_instruction(it_label);
            auto next = it;
            next++;
            if (next != ir.end()) {
                mark_head_instruction(next);
            }
            break;
        }
        case SplIrInstructionType::RETURN: {
            auto next = it;
            next++;
            if (next != ir.end()) {
                mark_head_instruction(next);
            }
            break;
        }
        }
    }
    // patch parent for instructions
    for (auto basic_block : basic_blocks) {
        auto current = basic_block->head;
        while (current++,
               current != ir.end() && (*current)->parent == nullptr) {
            (*current)->parent = basic_block;
        }
    }
    // build control flow
    // here we do not consider inter procedural analysis (do not consider
    // function call)
    for (auto it = ir.begin(); it != ir.end(); it++) {
        switch ((*it)->type) {
        case SplIrInstructionType::GOTO: {
            std::string label_name =
                (std::static_pointer_cast<SplIrGotoInstruction>(*it))
                    ->label.value()
                    ->repr;
            auto it_label = get_ir_itor_by_label_name(label_name);
            register_control_flow_edge(it, it_label);
            break;
        }
        case SplIrInstructionType::IF_GOTO: {
            std::string label_name =
                (std::static_pointer_cast<SplIrIfGotoInstruction>(*it))
                    ->label.value()
                    ->repr;
            auto it_label = get_ir_itor_by_label_name(label_name);
            register_control_flow_edge(it, it_label);
            auto next = it;
            next++;
            if (next != ir.end()) {
                register_control_flow_edge(it, next);
            }
            break;
        }
        }
    }
    return;
}

void SplIrModule::mark_head_instruction(
    SplIrInstructionList::iterator current) {
    if ((*current)->parent != nullptr) {
        return;
    }
    basic_blocks.push_back(
        std::make_shared<SplIrBasicBlock>(bb_counter->next()->repr, current));
    auto basic_block = basic_blocks.back();
    (*current)->parent = basic_block;
}

void SplIrModule::register_control_flow_edge(
    SplIrInstructionList::iterator from, SplIrInstructionList::iterator to) {
    (*from)->parent->successors.push_back((*to)->parent);
    (*to)->parent->predecessors.push_back((*from)->parent);
}

SplIrModule::SplIrModule()
    : var_counter(std::make_unique<SplIrAutoIncrementHelper>(*this, "v")),
      tmp_counter(std::make_unique<SplIrAutoIncrementHelper>(*this, "t")),
      label_counter(std::make_unique<SplIrAutoIncrementHelper>(*this, "label")),
      bb_counter(std::make_unique<SplIrAutoIncrementHelper>(*this, "bb")){};

std::shared_ptr<SplIrOperand>
SplIrModule::get_operand_by_name(std::string name) {
    if (operand_name_2_operand.count(name) == 0) {
        throw std::runtime_error("Operand " + name + " not found");
    }
    return operand_name_2_operand[name];
}

std::shared_ptr<SplIrOperand>
SplIrModule::get_or_make_constant_operand_by_name(std::string const_name) {
    if (operand_name_2_operand.count(const_name) == 0) {
        operand_name_2_operand[const_name] = std::make_shared<SplIrOperand>(
            SplIrOperand(SplIrOperandType::R_VALUE_CONSTANT, const_name));
    }
    return operand_name_2_operand[const_name];
}

std::shared_ptr<SplIrOperand> SplIrModule::get_or_make_function_operand_by_name(
    std::string func_name, bool make_if_not_exist = true) {
    if (operand_name_2_operand.count(func_name) == 0) {
        if (!make_if_not_exist) {
            throw std::runtime_error("Function " + func_name + " not found");
        }
        operand_name_2_operand[func_name] = std::make_shared<SplIrOperand>(
            SplIrOperand(SplIrOperandType::FUNCTION, func_name));
    }
    return operand_name_2_operand[func_name];
}

void SplIrModule::fill_ir(const SplIrInstructionList &ir) {
    this->ir = ir;
    build_use_list();
#ifdef SPL_IR_GENERATOR_DEBUG
    debug_print_use_list();
#endif
    build_basic_blocks();
#ifdef SPL_IR_GENERATOR_DEBUG
    debug_print_basic_blocks();
#endif
}

void SplIrModule::replace_usage(std::shared_ptr<SplIrInstruction> inst,
                                std::shared_ptr<SplIrOperand> old_op,
                                std::shared_ptr<SplIrOperand> new_op) {
    for (auto &operand : inst->operands) {
        if (operand.get() == old_op) {
            operand.get() = new_op;
            // reduce use count for old_op, increase use count for new_op
            auto old_use_list = use_lists[old_op->repr];
            auto new_use_list = use_lists[new_op->repr];
            for (auto it = old_use_list.begin(); it != old_use_list.end();
                 it++) {
                if (inst == *it) {
                    old_use_list.erase(it);
                    break;
                }
            }
            new_use_list.push_back(inst);
        }
    }
}

void SplIrModule::erase_instruction(SplIrInstructionList::iterator it) {
    // remove usage from use list
    for (auto operand : (*it)->operands) {
        auto &use_list = use_lists[operand.get()->repr];
        for (auto user_inst : use_list) {
            if (user_inst == (*it)) {
                use_list.erase(
                    std::find(use_list.begin(), use_list.end(), (*it)));
                break;
            }
        }
    }
    // remove instruction from ir list
    ir.erase(it);
}

SplIrInstructionList::iterator
SplIrModule::get_ir_itor_by_inst(std::shared_ptr<SplIrInstruction> inst) {
    return std::find(ir.begin(), ir.end(), inst);
}

SplIrInstructionList::iterator
SplIrModule::get_ir_itor_by_func_name(std::string func_name) {
    for (auto it = ir.begin(); it != ir.end(); it++) {
        if ((*it)->type == SplIrInstructionType::FUNCTION) {
            auto func_inst =
                std::static_pointer_cast<SplIrFunctionInstruction>(*it);
            if (func_inst->func->repr == func_name) {
                return it;
            }
        }
    }
    throw std::runtime_error("Function " + func_name + " not found");
}

SplIrInstructionList::iterator
SplIrModule::get_ir_itor_by_label_name(std::string label_name) {
    for (auto it = ir.begin(); it != ir.end(); it++) {
        if ((*it)->type == SplIrInstructionType::LABEL) {
            auto label_inst =
                std::static_pointer_cast<SplIrLabelInstruction>(*it);
            if (label_inst->label->repr == label_name) {
                return it;
            }
        }
    }
    return ir.end();
}

void SplIrModule::rebuild_basic_blocks() {
    for (auto it = ir.begin(); it != ir.end(); it++) {
        (*it)->parent = nullptr;
    }
    basic_blocks.clear();
    build_basic_blocks();
}

#endif /* SPL_IR_HPP */
