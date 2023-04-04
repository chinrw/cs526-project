#ifndef LLVM_RANGE_H
#define LLVM_RANGE_H

#include <llvm/Support/raw_ostream.h>

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

namespace llvm {

const unsigned maxIterations = 10;  // Or any other suitable value
struct ValueRange {
  int64_t lowerBound;
  int64_t upperBound;
};

// TODO DenseMap provides better performance in many cases when compared to
// std::unordered_map due to its memory layout and fewer cache misses.
using RangeMap = std::unordered_map<Value *, ConstantRange>;

class KintRangeAnalysisPass : public PassInfoMixin<KintRangeAnalysisPass> {
 public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);

 private:
  bool analyzeFunction(Function &F, RangeMap &globalRangeMap);
};

PassPluginLibraryInfo getKintRangeAnalysisPassPluginInfo();
}  // namespace llvm

#endif
