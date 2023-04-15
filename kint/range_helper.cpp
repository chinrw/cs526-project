#include "range.h"
#include <cstddef>
#include <cstdint>
#include <llvm-16/llvm/IR/ConstantRange.h>
#include <llvm-16/llvm/IR/GlobalVariable.h>
#include <llvm-16/llvm/IR/Instructions.h>
#include <llvm-16/llvm/IR/ValueMap.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <sys/types.h>

using namespace llvm;
//Initialize the set to record the out-of-bound GEP instructions

ConstantRange handleSelectInst(SelectInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found select instruction: " << operand->getOpcodeName() << "\n";
    // Add a full range for the current instruction to the global range map
    globalRangeMap.emplace(&I, ConstantRange::getFull(operand->getType()->getIntegerBitWidth()));
    // Get the true and false values from the SelectInst
    ConstantRange trueRange = globalRangeMap.at(operand->getTrueValue());
    ConstantRange falseRange = globalRangeMap.at(operand->getFalseValue());
    // Compute the new range by taking the union of the true and false value ranges
    ConstantRange outputRange = trueRange.unionWith(falseRange);
    // Update the global range map with the computed range
    return outputRange;
}

ConstantRange handleCastInst(CastInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found cast instruction: " << operand->getOpcodeName() << "\n";
    ConstantRange outputRange = ConstantRange::getFull(operand->getType()->getIntegerBitWidth());
    globalRangeMap.emplace(
        &I, ConstantRange::getFull(I.getType()->getIntegerBitWidth()));
    // Get the input operand of the cast instruction
    auto inp = operand->getOperand(0);
    // If the input operand is not an integer type, set the range to the full bit width
    if(!inp->getType()->isIntegerTy()) {
        outputRange = ConstantRange(operand->getType()->getIntegerBitWidth(), true);
    } else {
        // If the input is an integer type, compute the output range based on the cast operation
        auto inpRange = globalRangeMap.at(inp);
        const uint32_t bits = operand->getType()->getIntegerBitWidth();
        switch (operand->getOpcode()) {
        case CastInst::Trunc:
            outputRange = inpRange.truncate(bits);
            break;
        case CastInst::ZExt:
            outputRange = inpRange.zeroExtend(bits);
            break;
        case CastInst::SExt:
            outputRange = inpRange.signExtend(bits);
            break;
        default:
            outputRange = inpRange;
        }
    }
    return outputRange;
}

ConstantRange handlePHINode(PHINode *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found phi node: " << operand->getOpcodeName() << "\n";
    ConstantRange outputRange = ConstantRange::getEmpty(operand->getType()->getIntegerBitWidth());
    globalRangeMap.emplace(operand, outputRange);
    // Iterate through the incoming values of the PHI node
    for (unsigned i = 0; i < operand->getNumIncomingValues(); i++) {
        // Get the range of the incoming value
        ConstantRange incomingRange = globalRangeMap.at(operand->getIncomingValue(i));
        // Union the incoming range with the PHI node range
        outputRange = outputRange.unionWith(incomingRange);
        }
    // Update the global range map with the computed range
    return outputRange;
}

ConstantRange handleLoadInst(LoadInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found load instruction: " << operand->getOpcodeName() << "\n";
        globalRangeMap.emplace(
          &I, ConstantRange::getFull(I.getType()->getIntegerBitWidth()));
        // Get the address loaded by the LoadInst
        auto addr = operand->getPointerOperand();
        // Initialize an empty range for the LoadInst
        ConstantRange loadRange = ConstantRange::getEmpty(I.getType()->getIntegerBitWidth());
        // Check if the address is a global variable
        if (auto *global = dyn_cast_or_null<GlobalVariable>(addr)) {
          // If the global variable has an initializer, get the range of the initializer
          if (auto *init = dyn_cast_or_null<ConstantInt>(global->getInitializer())) {
            loadRange = ConstantRange(init->getValue());
          }
        } 
}