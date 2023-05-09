#ifndef LLVM_RANGE_H
#define LLVM_RANGE_H
#include "llvm/ADT/MapVector.h"
#include <llvm/IR/Function.h>
#include <optional>
#include <vector>
#define DEBUG_TYPE "range"

#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/StringRef.h>

#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include <set>

#include "kint_constant_range.h"
#include <z3++.h>

namespace llvm {

const unsigned maxIterations = 10; // Or any other suitable value

// TODO DenseMap provides better performance in many cases when compared to
// std::unordered_map due to its memory layout and fewer cache misses.
using RangeMap = std::unordered_map<Value *, KintConstantRange>;

class KintRangeAnalysisPass : public PassInfoMixin<KintRangeAnalysisPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  // for range error checking
  std::set<GetElementPtrInst *> gepOutOfRange;

private:
  // store the range for each value
  RangeMap globalRangeMap;
  DenseMap<GlobalValue *, KintConstantRange> globalValueRangeMap;
  DenseMap<Function *, KintConstantRange> functionReturnRangeMap;
  SetVector<Function *> taintedFunctions;
  SetVector<StringRef> sinkedFunctions;
  MapVector<Function *, std::vector<CallInst *>> functionsToTaintSources;
  DenseMap<BasicBlock *, SetVector<BasicBlock *>> backEdges;
  DenseMap<Value *, std::optional<z3::expr>> argValuetoSymbolicVar;
  std::map<Function *, RangeMap> functionsToRangeInfo;
  std::map<BasicBlock *, SmallVector<BasicBlock *, 2>> BBpathMap;
  std::optional<z3::solver> Solver;
  std::map<ICmpInst *, bool> impossibleBranches;
  z3::expr ValuetoSymbolicVar(Value *arg);

  void initGlobalVariables(Module &M);
  void initFunctionReturn(Module &M);
  void initRange(Module &M);
  void funcSinkCheck(Function &F);
  bool analyzeFunction(Function &F, RangeMap &globalRangeMap);
  void markSinkedFuncs(Function &F);
  void smtSolver(Module &M);
  void backEdgeAnalysis(Function &F);
  void pathSolver(BasicBlock *curBB, BasicBlock *predBB);
  bool addRangeConstaints(const KintConstantRange &range, const z3::expr &bv);
	bool sinkedReachable(Instruction *I);
	bool isTaintSource(const StringRef funcName);

	std::vector<CallInst *> getTaintSource(Function &F);

  KintConstantRange getRange(Value *var, RangeMap &rangeMap);
  KintConstantRange getRangeByBB(Value *var, BasicBlock *BB);
  KintConstantRange handleCallInst(CallInst *operand, RangeMap &globalRangeMap,
                                   Instruction &I);
  KintConstantRange handleStoreInst(StoreInst *operand,
                                    RangeMap &globalRangeMap, Instruction &I);
  KintConstantRange handleReturnInst(ReturnInst *operand,
                                     RangeMap &globalRangeMap, Instruction &I);
  KintConstantRange handleSelectInst(SelectInst *operand,
                                     RangeMap &globalRangeMap, Instruction &I);

  KintConstantRange handleCastInst(CastInst *operand, RangeMap &globalRangeMap,
                                   Instruction &I);
  KintConstantRange handlePHINode(PHINode *operand, RangeMap &globalRangeMap,
                                  Instruction &I);
  KintConstantRange handleLoadInst(LoadInst *operand, RangeMap &globalRangeMap,
                                   Instruction &I);
};

PassPluginLibraryInfo getKintRangeAnalysisPassPluginInfo();

} // namespace llvm

#endif
