#ifndef KINT_FUNCTION_PASS_H
#define KINT_FUNCTION_PASS_H

#include <llvm/IR/Function.h>

#include "llvm/IR/PassManager.h"

namespace llvm {
// This method implements what the pass does

// New PM implementation
struct KintFunctionPass : PassInfoMixin<KintFunctionPass> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  // Without isRequired returning true, this pass will be skipped for functions
 public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FPM);
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};

}  // namespace llvm

#endif
