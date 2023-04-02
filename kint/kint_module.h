#ifndef KINT_MODULE_PASS_H
#define KINT_MODULE_PASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {
class KintModulePass : public PassInfoMixin<KintModulePass> {
 public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};
}  // namespace llvm

#endif  // KINT_MODULE_PASS_H
