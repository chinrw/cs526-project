#include "kint_function.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void visitor(Function &F) {
  outs() << "(llvm-tutor) Hello from: " << F.getName() << "\n";
  outs() << "(llvm-tutor)   number of arguments: " << F.arg_size() << "\n";
}

PreservedAnalyses KintFunctionPass::run(Function &F,
                                        FunctionAnalysisManager &) {
  visitor(F);
  return PreservedAnalyses::all();
}
