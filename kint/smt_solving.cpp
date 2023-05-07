#include "kint_constant_range.h"
#include "range.h"
#include "z3++.h"
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
#include "llvm/IR/Function.h"
#include "llvm/IR/Argument.h"
#include <llvm-16/llvm/ADT/APInt.h>
#include <llvm-16/llvm/IR/ConstantRange.h>
#include <llvm-16/llvm/IR/Instructions.h>
#include <llvm-16/llvm/Support/Casting.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/GlobalAlias.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <regex>
#include <string>
#include <sys/types.h>

using namespace llvm;
bool KintRangeAnalysisPass::addRangeConstaints(const KintConstantRange &range, const z3::expr &BV) {
    if(range.isFullSet() || BV.is_const()) {
        return true;
    }
    if(range.isEmptySet()) {
        errs() << "lhs is empty set\n";
        return false;
    }

    Solver.value().add(z3::ule(BV, Solver.value().ctx().bv_val(range.getUnsignedMax().getZExtValue(), range.getBitWidth())));
    Solver.value().add(z3::uge(BV, Solver.value().ctx().bv_val(range.getUnsignedMin().getZExtValue(), range.getBitWidth())));
    return true;
}


void KintRangeAnalysisPass::smtSolver(Module &M) { 
    for (auto F : taintedFunctions) {
        if(F->isDeclaration()) {
            continue;
        }
    
        for (auto &BB : *F) {
            for (const auto &prev : predecessors(&BB)) {
                auto itBBbackEdges = backEdges.find(&BB);
                if(itBBbackEdges != backEdges.end() || prev == &BB) {
                    continue;
                }
                backEdges[&BB].insert(prev);
            }
        }
    }
    // Solve Constraints
    for (auto F : taintedFunctions) {
        if(F->isDeclaration()) {
            continue;
        }
        Solver.value().push();
        // Add function arg constraints
        for(auto &arg : F->args()) {
            if(!arg.getType()->isIntegerTy()) {
                continue;
            }
            const auto argName = F->getName().str() + "_" + std::to_string(arg.getArgNo());
            const auto argv = Solver.value().ctx().bv_const(argName.c_str(), arg.getType()->getIntegerBitWidth());
            argValuetoSymbolicVar[&arg] = argv;
            addRangeConstaints(getRangeByBB(&arg, &(F->getEntryBlock())), argv);
        }

        pathSolver(&(F->getEntryBlock()), nullptr);
        Solver.value().pop();
    }
}

void KintRangeAnalysisPass::pathSolver(BasicBlock *curBB, BasicBlock *predBB) { 

}