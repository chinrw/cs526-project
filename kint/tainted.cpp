#include "range.h"
#include <llvm/IR/Function.h>
using namespace llvm;

void checkFuncSinked(Module &M) {
  for (auto &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *CI = dyn_cast<CallInst>(&I)) {
          if (CI->getCalledFunction()->getName() == "taint") {
          }
        }
      }
    }
  }
}

void KintRangeAnalysisPass::funcSinkCheck(Function &F) {
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (CI->getCalledFunction()->getName() == "taint") {
        }
      }
    }
  }
}
