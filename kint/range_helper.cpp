#include "range.h"
#include <cstddef>
#include <cstdint>
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include <sys/types.h>

using namespace llvm;
//Initialize the set to record the out-of-bound GEP instructions

KintConstantRange handleSelectInst(SelectInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found select instruction: " << operand->getOpcodeName() << "\n";
    // Add a full range for the current instruction to the global range map
    globalRangeMap.emplace(&I, KintConstantRange::getFull(operand->getType()->getIntegerBitWidth()));
    // Get the true and false values from the SelectInst
    KintConstantRange trueRange = globalRangeMap.at(operand->getTrueValue());
    KintConstantRange falseRange = globalRangeMap.at(operand->getFalseValue());
    // Compute the new range by taking the union of the true and false value ranges
    KintConstantRange outputRange = trueRange.unionWith(falseRange);
    // Update the global range map with the computed range
    return outputRange;
}

KintConstantRange handleCastInst(CastInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found cast instruction: " << operand->getOpcodeName() << "\n";
    KintConstantRange outputRange = KintConstantRange::getFull(operand->getType()->getIntegerBitWidth());
    globalRangeMap.emplace(
        &I, KintConstantRange::getFull(I.getType()->getIntegerBitWidth()));
    // Get the input operand of the cast instruction
    auto inp = operand->getOperand(0);
    // If the input operand is not an integer type, set the range to the full bit width
    if(!inp->getType()->isIntegerTy()) {
        outputRange = KintConstantRange(operand->getType()->getIntegerBitWidth(), true);
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

KintConstantRange handlePHINode(PHINode *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found phi node: " << operand->getOpcodeName() << "\n";
    KintConstantRange outputRange = KintConstantRange::getEmpty(operand->getType()->getIntegerBitWidth());
    globalRangeMap.emplace(operand, outputRange);
    // Iterate through the incoming values of the PHI node
    for (unsigned i = 0; i < operand->getNumIncomingValues(); i++) {
        // Get the range of the incoming value
        KintConstantRange incomingRange = globalRangeMap.at(operand->getIncomingValue(i));
        // Union the incoming range with the PHI node range
        outputRange = outputRange.unionWith(incomingRange);
        }
    // Update the global range map with the computed range
    return outputRange;
}

KintConstantRange handleLoadInst(LoadInst *operand, RangeMap &globalRangeMap, Instruction &I) {
    outs() << "Found load instruction: " << operand->getOpcodeName() << "\n";
    KintConstantRange outputRange = KintConstantRange::getFull(operand->getType()->getIntegerBitWidth());
    globalRangeMap.emplace(operand, outputRange);
    // Get the pointer operand of the load instruction
    auto ptrAddr = operand->getPointerOperand();
    // If the address is a GlobalVariable, it retrieves the range associated with the global variable and assigns it to new_range.
    if (auto *GV = dyn_cast<GlobalVariable>(ptrAddr)) {
        if (GV->hasInitializer()) {
            if (auto *CI = dyn_cast<ConstantInt>(GV->getInitializer())) {
                outputRange = KintConstantRange(CI->getValue());
            }
        }
    } else if (auto gep = dyn_cast<GetElementPtrInst>(ptrAddr)) {
        KintRangeAnalysisPass rangeAnalysis;
        bool isInRange = false;
        auto gepAddr = gep->getPointerOperand();
        //If the base address is a GlobalVariable, it means we are accessing a global array
        if (auto *gepGV = dyn_cast<GlobalVariable>(gepAddr)) {
            // checks if the array is one-dimensional and if it has associated range information
            if (gep->getNumIndices() == 2 && globalRangeMap.count(gepGV)) {
                // retrieves the index being accessed in the array and calculates the 
                // size of the array and the range of the index.
                auto index = gep->getOperand(1);
                auto indexRange = globalRangeMap.at(index);
                auto arrayRange = globalRangeMap.at(gepGV);
                auto arraySize = arrayRange.getUpper().getLimitedValue();
                auto indexSize = indexRange.getUpper().getLimitedValue();
                // If the index is out of bounds, it inserts the GEP instruction into the gepOutOfRange set.
                if (indexSize >= arraySize) {
                    rangeAnalysis.gepOutOfRange.insert(gep);
                } 

                // iterates through the valid index range and calculates the union of the associated ranges,
                // and updateing the global range map.
                for (int i = indexRange.getLower().getLimitedValue(); i < indexSize; i++) {
                    auto indexRange = KintConstantRange(APInt(32, i));
                    auto arrayRange = globalRangeMap.at(gepGV);
                    auto newRange = arrayRange.unionWith(indexRange);
                    globalRangeMap.emplace(gep, newRange);
                }  
                isInRange = true;
            }
        }
        if(!isInRange) {
            // If the GEP operation was not successfully handled, a warning is printed,
            //  and global map range is set to the full range.
            outs() << "WARNING: GEP operation was not successfully handled: " << *gep << "\n";
            outputRange = KintConstantRange::getFull(operand->getType()->getIntegerBitWidth());
        }
    } else {
        // If the address is not a GlobalVariable, a warning is printed,
        // and global map range is set to the full range.
        outs() << "WARNING: Unknown address to load: " << *ptrAddr << "\n";
        outputRange = KintConstantRange::getFull(operand->getType()->getIntegerBitWidth());
    }
    return outputRange;
}