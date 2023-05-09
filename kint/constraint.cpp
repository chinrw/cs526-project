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

static z3::expr ConvertInt(z3::context &CTX, const APInt &I) {
  SmallVector<char> Str;
  I.toStringUnsigned(Str);
  return CTX.bv_val(Str.begin(), I.getBitWidth());
}

bool ValueConstraint::isAnalyzable(const Value *V) {
  auto Ty = V->getType();
  return Ty->isIntegerTy() || Ty->isPointerTy() || Ty->isFunctionTy();
}

z3::expr ValueConstraint::SymbolFor(const Value *V) {
  return CTX.bv_const(PtrToString(V), DL.getTypeSizeInBits(V->getType()));
}

z3::expr PathConstraint::Get(const BasicBlock *BB) {}

z3::expr ValueConstraint::Get(const Value *V) {
  // Precondition: Type of V isAnalyzable, i.e. no floats, vectors, etc.
  if (auto C = dyn_cast<Constant>(V)) {
    return GetConstant(C);
  } else if (auto BO = dyn_cast<BinaryOperator>(V)) {
    return GetBinaryOperator(BO);
  } else if (auto ICI = dyn_cast<ICmpInst>(V)) {
    return GetICmpInst(ICI);
  } else if (auto GEPI = dyn_cast<GetElementPtrInst>(V)) {
    return GetGEPOperator(cast<GEPOperator>(GEPI));
  } else if (auto TI = dyn_cast<TruncInst>(V)) {
    return GetTruncInst(TI);
  } else if (auto ZEI = dyn_cast<ZExtInst>(V)) {
    return GetZextInst(ZEI);
  } else if (auto SEI = dyn_cast<SExtInst>(V)) {
    return GetSExtInst(SEI);
  } else if (auto SI = dyn_cast<SelectInst>(V)) {
    return GetSelectInst(SI);
  } else if (auto BCI = dyn_cast<BitCastInst>(V)) {
    return GetBitCastInst(BCI);
  } else if (auto ITPI = dyn_cast<IntToPtrInst>(V)) {
    return GetIntToPtrInst(ITPI);
  } else if (auto PTII = dyn_cast<PtrToIntInst>(V)) {
    return GetPtrToIntInst(PTII);
  } else {
    // fallback, i.e. load, extractvalue, etc.
    errs() << "Unhandled Operator " << V->getNameOrAsOperand() << ", " << __FILE__ << ":"
           << __LINE__ << "\n";
    return SymbolFor(V);
  }
}

z3::expr ValueConstraint::GetTruncInst(const TruncInst *TI) {
  auto Src = TI->getOperand(0);
  // For integer types, DL.getTypeSizeInBits()
  // just wraps around getIntegerBitWidth()
  auto DestWidth = DL.getTypeSizeInBits(TI->getDestTy());
  return Get(Src).extract(DestWidth - 1, 0);
}

z3::expr ValueConstraint::GetZextInst(const ZExtInst *ZEI) {
  auto Src = Get(ZEI->getOperand(0));
  auto SrcWidth = DL.getTypeSizeInBits(ZEI->getSrcTy());
  auto DestWidth = DL.getTypeSizeInBits(ZEI->getDestTy());
  return z3::zext(Src, DestWidth - SrcWidth);
}

z3::expr ValueConstraint::GetSExtInst(const SExtInst *SEI) {
  auto Src = Get(SEI->getOperand(0));
  auto SrcWidth = DL.getTypeSizeInBits(SEI->getSrcTy());
  auto DestWidth = DL.getTypeSizeInBits(SEI->getDestTy());
  return z3::sext(Src, DestWidth - SrcWidth);
}

z3::expr ValueConstraint::GetSelectInst(const SelectInst *SI) {
  auto Condition = Get(SI->getCondition());
  auto TrueValue = Get(SI->getTrueValue());
  auto FalseValue = Get(SI->getTrueValue());
  return z3::ite(Condition, TrueValue, FalseValue);
}

z3::expr ValueConstraint::GetBitCastInst(const BitCastInst *BCI) {
  // May be float
  auto Src = BCI->getOperand(0);
  return  isAnalyzable(Src) ? Get(Src) : SymbolFor(Src);
}

