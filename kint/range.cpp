#include "range.h"
#include "kint_constant_range.h"

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

void KintRangeAnalysisPass::analyzeFunction(Function &F,
                                            RangeMap &globalRangeMap) {
  // TODO add a function to check if the globalRangeMap has converged
  for (BasicBlock &BB : F) {
    auto &F = *BB.getParent();
    // FIXME unused variable
    auto &sumRng = functionsToRangeInfo[&F][&BB];
    for (Instruction &I : BB) {
      // FIXME unused variable
      auto getRng = [&globalRangeMap, this](auto var) {
        return getRange(var, globalRangeMap);
      };
      if (auto *call = dyn_cast<CallInst>(&I)) {
        auto itI = globalRangeMap.find(&I);
        if (itI != globalRangeMap.end()) {
          globalRangeMap.at(&I) = this->handleCallInst(call, globalRangeMap, I);
        }
      } else if (auto *store = dyn_cast<StoreInst>(&I)) {
        this->handleStoreInst(store, globalRangeMap, I);
      } else if (auto *ret = dyn_cast<ReturnInst>(&I)) {
        this->handleReturnInst(ret, globalRangeMap, I);
      }
      if (auto *operand = dyn_cast<SelectInst>(&I)) {
        globalRangeMap.at(&I) =
            this->handleSelectInst(operand, globalRangeMap, I);
      } else if (auto *operand = dyn_cast<CastInst>(&I)) {
        globalRangeMap.emplace(
            &I, this->handleCastInst(operand, globalRangeMap, I));
      } else if (auto *operand = dyn_cast<BinaryOperator>(&I)) {
        // KintConstantRange outputRange =
        //     computeBinaryOperatorRange(operand, globalRangeMap);
        // globalRangeMap.emplace(&I, outputRange);
      } else if (auto *operand = dyn_cast<PHINode>(&I)) {
        globalRangeMap.at(&I) = this->handlePHINode(operand, globalRangeMap, I);
      } else if (auto *operand = dyn_cast<LoadInst>(&I)) {
        this->handleLoadInst(operand, globalRangeMap, I);
      }
    }
  }
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

  outs() << "Printing impossible branches\n";
  for (auto [Cmp, BB] : impossibleBranches) {
    if (BB) {
      outs() << "Cmp: " << *Cmp << "\n";
    }
  }
}

PreservedAnalyses KintRangeAnalysisPass::run(Module &M,
                                             ModuleAnalysisManager &MAM) {

  outs() << "Running KintRangeAnalysisPass on module" << M.getName() << "\n";

  auto ctx = new z3::context;
  Solver = z3::solver(*ctx);

  // mark taint sources
  for (auto &F : M) {
    if (!F.isDeclaration()) {
      backEdgeAnalysis(F);
    }
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

    for (auto F : rangeAnalysisFunctions) {
      rangeAnalysis(*F);
    }

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
