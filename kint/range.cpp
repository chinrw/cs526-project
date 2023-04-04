#include "range.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/ConstantRange.h"

using namespace llvm;

// compute the range for a given BinaryOperator instruction
ConstantRange computeBinaryOperatorRange(BinaryOperator *&BO,
                                         const RangeMap &globalRangeMap) {
  ConstantRange lhsRange = globalRangeMap.at(BO->getOperand(0));
  ConstantRange rhsRange = globalRangeMap.at(BO->getOperand(1));

  switch (BO->getOpcode()) {
    case Instruction::Add:
      return lhsRange.add(rhsRange);
    case Instruction::Sub:
      return lhsRange.sub(rhsRange);
    case Instruction::Mul:
      return lhsRange.multiply(rhsRange);
    case Instruction::SDiv:
      return lhsRange.sdiv(rhsRange);
    case Instruction::UDiv:
      return lhsRange.udiv(rhsRange);
    case Instruction::Shl:
      return lhsRange.shl(rhsRange);
    case Instruction::LShr:
      return lhsRange.lshr(rhsRange);
    case Instruction::AShr:
      return lhsRange.ashr(rhsRange);
    case Instruction::SRem:
      return lhsRange.srem(rhsRange);
    case Instruction::URem:
      return lhsRange.urem(rhsRange);
    case Instruction::And:
      return lhsRange.binaryAnd(rhsRange);
    case Instruction::Or:
      return lhsRange.binaryOr(rhsRange);
    case Instruction::Xor:
      return lhsRange.binaryXor(rhsRange);
    default:
      // Handle other instructions and cases as needed.
      errs() << "Unhandled binary operator: " << BO->getOpcodeName() << "\n";
      return rhsRange;
  }
}

bool KintRangeAnalysisPass::analyzeFunction(Function &F,
                                            RangeMap &globalRangeMap) {
  // TODO add a function to check if the globalRangeMap has converged
  bool functionConverged = false;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      // Get the range for operands or insert a new one if it doesn't exist
      // for (Use &use : I.operands()) {
      //   if (auto *operandValue = dyn_cast<Value>(&use)) {
      //     globalRangeMap.emplace(
      //         &I, ConstantRange::getFull(
      //                 operandValue->getType()->getIntegerBitWidth()));
      //   }
      // }
      if (auto *operand = dyn_cast_or_null<BinaryOperator>(&I)) {
        outs() << "Found binary operator: " << operand->getOpcodeName() << "\n";
        globalRangeMap.emplace(
            &I, ConstantRange::getFull(I.getType()->getIntegerBitWidth()));
        ConstantRange outputRange =
            computeBinaryOperatorRange(operand, globalRangeMap);
        globalRangeMap.at(&I) = outputRange;
      }
    }
  }
  return functionConverged;
}

PreservedAnalyses KintRangeAnalysisPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  auto &LCG = MAM.getResult<LazyCallGraphAnalysis>(M);

  RangeMap globalRangeMap;
  const size_t maxIterations = 10;

  // Initialize the ranges for the globalRangeMap.

  // Perform the range analysis iteratively until convergence or a fixed
  // number of iterations.
  for (size_t iteration = 0; iteration < maxIterations; ++iteration) {
    bool hasConverged = true;

    LCG.buildRefSCCs();

    for (LazyCallGraph::RefSCC &ref_scc : LCG.postorder_ref_sccs()) {
      for (LazyCallGraph::SCC &scc : ref_scc) {
        for (LazyCallGraph::Node &node : scc) {
          Function &F = node.getFunction();

          // analyzeFunction now returns a boolean indicating if the function
          // analysis has converged
          bool functionConverged = analyzeFunction(F, globalRangeMap);

          // Update hasConverged if function analysis has not converged
          hasConverged = functionConverged;
        }
      }
    }

    if (hasConverged) {
      break;
    }
  }

  // Use the globalRangeMap for further analysis or optimization.

  return PreservedAnalyses::all();
}
