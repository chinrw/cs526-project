#pragma once
#include <llvm/IR/ConstantRange.h>

namespace llvm {

class KintConstantRange : public ConstantRange {
public:
  // fix the ConstantRange constructor
  KintConstantRange() : ConstantRange(0, true) {}

  KintConstantRange(const ConstantRange &cr) : ConstantRange(cr) {}
  KintConstantRange(const APInt &value) : ConstantRange(value) {}
  KintConstantRange(const APInt &lower, const APInt &upper)
      : ConstantRange(lower, upper) {}
  KintConstantRange(unsigned bitwidth, bool isFullSet)
      : ConstantRange(bitwidth, isFullSet) {}
};

struct ValueRange {
  int64_t lowerBound;
  int64_t upperBound;
};

// TODO DenseMap provides better performance in many cases when compared to
// std::unordered_map due to its memory layout and fewer cache misses.
using RangeMap = std::unordered_map<Value *, KintConstantRange>;

} // namespace llvm
