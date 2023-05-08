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
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
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
    if (backEdges[curBB].contains(predBB)) {
        return;
    }

    auto currentBlockRange = functionsToRangeInfo[curBB->getParent()];

    // If there is a predecessor block
    if (nullptr != predBB) {
        if (auto terminator = predBB->getTerminator(); auto branch = dyn_cast<BranchInst>(terminator)) {
            if (branch->isConditional()) {
                if (auto cmp = dyn_cast<ICmpInst>(branch->getCondition())) {
                    auto lhs = cmp->getOperand(0);
                    auto rhs = cmp->getOperand(1);

                    if (!lhs->getType()->isIntegerTy() || !rhs->getType()->isIntegerTy()) {
                        outs() << "The branch operands are not integers" << *cmp << "\n";
                    } else {
                        bool isTrueBranch = branch->getSuccessor(0) == curBB;

                        if (impossibleBranches.find(cmp) != impossibleBranches.end() && impossibleBranches[cmp] == isTrueBranch) {
                            return;
                        }

                        const auto getTrueBranch = [lhs, rhs, cmp, this]() {
                            switch (cmp->getPredicate()) {
                            case ICmpInst::ICMP_EQ:
                                return ValuetoSymbolicVar(lhs) == ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_NE:
                                return ValuetoSymbolicVar(lhs) != ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_UGT:
                                return ValuetoSymbolicVar(lhs) > ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_UGE:
                                return ValuetoSymbolicVar(lhs) >= ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_ULT:
                                return ValuetoSymbolicVar(lhs) < ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_ULE:
                                return ValuetoSymbolicVar(lhs) <= ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_SGT:
                                return ValuetoSymbolicVar(lhs) > ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_SGE:
                                return ValuetoSymbolicVar(lhs) >= ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_SLT:
                                return ValuetoSymbolicVar(lhs) < ValuetoSymbolicVar(rhs);
                            case ICmpInst::ICMP_SLE:
                                return ValuetoSymbolicVar(lhs) <= ValuetoSymbolicVar(rhs);
                            default:
                                outs() << "Unhandled predicate: " << cmp->getPredicate() << "\n";
                                break;
                            }
                        };

                        const auto check = [cmp, isTrueBranch, this] {
                            if (Solver.value().check() == z3::unsat) {
                                outs() << "[SMT solving] cannot continue" << (isTrueBranch ? "true" : "false") << " branch of " << *cmp << "\n";
                                return false;
                            }
                            return true;
                        };

                        if (isTrueBranch) {
                            Solver.value().add(getTrueBranch());
                            if(!check()) {
                                return;
                            }
                            argValuetoSymbolicVar[cmp] = Solver.value().ctx().bv_val(true, 1);
                        } else {
                            Solver.value().add(!getTrueBranch());
                            if(!check()) {
                                return;
                            }
                            argValuetoSymbolicVar[cmp] = Solver.value().ctx().bv_val(false, 1);
                        }
                    }
                }
            }
        } else if (auto switchInst = dyn_cast<SwitchInst>(terminator)) {
            auto cond = switchInst ->getCondition();
            if(cond->getType()->isIntegerTy()) {
                auto condRange = getRangeByBB(cond, predBB);
                auto emptyRange = KintConstantRange(cond->getType()->getIntegerBitWidth(), false);

                if(switchInst->getDefaultDest() == curBB) {
                    for(auto c : switchInst->cases()) {
                        auto caseVal = c.getCaseValue();
                        Solver.value().add(ValuetoSymbolicVar(cond) != Solver.value().ctx().bv_val(caseVal->getZExtValue(), cond->getType()->getIntegerBitWidth()));
                    }
                } else {
                    for (auto c : switchInst->cases()) {
                        if(c.getCaseSuccessor() == curBB) {
                            auto caseVal = c.getCaseValue();
                            Solver.value().add(ValuetoSymbolicVar(cond) == Solver.value().ctx().bv_val(c.getCaseValue()->getZExtValue(), cond->getType()->getIntegerBitWidth()));
                            break;
                        }
                    }
                }
            }
        } else {
            errs() << "Unknown terminator instruction: " << *predBB->getTerminator() << "\n";
        }
    }

    for (auto &Inst : *curBB) {
        if (!currentBlockRange.count(&Inst) || !Inst.getType()->isIntegerTy()) {
            continue;
        }

        if (auto OP = dyn_cast<BinaryOperator>(&Inst)) {
        // TODO: Check if the binary operation is valid
        // TODO: Propagate the range information
        // TODO: Check if the range constraint can be added to the solver
            if (!addRangeConstaints(getRangeByBB(&Inst, Inst.getParent()), ValuetoSymbolicVar(OP)))
                return;
        } else if (auto OP = dyn_cast<CastInst>(&Inst)) {
            //TODO: If the instruction is a cast, propagate the range information
            if (!addRangeConstaints(getRangeByBB(&Inst, Inst.getParent()), ValuetoSymbolicVar(OP)))
                return;
        } else {
            const auto name = "\%vid" + std::to_string(Inst.getValueID());
            argValuetoSymbolicVar[&Inst] = Solver.value().ctx().bv_const(name.c_str(), Inst.getType()->getIntegerBitWidth());
            if (!addRangeConstaints(getRangeByBB(&Inst, Inst.getParent()), ValuetoSymbolicVar(&Inst)))
                return;
        }
    }

    for (auto success : BBpathMap[curBB]) {
        Solver.value().push();
        pathSolver(success, curBB);
        Solver.value().pop();
    }
}