#include "kint_module.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

PreservedAnalyses KintModulePass::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << "Running KintModulePass on module: " << M.getName() << "\n";
	for (auto &F : M) {
		errs() << "Function: " << F.getName() << "\n";
	}
  return PreservedAnalyses::all();
}
