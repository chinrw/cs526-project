#include "smt_query.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace llvm;

namespace {
class CallInstVisitor : public InstVisitor<CallInstVisitor> {
public:
  CallInstVisitor() {}
  void visitCallInst(CallInst &CI);

private:
};
} // anonymous namespace

void CallInstVisitor::visitCallInst(CallInst &CI) {
// TODO
}

PreservedAnalyses SMTQueryPass::run(Function &F, FunctionAnalysisManager &) {
  CallInstVisitor CIV;
  CIV.visit(F);

  return PreservedAnalyses::all();
}
