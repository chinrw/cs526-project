#include "check_insertion.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstVisitor.h"
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
  auto Ty = Type::getIntNTy(CTX, sizeof(T) * 8);
  auto IsSigned = std::numeric_limits<T>::is_signed;

  return ConstantInt::get(Ty, V, IsSigned);
}

std::vector<Type*> getTypes(ArrayRef<Value*> V) {
  std::vector<Type*> Ty;
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
  case Instruction::SDiv:
  case Instruction::UDiv:
  case Instruction::SRem: //?? not in paper
  case Instruction::URem: //?? not in paper
    break;
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    break;
  }
  outs() << BO.getName() << " BO found! " << "\n";
}

void BinaryOperatorVisitor::insertOverflowCheck(BinaryOperator &BO) {
  // should 
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
  assert(Fn.getFunctionType() == FnTy && "__kint_overflow type differ!");
  auto CI = CallInst::Create(Fn, Arg, "", &BO);
}

void BinaryOperatorVisitor::insertDivCheck(BinaryOperator &BO) {

}

void BinaryOperatorVisitor::insertShiftCheck(BinaryOperator &BO) {

}

bool BinaryOperatorVisitor::isObservable(const Instruction &Inst) {
  // todo
  return true;
}

PreservedAnalyses CheckInsertionPass::run(Function &F,
                                          FunctionAnalysisManager &) {
  // Instruction::Add;
  BinaryOperatorVisitor BOV;
  BOV.visit(F);

  return PreservedAnalyses::all();
}
