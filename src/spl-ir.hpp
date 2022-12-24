#ifndef SPL_IR_HPP
#define SPL_IR_HPP

#include <string>

using SplIrVariableRepr = std::string;

constexpr size_t SPL_INT_SIZE = 4;

class SplIrOperand {
    enum OperandType { CONSTANT, VARIABLE, FUNCTION, LABEL };
};

class SplIrInstruction {};

class SplIrBasicBlock {};

class SplIrAutoIncrementHelper {
  private:
    std::string prefix;
    int counter;

  public:
    SplIrAutoIncrementHelper(std::string prefix, int start = 0)
        : prefix(prefix), counter(start) {}
    std::string get() { return prefix + std::to_string(counter); }
};

#endif /* SPL_IR_HPP */
