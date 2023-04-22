#ifndef KINT_CONSTRAINT_H
#define KINT_CONSTRAINT_H

#include "z3++.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Value.h"

class PathConstraint {
  z3::context &CTX;
  const ValueConstraint &VC;

public:
  PathConstraint(z3::context &CTX, const ValueConstraint &VC)
      : CTX(CTX), VC(VC){};
  z3::expr Get(const llvm::BasicBlock *BB);
};

class ValueConstraint {
  z3::context &CTX;
  const llvm::DataLayout &DL;
  z3::expr SymbolFor(const Value *V);
  z3::expr GetConstant(const llvm::Constant *C);
  z3::expr ValueConstraint::GetBinaryOperator(const BinaryOperator *BO);

public:
  ValueConstraint(z3::context &CTX, const llvm::DataLayout &DL)
      : CTX(CTX), DL(DL) {}
  z3::expr Get(const llvm::Value *V);
};

#endif
