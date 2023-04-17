#include "kint_module.h"
#include "range.h"

#include "llvm/IR/ConstantRange.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

void KintRangeAnalysisPass::initGlobalVariables(Module &M) {
  for (const auto &G : M.globals()) {

    if (G.getValueType()->isIntegerTy()) {
      if (auto *C = dyn_cast<ConstantInt>(G.getInitializer())) {
        DEBUG_WITH_TYPE("range", dbgs() << "Global variable " << G.getName()
                                        << " has initializer " << *C << "\n");
        // get range from initializer

      } else {
        DEBUG_WITH_TYPE("range", dbgs() << "Global variable " << G.getName()
                                        << " has no initializer "
                                        << "\n");
        // get full range
        globalValueRangeMap[&G] =
            KintConstantRange::getFull(G.getValueType()->getIntegerBitWidth());
      }
    } else if (auto *VT = dyn_cast<ArrayType>(G.getValueType())) {
      // TODO handle more complex array types (2D array)
      // integer array
      if (VT->getElementType()->isIntegerTy()) {
        if (auto *C = dyn_cast<ConstantDataArray>(G.getInitializer())) {
          // get range from initializer
          DEBUG_WITH_TYPE("range", dbgs() << "Global variable " << G.getName()
                                          << " has initializer " << *C << "\n");
          // get const range from initializer
          for (unsigned i = 0; i < C->getNumElements(); i++) {
            auto CI = dyn_cast<ConstantInt>(C->getElementAsConstant(i));
            DEBUG_WITH_TYPE("range", dbgs()
                                         << "Global variable " << G.getName()
                                         << " has initializer " << *CI << "\n");
            globalValueRangeMap[&G] = KintConstantRange(CI->getValue());
          }
        } else if (auto *C =
                       dyn_cast<ConstantAggregateZero>(G.getInitializer())) {
          // get zero
          for (unsigned i = 0; i < C->getElementCount().getFixedValue(); i++) {
            globalValueRangeMap[&G] = KintConstantRange(APInt::getZero(
                C->getElementValue(i)->getType()->getIntegerBitWidth()));
          }

          DEBUG_WITH_TYPE("range", dbgs() << "Global variable " << G.getName()
                                          << " has initializer " << *C << "\n");
        } else {
          outs() << "Global variable " << G.getName() << " has no initializer "
                 << "\n";
          // get full range
          for (unsigned i = 0; i < VT->getNumElements(); i++) {
            globalValueRangeMap[&G] = KintConstantRange::getFull(
                VT->getElementType()->getIntegerBitWidth());
          }
        }
      } else if (VT->getElementType()->isPointerTy()) {
        // TODO handle pointer array
				// Maybe use recursive function to handle more complex array types
      } else {
        // TODO handle other types
      }

    } else if (G.getValueType()->isStructTy()) {
      // TODO handle struct types
    } else if (G.getValueType()->isPointerTy()) {
      // TODO handle Pointer types
    } else {
      // TODO handle other types
    }
  }
}

void KintRangeAnalysisPass::initRange(Module &M) {

  // init global variables
  this->initGlobalVariables(M);
}
