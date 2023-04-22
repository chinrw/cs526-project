#pragma once
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/ConstantRange.h>
#include <vector>

namespace llvm {
// Add functions for userspace test
const auto TAINT_FUNCS = std::vector<StringRef>{
    "fgets",  "gets",    "scanf",  "sscanf",
    "vscanf", "vsscanf", "fscanf", "vfscanf",
};

const auto SINK_FUNCS = DenseMap<StringRef, uint64_t>{
    {"dma_alloc_from_coherent", 1},
    {"__kmalloc", 0},
    {"kmalloc", 0},
    {"__kmalloc_node", 0},
    {"kmalloc_node", 0},
    {"kzalloc", 0},
    {"kcalloc", 0},
    {"kcalloc", 1},
    {"kmemdup", 1},
    {"memdup_user", 1},
    {"pci_alloc_consistent", 1},
    {"copy_from_user", 2},
    {"memcpy", 2},
    {"mem_alloc", 0},
    {"__sink0", 0},
    {"__sink1", 1},
    {"__sink2", 2},
    {"__writel", 0},
    {"__vmalloc", 0},
    {"vmalloc", 0},
    {"vmalloc_user", 0},
    {"vmalloc_node", 0},
    {"vzalloc", 0},
    {"vzalloc_node", 0},
};

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