static z3::expr SResize(const z3::expr& BV, unsigned SrcWidth, unsigned DestWidth) {
  if (SrcWidth == DestWidth) {
    return BV;
  } else if (SrcWidth < DestWidth) {
    return z3::sext(BV, DestWidth - SrcWidth);
  } else {
    return BV.extract(DestWidth - 1, 0);
  }
}

z3::expr ValueConstraint::GetIntToPtrInst(const IntToPtrInst *CI) {
  auto SrcWidth = DL.getTypeSizeInBits(CI->getSrcTy());
  auto DestWidth = DL.getTypeSizeInBits(CI->getDestTy());
  return SResize(Get(CI->getOperand(0)), SrcWidth, DestWidth);
}

z3::expr ValueConstraint::GetPtrToIntInst(const PtrToIntInst *CI) {
  auto SrcWidth = DL.getTypeSizeInBits(CI->getSrcTy());
  auto DestWidth = DL.getTypeSizeInBits(CI->getDestTy());
  return SResize(Get(CI->getOperand(0)), SrcWidth, DestWidth);
}

/*
Possible future work: https://llvm.org/docs/GetElementPtr.html
"There is currently no checker for the getelementptr rules... It would be
possible to write a static checker... However, no such checker exists today."
*/
z3::expr ValueConstraint::GetGEPOperator(const GEPOperator *GEPO) {
  unsigned BitWidth = DL.getIndexTypeSizeInBits(GEPO->getType());
  MapVector<Value *, APInt> VariableOffsets;
  APInt ConstantOffset(BitWidth, 0);
  if (!GEPO->collectOffset(DL, BitWidth, VariableOffsets, ConstantOffset)) {
    // fallback
    errs() << "CollectOffset failed! " << __FILE__ << ":" << __LINE__ << "\n";
    return SymbolFor(GEPO);
  }
  auto Result = ConvertInt(CTX, ConstantOffset);
  for (const auto &[V, Offset] : VariableOffsets) {
    Result = Result + ConvertInt(CTX, Offset) * Get(V);
  }
  return Result;
}

z3::expr ValueConstraint::GetICmpInst(const ICmpInst *ICI) {
  auto LHS = Get(ICI->getOperand(0));
  auto RHS = Get(ICI->getOperand(1));

  switch (ICI->getPredicate()) {
  default:
    llvm_unreachable("Unknown ICmpInst!");
  case CmpInst::ICMP_EQ:
    return LHS == RHS;
  case CmpInst::ICMP_NE:
    return LHS != RHS;
  case CmpInst::ICMP_UGT:
    return z3::ugt(LHS, RHS);
  case CmpInst::ICMP_UGE:
    return z3::uge(LHS, RHS);
  case CmpInst::ICMP_ULT:
    return z3::ult(LHS, RHS);
  case CmpInst::ICMP_ULE:
    return z3::ule(LHS, RHS);
  case CmpInst::ICMP_SGT:
    return LHS > RHS;
  case CmpInst::ICMP_SGE:
    return LHS >= RHS;
  case CmpInst::ICMP_SLT:
    return LHS < RHS;
  case CmpInst::ICMP_SLE:
    return LHS <= RHS;
  }
}

z3::expr ValueConstraint::GetBinaryOperator(const BinaryOperator *BO) {
  auto LHS = Get(BO->getOperand(0));
  auto RHS = Get(BO->getOperand(1));

  switch (BO->getOpcode()) {
  default:
    assert(BO->getType()->isIntegerTy() && "Non Integer Operator!");
    llvm_unreachable("Unknown BinaryOperator!");
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
    return ConvertInt(CTX, CI->getValue());
  } else if (auto CPN = dyn_cast<ConstantPointerNull>(C)) {
    return CTX.bv_val(0, DL.getPointerTypeSizeInBits(CPN->getType()));
  } else if (auto GEPO = dyn_cast<GEPOperator>(C)) {
    return GetGEPOperator(GEPO);
  }
  // Not sure why other constexprs are ignored
  return SymbolFor(C);
}
