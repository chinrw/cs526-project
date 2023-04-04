#include "check_insertion.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>

using namespace llvm;

namespace {
  class BinaryOperatorVisitor : public InstVisitor<BinaryOperatorVisitor> {
  public:
    BinaryOperatorVisitor() {}
    void visitBinaryOperator(BinaryOperator &BO);
  private:
    // insert check before the instruction if needed
    void insertOverflowCheck(BinaryOperator &BO);
    void insertDivCheck(BinaryOperator &BO);
    void insertShiftCheck(BinaryOperator &BO);
    bool isObservable(const Instruction &Inst);
  };
} // anonymous namespace

template <class T>
static ConstantInt * getAnyInt(LLVMContext &CTX, T V) {
  static_assert(std::is_enum<T>::value || std::is_integral<T>::value, "Integral Required.");
  // LLVM IR cmp returns int1; c++ bool converted to int 1
  auto Ty = Type::getIntNTy(CTX, std::is_same<T, bool>::value ? 1 : sizeof(T) * 8);
  auto IsSigned = std::numeric_limits<T>::is_signed;

  return ConstantInt::get(Ty, V, IsSigned);
}

SmallVector<Type*> getTypes(const ArrayRef<Value*> V) {
  SmallVector<Type*> Ty;
  for (auto E : V) {
    Ty.push_back(E->getType());
  }
  return Ty;
}

void BinaryOperatorVisitor::visitBinaryOperator(BinaryOperator &BO) {
  switch (BO.getOpcode()) {
  default:
    break;
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
    //?? handle pointer
    insertOverflowCheck(BO);
    break;
  case Instruction::UDiv:
  case Instruction::URem: //?? not in paper
  case Instruction::SDiv:
  case Instruction::SRem: //?? not in paper
    insertDivCheck(BO);
    break;
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    break;
  }
  outs() << BO.getName() << " BO found! " << "\n";
}

void BinaryOperatorVisitor::insertOverflowCheck(BinaryOperator &BO) {
  // do insertion right before instruction
  auto M = BO.getModule();
  auto &CTX = M->getContext();

  // argument list:
  // opcode, nsw, arg0, arg1
  // unsigned wrap should always be enabled in C semantics
  Value* Arg[] = {
    getAnyInt(CTX, BO.getOpcode()),
    getAnyInt(CTX, BO.hasNoSignedWrap()),
    BO.getOperand(0),
    BO.getOperand(1),
  };
  auto FnTy = FunctionType::get(Type::getVoidTy(CTX), getTypes(Arg), false);
  auto Fn = M->getOrInsertFunction("__kint_overflow", FnTy);
  assert(Fn.getFunctionType() == FnTy && "__kint_overflow has conflicting types!");

  auto Builder = IRBuilder(&BO);
  Builder.CreateCall(Fn, Arg);
  // TODO: are operands constant? can we get their values?
  // TODO: can we use existing checks, i.e *MayOverflow/computeOverflowFor*
}

void BinaryOperatorVisitor::insertDivCheck(BinaryOperator &BO) {
  auto M = BO.getModule();
  auto &CTX = M->getContext();

  auto FnTy = FunctionType::get(Type::getVoidTy(CTX), {Type::getInt1Ty(CTX)}, false);
  auto Fn = M->getOrInsertFunction("__kint_check", FnTy);
  assert(Fn.getFunctionType() == FnTy && "__kint_check has conflicting types!");
  
  auto Builder = IRBuilder(&BO);
  auto Dividend = BO.getOperand(0);
  auto Divisor = BO.getOperand(1);
  auto OperandTy = Dividend->getType();

  // UB: div by 0
  auto Zero = ConstantInt::getNullValue(OperandTy);
  auto Check = Builder.CreateICmpEQ(Divisor, Zero, "Div0");

  if (BO.getOpcode() == BinaryOperator::SDiv ||
      BO.getOpcode() == BinaryOperator::SRem) {
    // UB: MIN/-1 overflow
    auto NegOne = Constant::getAllOnesValue(OperandTy);
    auto DivisorIsNegOne = Builder.CreateICmpEQ(Divisor, NegOne);

    auto Min = ConstantInt::get(OperandTy, APInt::getSignedMinValue(OperandTy->getIntegerBitWidth()));
    auto DividendIsMin = Builder.CreateICmpEQ(Dividend, Min);

    auto Overflow = Builder.CreateAnd(DivisorIsNegOne, DividendIsMin, "DivOverflow");
    Check = Builder.CreateOr(Check, Overflow);
  }

  Builder.CreateCall(Fn, {Check});
}

void BinaryOperatorVisitor::insertShiftCheck(BinaryOperator &BO) {

}

bool BinaryOperatorVisitor::isObservable(const Instruction &Inst) {
  // TODO
  return true;
}

PreservedAnalyses CheckInsertionPass::run(Function &F,
                                          FunctionAnalysisManager &) {
  // Instruction::Add;
  BinaryOperatorVisitor BOV;
  BOV.visit(F);

  return PreservedAnalyses::all();
}
