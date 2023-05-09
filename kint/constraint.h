#ifndef KINT_CONSTRAINT_H
#define KINT_CONSTRAINT_H

#include "z3++.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"

class ValueConstraint {
public:
  ValueConstraint(z3::context &CTX, const llvm::DataLayout &DL)
      : CTX(CTX), DL(DL) {}
  z3::expr Get(const llvm::Value *V);
  bool isAnalyzable(const llvm::Value *V);

private:
  z3::context &CTX;
  const llvm::DataLayout &DL;
  z3::expr SymbolFor(const llvm::Value *V);
  z3::expr GetConstant(const llvm::Constant *C);
  z3::expr GetBinaryOperator(const llvm::BinaryOperator *BO);
  z3::expr GetICmpInst(const llvm::ICmpInst *ICI);
  z3::expr GetGEPOperator(const llvm::GEPOperator *GEPO);
  z3::expr GetTruncInst(const llvm::TruncInst *TI);
  z3::expr GetZextInst(const llvm::ZExtInst *ZEI);
  z3::expr GetSExtInst(const llvm::SExtInst *SEI);
  z3::expr GetSelectInst(const llvm::SelectInst *SI);
  z3::expr GetBitCastInst(const llvm::BitCastInst *BCI);
  z3::expr GetIntToPtrInst(const llvm::IntToPtrInst *ITPI);
  z3::expr GetPtrToIntInst(const llvm::PtrToIntInst *PTII);
};

class PathConstraint {
public:
  typedef std::pair<const llvm::BasicBlock *, const llvm::BasicBlock *> Edge;
  typedef llvm::SmallVectorImpl<Edge> EdgeVector;

  PathConstraint(z3::context &CTX, ValueConstraint &VC, EdgeVector &BackEdges)
      : CTX(CTX), VC(VC), BackEdges(BackEdges){};
  z3::expr Get(const llvm::BasicBlock *BB);

private:
  z3::context &CTX;
  ValueConstraint &VC;
  const EdgeVector &BackEdges;  // How many loops can a function have?
  z3::expr EdgeCondition(const llvm::BasicBlock *BB,
                         const llvm::BasicBlock *Pred);
  z3::expr PhiAssignment(const llvm::BasicBlock *BB,
                         const llvm::BasicBlock *Pred);
  Edge e;
};

#endif
