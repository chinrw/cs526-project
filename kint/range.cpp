#include "range.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>

#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/ConstantRange.h"

using namespace llvm;

// compute the range for a given BinaryOperator instruction
KintConstantRange computeBinaryOperatorRange(BinaryOperator *&BO,
                                         const RangeMap &globalRangeMap) {
  // FIXME: this is a hack to get the range for the operands
  KintConstantRange lhsRange = KintConstantRange(globalRangeMap.at(BO->getOperand(0)));
  KintConstantRange rhsRange = KintConstantRange(globalRangeMap.at(BO->getOperand(1)));

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
      // if (const auto store = dyn_cast<StoreInst>(&I)) {
      //   // get global variable
      //   const auto global =
      //       dyn_cast<GlobalVariable>(store->getPointerOperand());
      // }

      if (auto *operand = dyn_cast_or_null<BinaryOperator>(&I)) {
        outs() << "Found binary operator: " << operand->getOpcodeName() << "\n";
        // FIXME this has the wrong key for the map
        globalRangeMap.emplace(
            operand->getOperand(0),
            ConstantRange::getFull(
                operand->getOperand(0)->getType()->getIntegerBitWidth()));
        globalRangeMap.emplace(
            operand->getOperand(1),
            ConstantRange::getFull(
                operand->getOperand(1)->getType()->getIntegerBitWidth()));

        // ConstantRange outputRange =
        //     computeBinaryOperatorRange(operand, globalRangeMap);
        // globalRangeMap.at(&I) = outputRange;
      } else if (auto *operand = dyn_cast_or_null<SelectInst>(&I)) {
        globalRangeMap.at(&I) = handleSelectInst(operand, globalRangeMap, I);
      } else if (auto *operand = dyn_cast_or_null<CastInst>(&I)) {
        globalRangeMap.at(&I) = handleCastInst(operand, globalRangeMap, I);
      } else if (auto *operand = dyn_cast_or_null<PHINode>(&I)) {
        globalRangeMap.at(&I) = handlePHINode(operand, globalRangeMap, I);
      } else if (auto *operand = dyn_cast_or_null<LoadInst>(&I)) {
        globalRangeMap.at(&I) = handleLoadInst(operand, globalRangeMap, I);
      }
    }
  }
  return functionConverged;
}

PreservedAnalyses KintRangeAnalysisPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {

  outs() << "Running KintRangeAnalysisPass on module" << M.getName() << "\n";
  auto &LCG = MAM.getResult<LazyCallGraphAnalysis>(M);

  const size_t maxIterations = 10;

  // Initialize the ranges for the globalRangeMap.

  // Perform the range analysis iteratively until convergence or a fixed
  // number of iterations.
  for (size_t iteration = 0; iteration < maxIterations; ++iteration) {
    bool hasConverged = true;

    LCG.buildRefSCCs();

    initRange(M);

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
