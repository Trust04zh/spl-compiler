#ifndef SPL_IR_HPP
#define SPL_IR_HPP

#include <sstream>
#include <string>

using SplIrVariableRepr = std::string;

constexpr size_t SPL_INT_SIZE = 4;

class SplIrOperand {
  public:
    enum OperandType {
        L_VALUE_CONSTANT,
        R_VALUE_TEMPORARY, // they are always intermediate products of
                           // expressions, do optimization on them
        R_VALUE_VARIABLE,
        FUNCTION,
        LABEL
    };
    bool is_value() const {
        return type == L_VALUE_CONSTANT || type == R_VALUE_TEMPORARY ||
               type == R_VALUE_VARIABLE;
    }
    bool is_r_value() const {
        return type == R_VALUE_TEMPORARY || type == R_VALUE_VARIABLE;
    }
    bool is_r_value_variable() const { return type == R_VALUE_VARIABLE; }
    bool is_function() const { return type == FUNCTION; }
    bool is_label() const { return type == LABEL; }
    OperandType type;
    std::string repr;
    SplIrOperand(OperandType type, std::string repr) : type(type), repr(repr) {}
};

class SplIrInstruction {
  public:
    enum InstructionType {
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
    InstructionType type;
    virtual void print(std::stringstream &out) = 0;
};

class SplIrLabelInstruction : public SplIrInstruction {
  public:
    SplIrOperand label;
    SplIrLabelInstruction(SplIrOperand &&label) : label(std::move(label)) {
        if (!this->label.is_label()) {
            throw std::runtime_error(
                "Label instruction must have label operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "LABEL " << label.repr << " :" << std::endl;
    }
};

class SplIrFunctionInstruction : public SplIrInstruction {
  public:
    SplIrOperand func;
    SplIrFunctionInstruction(SplIrOperand &&func) : func(std::move(func)) {
        if (!this->func.is_function()) {
            throw std::runtime_error(
                "Function instruction must have function operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "FUNCTION " << func.repr << std::endl;
    }
};

class SplIrAssignInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, src;
    SplIrAssignInstruction(SplIrOperand &&dst, SplIrOperand &&src)
        : dst(std::move(dst)), src(std::move(src)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error(
                "Assign instruction must have r-value destination operand");
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
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error(
                "AssignAdd instruction must have r-value destination operand");
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
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("AssignMinus instruction must have "
                                     "r-value destination operand");
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
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error(
                "AssignMul instruction must have r-value destination operand");
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
        : dst(std::move(dst)), src1(std::move(src1)), src2(std::move(src2)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error(
                "AssignDiv instruction must have r-value destination operand");
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
        : dst(std::move(dst)), src(std::move(src)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("AssignAddress instruction must have "
                                     "r-value destination operand");
        }
        if (!this->src.is_r_value_variable()) {
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
        : dst(std::move(dst)), src(std::move(src)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "r-value destination operand");
        }
        if (!this->src.is_r_value()) {
            throw std::runtime_error("AssignDerefSrc instruction must have "
                                     "r-value source operand");
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
        : dst(std::move(dst)), src(std::move(src)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("AssignDerefDst instruction must have "
                                     "r-value destination operand");
        }
        if (!this->src.is_r_value()) {
            throw std::runtime_error("AssignDerefDst instruction must have "
                                     "r-value source operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "*" << dst.repr << " := " << src.repr << std::endl;
    }
};

class SplIrGotoInstruction : public SplIrInstruction {
  public:
    SplIrOperand label;
    SplIrGotoInstruction(SplIrOperand &&label) : label(label) {
        if (!this->label.is_label()) {
            throw std::runtime_error(
                "Goto instruction must have label operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "GOTO " << label.repr << std::endl;
    }
};

class SplIrIfGotoInstruction : public SplIrInstruction {
  public:
    enum Relop { EQ, NE, LT, LE, GT, GE };

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
    SplIrOperand lhs, rhs, label;
    Relop relop;
    SplIrIfGotoInstruction(SplIrOperand &&lhs, SplIrOperand &&rhs, Relop relop,
                           SplIrOperand &&label)
        : lhs(std::move(lhs)), rhs(std::move(rhs)), relop(relop),
          label(std::move(label)) {
        if (!this->lhs.is_value() || !this->rhs.is_value()) {
            throw std::runtime_error("IfGoto instruction must have value "
                                     "operands");
        }
        if (!this->label.is_label()) {
            throw std::runtime_error("IfGoto instruction must have label "
                                     "operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "IF " << lhs.repr << " " << relop_to_string(relop) << " "
            << rhs.repr << " GOTO " << label.repr << std::endl;
    }
};

class SplIrReturnInstruction : public SplIrInstruction {
  public:
    SplIrOperand src;
    SplIrReturnInstruction(SplIrOperand &&src) : src(std::move(src)) {
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
    SplIrDecInstruction(SplIrOperand &&variable, int size) : variable(std::move(variable)), size(size) {
        if (!this->variable.is_r_value_variable()) {
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
    SplIrArgInstruction(SplIrOperand &&arg) : arg(std::move(arg)) {
        if (!this->arg.is_value()) {
            throw std::runtime_error("Arg instruction must have value operand");
        }
    }
    void print(std::stringstream &out) override {
        out << "ARG " << arg.repr << std::endl;
    }
};

class SplIrCallInstruction : public SplIrInstruction {
  public:
    SplIrOperand dst, func;
    SplIrCallInstruction(SplIrOperand &&dst, SplIrOperand &&func)
        : dst(std::move(dst)), func(std::move(func)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("Call instruction must have "
                                     "r-value destination operand");
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
    SplIrParamInstruction(SplIrOperand &&param) : param(std::move(param)) {
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
    SplIrReadInstruction(SplIrOperand &&dst) : dst(std::move(dst)) {
        if (!this->dst.is_r_value()) {
            throw std::runtime_error("Read instruction must have "
                                     "r-value destination operand");
        }
    }
    void print(std::stringstream &out) override {
        out << dst.repr << " := READ" << std::endl;
    }
};

class SplIrWriteInstruction : public SplIrInstruction {
  public:
    SplIrOperand src;
    SplIrWriteInstruction(SplIrOperand &&src) : src(std::move(src)) {
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

class SplIrAutoIncrementHelper {
  private:
    std::string prefix;
    int counter;

  public:
    SplIrAutoIncrementHelper(std::string prefix, int start = 0)
        : prefix(prefix), counter(start) {}
    std::string next() { return prefix + std::to_string(counter); }
};

#endif /* SPL_IR_HPP */
