#include "constraint.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Type.h>
#include <string>

using namespace llvm;

static const char *PtrToString(const Value *I) {
  std::string name;
  raw_string_ostream(name) << I;
  return name.c_str();
}

z3::expr ValueConstraint::SymbolFor(const Value *V) {
  CTX.bv_const(PtrToString(V), DL.getTypeSizeInBits(V->getType()));
}

z3::expr PathConstraint::Get(const BasicBlock *BB) {}

z3::expr ValueConstraint::Get(const Value *V) {
  if (auto C = dyn_cast<Constant>(V)) {
    return GetConstant(C);
  } else if (auto BO = dyn_cast<BinaryOperator>(V)) {
    return GetBinaryOperator(BO);
  } else if (auto UO = dyn_cast<UnaryOperator>(V) ) {

  }
}

z3::expr ValueConstraint::GetBinaryOperator(const BinaryOperator *BO) {
  auto LHS = Get(BO->getOperand(0));
  auto RHS = Get(BO->getOperand(1));

  switch (BO->getOpcode()) {
  default:
    assert(BO->getType()->isIntegerTy() && "Non Integer Operator!");
    assert(false && "Unreachable!");
  case Instruction::Add:
    return LHS + RHS;
  case Instruction::Sub:
    return LHS - RHS;
  case Instruction::Mul:
    return LHS * RHS;
  case Instruction::UDiv:
    return z3::udiv(LHS, RHS);
  case Instruction::SDiv:
    return LHS / RHS;
  case Instruction::URem:
    return z3::urem(LHS, RHS);
  case Instruction::SRem:
    return z3::srem(LHS, RHS);
  case Instruction::Shl:
    return z3::shl(LHS, RHS);
  case Instruction::LShr:
    return z3::lshr(LHS, RHS);
  case Instruction::AShr:
    return z3::ashr(LHS, RHS);
  case Instruction::And:
    return LHS && RHS;
  case Instruction::Or:
    return LHS || RHS;
  case Instruction::Xor:
    return LHS ^ RHS;
  }
}

z3::expr ValueConstraint::GetConstant(const Constant *C) {
  if (auto CI = dyn_cast<ConstantInt>(C)) {
    SmallVector<char> Str;
    CI->getValue().toStringUnsigned(Str);
    return CTX.bv_val(Str.begin(), CI->getBitWidth());

  } else if (isa<ConstantPointerNull>(C)) {
    return CTX.bv_val(0, DL.getPointerSizeInBits());

  } else if (auto GEP = dyn_cast<GEPOperator>(C)) {
    // TODO: GEP constant expr
    return SymbolFor(C);
  }
  return SymbolFor(C);
}