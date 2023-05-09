#include "range.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <llvm/Support/Casting.h>

#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/ConstantRange.h"

using namespace llvm;

// compute the range for a given BinaryOperator instruction
KintConstantRange computeBinaryOperatorRange(BinaryOperator *&BO,
                                             const RangeMap &globalRangeMap) {
  // FIXME: this is a hack to get the range for the operands
  KintConstantRange lhsRange =
      KintConstantRange(globalRangeMap.at(BO->getOperand(0)));
  KintConstantRange rhsRange =
      KintConstantRange(globalRangeMap.at(BO->getOperand(1)));

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
      auto &F = *BB.getParent();
      auto &sumRng = functionsToRangeInfo[&F][&BB];
    for (Instruction &I : BB) {
      auto getRng = [&globalRangeMap, this](auto var) 
      {return getRange(var, globalRangeMap);};
      if (auto *call = dyn_cast_or_null<CallInst>(&I)) {
        auto itI = globalRangeMap.find(&I);
        if (itI != globalRangeMap.end()) {
          globalRangeMap.at(&I) = this->handleCallInst(call, globalRangeMap, I);
        }
      } else if (auto *store = dyn_cast_or_null<StoreInst>(&I)) {
          globalRangeMap.at(&I) = 
              this->handleStoreInst(store, globalRangeMap, I);
      } else if (auto *ret = dyn_cast_or_null<ReturnInst>(&I)) {
          globalRangeMap.at(&I) = 
              this->handleReturnInst(ret, globalRangeMap, I);
      }
      if (auto *operand = dyn_cast_or_null<BinaryOperator>(&I)) {
        // outs() << "Found binary operator: " << operand->getOpcodeName() <<
        // "\n";
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
        globalRangeMap.at(&I) =
            this->handleSelectInst(operand, globalRangeMap, I);
        } else if (auto *operand = dyn_cast_or_null<CastInst>(&I)) {
          globalRangeMap.at(&I) = this->handleCastInst(operand,
          globalRangeMap, I);
        } else if (auto *operand = dyn_cast_or_null<PHINode>(&I)) {
          globalRangeMap.at(&I) = this->handlePHINode(operand,
          globalRangeMap, I);
        } else if (auto *operand = dyn_cast_or_null<LoadInst>(&I)) {
          globalRangeMap.at(&I) = this->handleLoadInst(operand,
          globalRangeMap, I);
      }
    }
  }
  return functionConverged;
}

void KintRangeAnalysisPass::printRanges() {
  outs() << "Printing ranges for all function return\n";
  for (auto [F, range] : functionReturnRangeMap) {
    outs() << "Function: " << F->getName() << "\n";
    outs() << "Range: " << range << "\n";
  }

  outs() << "Printing ranges for all global variables\n";
  for (auto [GV, range] : globalRangeMap) {
    outs() << "Global Variable: " << GV->getName() << "\n";
    outs() << "Range: " << range << "\n";
  }

  outs() << "Printing ranges for array index out of bounds\n";
  for (auto G : gepOutOfRange) {
    outs() << "Instruction name: " << G->getName() << "\n";
  }

  outs() << "Printing tainted Functions\n";
  for (auto F : taintedFunctions) {
    outs() << "Function: " << F->getName() << "\n";
  }

  outs() << "Printing tainted Instructions\n";
  for (auto [F, I] : functionsToTaintSources) {
    outs() << "Function: " << F->getName() << "\n";
  }

  outs() << "Printing sinked Functions\n";
  for (auto F : sinkedFunctions) {
    outs() << "Function: " << F << "\n";
  }
}

PreservedAnalyses KintRangeAnalysisPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {

  outs() << "Running KintRangeAnalysisPass on module" << M.getName() << "\n";
  auto &LCG = MAM.getResult<LazyCallGraphAnalysis>(M);

  auto ctx = new z3::context;
  Solver = z3::solver(*ctx);

  // mark taint sources
  for (auto &F : M) {
    auto taintSources = getTaintSource(F);
    markSinkedFuncs(F);
    if (isTaintSource(F.getName())) {
      functionsToTaintSources[&F] = std::move(taintSources);
    }
  }

  // Initialize the ranges for the globalRangeMap.
  initRange(M);

  const size_t maxIterations = 10;

  // Perform the range analysis iteratively until convergence or a fixed
  // number of iterations.
  for (size_t iteration = 0; iteration < maxIterations; ++iteration) {

    const auto originalFunctionsRangeInfo = functionsToRangeInfo;
    const auto originalGlobalValueRangeMap = globalValueRangeMap;
    const auto originalFunctionReturnRangeMap = functionReturnRangeMap;

    if (functionsToRangeInfo == originalFunctionsRangeInfo &&
        globalValueRangeMap == originalGlobalValueRangeMap &&
        functionReturnRangeMap == originalFunctionReturnRangeMap) {
      // converaged
      break;
    }
  }

  smtSolver(M);
  printRanges();

  return PreservedAnalyses::all();
}
