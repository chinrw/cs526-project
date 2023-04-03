#include "check_insertion.h"
#include "kint_function.h"
#include "kint_module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "range.h"
#include "z3++.h"

using namespace llvm;
//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
void registerKintPass(PassBuilder &PB) {
  PB.registerPipelineParsingCallback(
      [](StringRef Name, FunctionPassManager &FPM,
         ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "kint-func-pass") {
          FPM.addPass(KintFunctionPass());
          return true;
        }
        return false;
      });
  PB.registerPipelineParsingCallback(
      [](StringRef Name, ModulePassManager &MPM,
         ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "kint-module-pass") {
          MPM.addPass(KintModulePass());
          return true;
        }
        return false;
      });
  PB.registerPipelineParsingCallback(
      [](StringRef Name, FunctionPassManager &FPM,
         ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "check-insertion-pass") {
          FPM.addPass(CheckInsertionPass());
          return true;
        }
        return false;
      });
  PB.registerPipelineParsingCallback(
      [](StringRef Name, ModulePassManager &MPM,
         ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "kint-range-analysis") {
          MPM.addPass(KintRangeAnalysisPass());
          return true;
        }
        return false;
      });
}

llvm::PassPluginLibraryInfo getKintPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KintPass", LLVM_VERSION_STRING,
          registerKintPass};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=kint-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKintPluginInfo();
}
