#ifndef SMT_QUERY_PASS_H
#define SMT_QUERY_PASS_H

#include <llvm/IR/Function.h>

#include "llvm/IR/PassManager.h"

namespace llvm {
struct SMTQueryPass : PassInfoMixin<SMTQueryPass> {
 public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FPM);
  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};

}  // namespace llvm

#endif
