#ifndef LLVM_RANGE_H
#define LLVM_RANGE_H

#include <llvm/Support/raw_ostream.h>

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
