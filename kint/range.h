#ifndef LLVM_RANGE_H
#define LLVM_RANGE_H
#define DEBUG_TYPE "range"

#include "llvm/IR/GlobalValue.h"
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
  DenseMap<const GlobalValue *, KintConstantRange> globalValueRangeMap;
  DenseMap<const Function *, KintConstantRange> functionReturnRangeMap;
  SetVector<Function *> taintedFunctions;
  SetVector<StringRef> sinkedFunctions;

  void initGlobalVariables(Module &M);
  void initFunctionReturn(Module &M);
  void initRange(Module &M);
  void funcSinkCheck(Function &F);
  bool analyzeFunction(Function &F, RangeMap &globalRangeMap);
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
