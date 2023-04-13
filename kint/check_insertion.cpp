#include "check_insertion.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FormatVariadic.h"
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
  static bool isObservable(const Instruction &Inst);
  static bool isDirectlyObservable(const Instruction &Inst);
};
} // anonymous namespace

template <class T> static ConstantInt *getAnyInt(LLVMContext &CTX, T V) {
  static_assert(std::is_enum<T>::value || std::is_integral<T>::value,
                "Integral Required.");
  // LLVM IR cmp returns int1; c++ bool converted to int 1
  const unsigned BitWidth = std::is_same<T, bool>::value ? 1 : sizeof(T) * 8;
  auto Ty = Type::getIntNTy(CTX, BitWidth);
  auto IsSigned = std::numeric_limits<T>::is_signed;

  return ConstantInt::get(Ty, V, IsSigned);
}

static SmallVector<Type *> getTypes(const ArrayRef<Value *> V) {
  SmallVector<Type *> Ty;
  for (auto E : V) {
    Ty.push_back(E->getType());
  }
  return Ty;
}

static FunctionCallee declareVoidFn(Module *M, StringRef Name,
                                    ArrayRef<Type *> Params) {
  auto VoidTy = Type::getVoidTy(M->getContext());
  auto FnTy = FunctionType::get(VoidTy, Params, false);
  auto Fn = M->getOrInsertFunction(Name, FnTy);

  return Fn;
}

void BinaryOperatorVisitor::visitBinaryOperator(BinaryOperator &BO) {
  if (!isObservable(BO)) {
    return;
  }
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
    insertShiftCheck(BO);
  }
  outs() << BO.getName() << " BO found! "
         << "\n";
}

void BinaryOperatorVisitor::insertOverflowCheck(BinaryOperator &BO) {
  // do insertion right before instruction
  auto M = BO.getModule();
  auto &CTX = M->getContext();
  auto Ty = cast<IntegerType>(BO.getType());

  // argument list:
  // opcode, nsw, arg0, arg1
  // unsigned wrap should always be enabled in C semantics
  Value *Arg[] = {
      getAnyInt(CTX, BO.getOpcode()),
      getAnyInt(CTX, BO.hasNoSignedWrap()),
      BO.getOperand(0),
      BO.getOperand(1),
  };

  auto FnName = formatv("__kint_overflow.{0}", Ty->getBitWidth()).str();
  auto Fn = declareVoidFn(M, FnName, getTypes(Arg));

  auto Builder = IRBuilder(&BO);

  Builder.CreateCall(Fn, Arg);
  // TODO: are operands constant? can we get their values?
  // TODO: can we use existing checks, i.e *MayOverflow/computeOverflowFor*
}

void BinaryOperatorVisitor::insertDivCheck(BinaryOperator &BO) {
  auto M = BO.getModule();
  auto &CTX = M->getContext();
  auto Ty = cast<IntegerType>(BO.getType());

  auto Fn = declareVoidFn(M, "__kint_check", {Type::getInt1Ty(CTX)});

  auto Builder = IRBuilder(&BO);
  auto Numerator = BO.getOperand(0);
  auto Denominator = BO.getOperand(1);

  // UB: div by 0
  auto Zero = ConstantInt::getNullValue(Ty);
  auto Check = Builder.CreateICmpEQ(Denominator, Zero, "DivZero");

  if (BO.getOpcode() == BinaryOperator::SDiv ||
      BO.getOpcode() == BinaryOperator::SRem) {
    // UB: MIN/-1 overflow
    auto NegOne = Constant::getAllOnesValue(Ty);
    auto DenominatorIsNegOne = Builder.CreateICmpEQ(Denominator, NegOne);

    auto Min =
        ConstantInt::get(Ty, APInt::getSignedMinValue(Ty->getBitWidth()));
    auto NumeratorIsMin = Builder.CreateICmpEQ(Numerator, Min);

    auto Overflow =
        Builder.CreateAnd(DenominatorIsNegOne, NumeratorIsMin, "DivOverflow");
    Check = Builder.CreateOr(Check, Overflow);
  }

  Builder.CreateCall(Fn, {Check});
}

void BinaryOperatorVisitor::insertShiftCheck(BinaryOperator &BO) {
  auto M = BO.getModule();
  auto &CTX = M->getContext();
  auto Ty = cast<IntegerType>(BO.getType());

  auto Fn = declareVoidFn(M, "__kint_check", {Type::getInt1Ty(CTX)});

  auto Builder = IRBuilder(&BO);
  auto Amount = BO.getOperand(1);
  // At the IR level, shifting by negative amount is just shifting >= N bits
  auto Check = Builder.CreateICmpUGE(
      Amount, ConstantInt::get(Ty, Ty->getBitWidth()), "ShiftOverflow");

  Builder.CreateCall(Fn, {Check});
}

bool BinaryOperatorVisitor::isDirectlyObservable(const Instruction &Inst) {
  // The original KINT implementation also considers br out of loop as
  // observable and other br as unobservable.

  // check store, ret, phi
  // check call, load, div/rem with potential side-effects, including UB
  return !isSafeToSpeculativelyExecute(&Inst);
}

bool BinaryOperatorVisitor::isObservable(const Instruction &Inst) {
  // TODO: consider changing to getObservationPoints()
  SmallPtrSet<const Value *, 16> Visited;
  SmallVector<const Value *> Worklist{&Inst};

  while (!Worklist.empty()) {
    auto Curr = Worklist.pop_back_val();
    if (Visited.contains(Curr)) {
      continue;
    }
    Visited.insert(Curr);

    auto CurrInst = dyn_cast<const Instruction>(Curr);
    if (!CurrInst) {
      errs() << "KINT warn: User is not an instruction!\n";
      continue;
    }
    if (isDirectlyObservable(*CurrInst)) {
      outs() << Inst.getName() << ".isObservable: true\n";
      return true;
    }

    auto Users = Curr->users();
    Worklist.append(Users.begin(), Users.end());
  }
  outs() << Inst.getName() << ".isObservable: false\n";
  return false;
}

PreservedAnalyses CheckInsertionPass::run(Function &F,
                                          FunctionAnalysisManager &) {
  BinaryOperatorVisitor BOV;
  BOV.visit(F);

  // TODO: Injecting calls may affect analyses. Consider using analysis pass.
  return PreservedAnalyses::none();
}
