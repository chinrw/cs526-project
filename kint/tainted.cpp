#include "kint_constant_range.h"
#include "range.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Demangle/Demangle.h"
#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Metadata.h>
#include <llvm/Support/raw_ostream.h>
using namespace llvm;
using namespace std;

constexpr StringRef IR_SINK_FUNC = "kint.sink";
constexpr StringRef IR_TAINT_FUNC = "kint.taint";
constexpr StringRef IR_TAINT_FUNC_ARG = "kint.taint.arg";

bool checkFuncSinked(StringRef funcName) {
  for (const auto &s : SINK_FUNCS) {
    if (funcName == s.first) {
      return true;
    }
  }
  return false;
}

vector<Function *> getSinkFuncs(Instruction *I) {
  vector<Function *> ret;
  for (Value *user : I->users()) {
    if (CallInst *call = dyn_cast<CallInst>(user)) {
      const char *mangled_name = call->getCalledFunction()->getName().data();
      StringRef demangled_name =
          itaniumDemangle(mangled_name, nullptr, nullptr, nullptr);
      if (checkFuncSinked(demangled_name)) {
        ret.push_back(call->getCalledFunction());
      }
    }
  }
  return ret;
}

static void setMetaDataSink(Instruction *I, const StringRef sinkName) {
  // TODO: check if the instruction already has the metadata
  auto &ctx = I->getContext();
  // set the metadata to the instruction as SINK_FUNC
  I->setMetadata(IR_SINK_FUNC, MDNode::get(ctx, MDString::get(ctx, sinkName)));
}

static void setMetadataTaint(Instruction *I, const StringRef taintName) {
  auto &ctx = I->getContext();
  // set the metadata to the instruction as TAINT_FUNC
  I->setMetadata(IR_TAINT_FUNC,
                 MDNode::get(ctx, MDString::get(ctx, taintName)));
}

void KintRangeAnalysisPass::markSinkedFuncs(Function &F) {

  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (CI->getCalledFunction() == nullptr)
          continue;
        auto funcName = CI->getCalledFunction()->getName();
        if (checkFuncSinked(funcName)) {
          const auto demangled_name =
              itaniumDemangle(funcName.data(), nullptr, nullptr, nullptr);
          if (demangled_name == funcName.data()) {
            auto paraNum = SINK_FUNCS.find(funcName)->getSecond();
            if (auto arg =
                    dyn_cast_or_null<Instruction>(CI->getArgOperand(paraNum))) {
              outs() << "Taint Analysis -> Found sink function: " << funcName
                     << "\n";
              setMetaDataSink(arg, funcName);
              break;
            }
          } else if (StringRef(demangled_name).startswith(funcName)) {
            outs() << "Taint Analysis -> Possible missing: " << funcName
                   << "\n";
          }
        }
      }
    }
  }
  // TODO:  check if the function is a taint source
}

bool KintRangeAnalysisPass::isTaintSource(const StringRef funcName) {

  if (funcName.startswith("sys_"))
    return true;

  for (const auto &s : TAINT_FUNCS) {
    // check for syscall functions
    if (funcName.contains(s))
      return true;
  }
  return false;
}

void KintRangeAnalysisPass::funcSinkCheck(Function &F) {
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        SINK_FUNCS.find(CI->getCalledFunction()->getName());
        if (SINK_FUNCS.find(CI->getCalledFunction()->getName()) !=
            SINK_FUNCS.end()) {
          outs() << "Found sink function: "
                 << CI->getCalledFunction()->getName() << "\n";
        }
      }
    }
  }
}

vector<CallInst *> KintRangeAnalysisPass::getTaintSource(Function &F) {
  vector<CallInst *> taintSources;

  // Check if this function is the taint source.
  const auto functionName = F.getName();
  if (isTaintSource(functionName)) {
    outs() << "Taint Analysis -> Found taint source: " << functionName << "\n";
    for (auto &arg : F.args()) {

      // TODO: check if have a better way to get the taint source
      auto callName = functionName.str() + IR_TAINT_FUNC_ARG.str() +
                      to_string(arg.getArgNo());

      outs() << "Taint Analysis -> Taint arg -> call inst: " << callName
             << "\n";

      auto Callee = F.getParent()->getOrInsertFunction(callName, arg.getType());

      // auto callInst = CallInst::Create(
      //     Callee, arg.getName(), &*F.getEntryBlock().getFirstInsertionPt());
      // Insert the call instruction before the terminator of the entry block
      if (F.empty()) {
        BasicBlock::Create(F.getContext(), "entry", &F);
      }

      auto &entryBlock = F.getEntryBlock();
      Instruction *terminator = entryBlock.getTerminator();
      auto callInst = CallInst::Create(Callee, arg.getName(), terminator);

      taintSources.push_back(callInst);
      arg.replaceAllUsesWith(callInst);
    }
  }
  return taintSources;
}

bool KintRangeAnalysisPass::sinkedReachable(Instruction *I) {
  if (nullptr == I) {
    return false;
  } else if (I->getMetadata(IR_SINK_FUNC)) {
    for (auto f : getSinkFuncs(I)) {
      taintedFunctions.insert(f);
    }
    return true;
  }

  bool sinked = false;

  if (auto SI = dyn_cast<StoreInst>(I)) {
    auto p = SI->getPointerOperand();
    if (auto GV = dyn_cast<GlobalVariable>(p)) {
      for (auto U : GV->users()) {
        if (auto UI = dyn_cast<Instruction>(U)) {
          if (U != SI) {
            sinked = sinked || sinkedReachable(UI);
          }
        }
      }
      if (sinked) {
        markSinkedFuncs(*I->getFunction());
        GV->setMetadata(IR_SINK_FUNC, I->getMetadata(IR_SINK_FUNC));
        return true;
      }
    }
  } else {
    if (auto CI = dyn_cast<CallInst>(I)) {
      if (auto F = CI->getCalledFunction()) {
        if (!F->isDeclaration()) {
          // TODO add a check for boradcast
          sinked = true;
          taintedFunctions.insert(F);
        }
      }
    }
  }

  for (auto U : I->users()) {
    if (auto UI = dyn_cast<Instruction>(U)) {
      sinked = sinked || sinkedReachable(UI);
    }
  }

  if (sinked) {
    setMetadataTaint(I, "");
    if (auto CI = dyn_cast<CallInst>(I)) {
      if (auto F = CI->getCalledFunction()) {
        if (!F->getReturnType()->isVoidTy()) {
          taintedFunctions.insert(F);
        }
      }
    }
    return true;
  }
  return false;
}
