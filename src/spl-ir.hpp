#ifndef SPL_IR_HPP
#define SPL_IR_HPP

#include "spl-enum.hpp"
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
    LABEL
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

class SplIrAutoIncrementHelper;

class SplIrOperand;
class SplIrInstruction;
class SplIrBasicBlock;
class SplIrModule;

using SplIrList = std::list<std::shared_ptr<SplIrInstruction>>;

class SplIrOperand {
  public:
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
    SplIrOperandType type;
    std::string repr;
    SplIrOperand(SplIrOperandType type, std::string repr)
        : type(type), repr(repr) {}
    SplIrOperand(const SplIrOperand &) = default;
};

class SplIrInstruction {
  public:
    SplIrInstructionType type;
    explicit SplIrInstruction(SplIrInstructionType type) : type(type) {}
    virtual void print(std::stringstream &out) = 0;
};

class SplIrLabelInstruction : public SplIrInstruction {
  public:
    SplIrOperand label;
    SplIrLabelInstruction(SplIrOperand &&label)
        : label(std::move(label)),
          SplIrInstruction(SplIrInstructionType::LABEL) {
        if (!this->label.is_label()) {
            throw std::runtime_error(
                "Label instruction must have label operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "LABEL " << label.repr << " :" << std::endl;
    }
};

class Patchable {
  public:
    virtual void patch(const SplIrOperand &label) = 0;
};

class SplIrFunctionInstruction : public SplIrInstruction {
  public:
    SplIrOperand func;
    SplIrFunctionInstruction(SplIrOperand &&func)
        : func(std::move(func)),
          SplIrInstruction(SplIrInstructionType::FUNCTION) {
        if (!this->func.is_function()) {
            throw std::runtime_error(
                "Function instruction must have function operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "FUNCTION " << func.repr << " :" << std::endl;
    }
};

class SplIrAssignInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src;
    SplIrAssignInstruction(SplIrOperand &&dst, SplIrOperand &&src)
        : dst(std::move(dst)), src(std::move(src)),
          SplIrInstruction(SplIrInstructionType::ASSIGN) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error(
                "Assign instruction must have l-value destination operand");
        }
        if (!this->src.is_value()) {
            throw std::runtime_error(
                "Assign instruction must have value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := " << src.repr << std::endl;
    }
};

class SplIrAssignAddInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src1, src2;
    SplIrAssignAddInstruction(SplIrOperand &&dst, SplIrOperand &&src1,
                              SplIrOperand &&src2)
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_ADD) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error(
                "AssignAdd instruction must have l-value destination operand");
        }
        if (!this->src1.is_value() || !this->src2.is_value()) {
            throw std::runtime_error(
                "AssignAdd instruction must have value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := " << src1.repr << " + " << src2.repr
            << std::endl;
    }
};

class SplIrAssignMinusInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src1, src2;
    SplIrAssignMinusInstruction(SplIrOperand &&dst, SplIrOperand &&src1,
                                SplIrOperand &&src2)
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_MINUS) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("AssignMinus instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src1.is_value() || !this->src2.is_value()) {
            throw std::runtime_error(
                "AssignMinus instruction must have value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := " << src1.repr << " - " << src2.repr
            << std::endl;
    }
};

class SplIrAssignMulInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src1, src2;
    SplIrAssignMulInstruction(SplIrOperand &&dst, SplIrOperand &&src1,
                              SplIrOperand &&src2)
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_MUL) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error(
                "AssignMul instruction must have l-value destination operand");
        }
        if (!this->src1.is_value() || !this->src2.is_value()) {
            throw std::runtime_error(
                "AssignMul instruction must have value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := " << src1.repr << " * " << src2.repr
            << std::endl;
    }
};

class SplIrAssignDivInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src1, src2;
    SplIrAssignDivInstruction(SplIrOperand &&dst, SplIrOperand &&src1,
                              SplIrOperand &&src2)
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DIV) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error(
                "AssignDiv instruction must have l-value destination operand");
        }
        if (!this->src1.is_value() || !this->src2.is_value()) {
            throw std::runtime_error(
                "AssignDiv instruction must have value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := " << src1.repr << " / " << src2.repr
            << std::endl;
    }
};

class SplIrAssignAddressInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src;
    SplIrAssignAddressInstruction(SplIrOperand &&dst, SplIrOperand &&src)
        : dst(std::move(dst)), src(std::move(src)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_ADDRESS) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("AssignAddress instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src.is_l_value_variable()) {
            throw std::runtime_error("AssignAddress instruction must have "
                                     "r-value variable source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := &" << src.repr << std::endl;
    }
};

class SplIrAssignDerefSrcInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src;
    SplIrAssignDerefSrcInstruction(SplIrOperand &&dst, SplIrOperand &&src)
        : dst(std::move(dst)), src(std::move(src)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DEREF_SRC) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "l-value destination operand");
        }
        if (!this->src.is_l_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "l-value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := *" << src.repr << std::endl;
    }
};

class SplIrAssignDerefDstInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src;
    SplIrAssignDerefDstInstruction(SplIrOperand &&dst, SplIrOperand &&src)
        : dst(std::move(dst)), src(std::move(src)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_DEREF_DST) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("AssignDerefDst instruction must have "
                                     "l-value destination operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "*" << dst.repr << " := " << src.repr << std::endl;
    }
};

class SplIrGotoInstruction : public SplIrInstruction, public Patchable {
  public:
    std::optional<SplIrOperand> label;
    explicit SplIrGotoInstruction(SplIrOperand &&label)
        : label{std::move(label)},
          SplIrInstruction(SplIrInstructionType::GOTO) {}
    SplIrGotoInstruction() : SplIrInstruction(SplIrInstructionType::GOTO) {}

    void patch(const SplIrOperand &label) override { this->label = label; }

