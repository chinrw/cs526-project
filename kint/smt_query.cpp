#include "smt_query.h"
#include "constraint.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
class CallInstVisitor : public InstVisitor<CallInstVisitor> {
public:
  CallInstVisitor() {}
  void visitCallInst(CallInst &CI);
  void CheckFunction(Function &F);
  void Solve(z3::expr Constraint);

private:
  ValueConstraint *VC;
  PathConstraint *PC;
};
} // anonymous namespace

void CallInstVisitor::visitCallInst(CallInst &CI) {
  auto Fn = CI.getCalledFunction();
  if (!Fn) {
    return;
  }
  auto Name = Fn->getName();
  if (Name.equals("__kint_check")) {
    auto P = PC->Get(CI.getParent());
    auto V = VC->Get(CI.getOperand(0)).bit2bool(0);
    Solve(P && V);
  } else if (Name.startswith("__kint_overflow.")) {
    auto P = PC->Get(CI.getParent());

    // auto BitWidth = std::stoi(Name.substr(Name.find_last_of(".") + 1).str());

    auto Opcode = dyn_cast<ConstantInt>(CI.getOperand(0))->getSExtValue();
    auto NSW =
        dyn_cast<ConstantInt>(CI.getOperand(1))->getValue().getBoolValue();
    auto LHS = VC->Get(CI.getOperand(2));
    auto RHS = VC->Get(CI.getOperand(3));
    // we need the condition for overflows
    auto V = LHS.ctx().bool_val(false);
    if (Opcode == Instruction::Add) {
      V = V || !z3::bvadd_no_overflow(LHS, RHS, NSW);
      if (NSW) {
        V = V || !z3::bvadd_no_underflow(LHS, RHS);
      }
    } else if (Opcode == Instruction::Sub) {
      V = V || !z3::bvsub_no_underflow(LHS, RHS, NSW);
      if (NSW) {
        V = V || !z3::bvsub_no_overflow(LHS, RHS);
      }
    } else if (Opcode == Instruction::Mul) {
      V = V || !z3::bvmul_no_overflow(LHS, RHS, NSW);
      if (NSW) {
        V = V || !z3::bvmul_no_underflow(LHS, RHS);
      }
    } else {
      llvm_unreachable("Opcode Impossible");
    }
    Solve(P && V);
  }
}

void CallInstVisitor::CheckFunction(Function &F) {
  z3::context CTX;
  SmallVector<PathConstraint::Edge, 16> BackEdges;
  FindFunctionBackedges(F, BackEdges);

  ValueConstraint VC{CTX, F.getParent()->getDataLayout()};
  PathConstraint PC{CTX, VC, BackEdges};

  // try new()
  this->VC = &VC;
  this->PC = &PC;
  visit(F);
  this->VC = nullptr;
  this->PC = nullptr;
}

void CallInstVisitor::Solve(z3::expr Constraint) {
  auto &CTX = Constraint.ctx();
  z3::solver Solver(CTX);
  Solver.add(!Constraint);
  errs() << "==================================================\n";
  errs() << "Constraint:\n" << Constraint.to_string() << "\n";
  if (Solver.check() == z3::sat) {
    errs() << "Error example:\n" << Solver.get_model().to_string() << "\n";
  } else {
    errs() << "OK\n";
  }
  errs() << "==================================================\n";
}

PreservedAnalyses SMTQueryPass::run(Function &F, FunctionAnalysisManager &) {
  errs() << "Checking function " << F.getName() << ":\n";
  CallInstVisitor CIV;
  CIV.CheckFunction(F);

  return PreservedAnalyses::all();
}