    void print(std::stringstream &out) override {
        if (!this->label.has_value()) {
            throw std::runtime_error("Goto instruction label unfilled");
        }
        if (!this->label->is_label()) {
            throw std::runtime_error(
                "Goto instruction must have label operand");
        }
        out << "GOTO " << label->repr << std::endl;
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
    SplIrOperand lhs, rhs;
    std::optional<SplIrOperand> label;
    Relop relop;
    SplIrIfGotoInstruction(SplIrOperand &&lhs, SplIrOperand &&rhs,
                           const SplAstNodeType &relop)
        : lhs(std::move(lhs)), rhs(std::move(rhs)),
          relop(astNodeTypeToRelop(relop)),
          SplIrInstruction(SplIrInstructionType::IF_GOTO) {
        if (!this->lhs.is_value() || !this->rhs.is_value()) {
            throw std::runtime_error("IfGoto instruction must have value "
                                     "operands");
        }
    }
    SplIrIfGotoInstruction(SplIrOperand &&lhs, SplIrOperand &&rhs, Relop relop)
        : lhs(std::move(lhs)), rhs(std::move(rhs)), relop(relop),
          SplIrInstruction(SplIrInstructionType::IF_GOTO) {
        if (!this->lhs.is_value() || !this->rhs.is_value()) {
            throw std::runtime_error("IfGoto instruction must have value "
                                     "operands");
        }
    }

    void patch(const SplIrOperand &label) override { this->label = label; }

    void print(std::stringstream &out) override {
        if (!this->label.has_value()) {
            throw std::runtime_error("IfGoto instruction has no label");
        }
        if (!this->label->is_label()) {
            throw std::runtime_error("IfGoto instruction must have label "
                                     "operand");
        }
        out << "IF " << lhs.repr << " " << relop_to_string(relop) << " "
            << rhs.repr << " GOTO " << label->repr << std::endl;
    }
};

class SplIrReturnInstruction : public SplIrInstruction {
  public:
    SplIrOperand src;
    SplIrReturnInstruction(SplIrOperand &&src)
        : src(std::move(src)), SplIrInstruction(SplIrInstructionType::RETURN) {
        if (!this->src.is_value()) {
            throw std::runtime_error(
                "Return instruction must have value operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "RETURN " << src.repr << std::endl;
    }
};

class SplIrDecInstruction : public SplIrInstruction {
  public:
    SplIrOperand variable;
    int size;
    SplIrDecInstruction(SplIrOperand &&variable, int size)
        : variable(std::move(variable)), size(size),
          SplIrInstruction(SplIrInstructionType::DEC) {
        if (!this->variable.is_l_value_variable()) {
            throw std::runtime_error("Dec instruction must have "
                                     "r-value variable source operand");
        }
        if (size <= 0 || size % 4 != 0) {
            throw std::runtime_error("Dec instruction must have "
                                     "positive multiple of 4 size");
        }
    }
    void print(std::stringstream &out) override {
        out << "DEC " << variable.repr << " " << size << std::endl;
    }
};

class SplIrArgInstruction : public SplIrInstruction {
  public:
    SplIrOperand arg;
    SplIrArgInstruction(SplIrOperand &&arg)
        : arg(std::move(arg)), SplIrInstruction(SplIrInstructionType::ARG) {
        if (!this->arg.is_value()) {
            throw std::runtime_error("Arg instruction must have value operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "ARG " << arg.repr << std::endl;
    }
};

class SplIrAssignCallInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, func;
    SplIrAssignCallInstruction(SplIrOperand &&dst, SplIrOperand &&func)
        : dst(std::move(dst)), func(std::move(func)),
          SplIrInstruction(SplIrInstructionType::ASSIGN_CALL) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("Call instruction must have "
                                     "l-value destination operand");
        }
        if (!this->func.is_function()) {
            throw std::runtime_error("Call instruction must have "
                                     "function operand to call");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := CALL " << func.repr << std::endl;
    }
};

class SplIrParamInstruction : public SplIrInstruction {
  public:
    SplIrOperand param;
    SplIrParamInstruction(SplIrOperand &&param)
        : param(std::move(param)),
          SplIrInstruction(SplIrInstructionType::PARAM) {
        if (!this->param.is_value()) {
            throw std::runtime_error(
                "Param instruction must have value operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "PARAM " << param.repr << std::endl;
    }
};

class SplIrReadInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst;
    SplIrReadInstruction(SplIrOperand &&dst)
        : dst(std::move(dst)), SplIrInstruction(SplIrInstructionType::READ) {
        if (!this->dst.is_l_value()) {
            throw std::runtime_error("Read instruction must have "
                                     "l-value destination operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "READ " << dst.repr << std::endl;
    }
};

class SplIrWriteInstruction : public SplIrInstruction {
  public:
    SplIrOperand src;
    SplIrWriteInstruction(SplIrOperand &&src)
        : src(std::move(src)), SplIrInstruction(SplIrInstructionType::WRITE) {
        if (!this->src.is_value()) {
            throw std::runtime_error("Write instruction must have "
                                     "value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "WRITE " << src.repr << std::endl;
    }
};

class SplIrBasicBlock {};

class SplIrModule {
  public:
    SplIrList ir_list;
    std::unordered_map<std::string, SplIrList> use_list;
    std::list<SplIrBasicBlock> basic_blocks;

    explicit SplIrModule(const SplIrList &ir_list) : ir_list(ir_list) {}

};

class SplIrAutoIncrementHelper {
  private:
    std::string prefix;
    int counter;

  public:
    SplIrAutoIncrementHelper(std::string prefix, int start = 0)
        : prefix(prefix), counter(start) {}
    std::string next() {
        std::string str = prefix + std::to_string(counter);
        counter++;
        return str;
    }
};

#endif /* SPL_IR_HPP */
