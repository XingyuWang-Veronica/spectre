#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

/*
 * gadget finding for the following pattern:
 *
 * secret = load(secret_addr);
 * if (condition) {
 * 	call secret;
 * }
 *
 * where secret is a function pointer
 */

using namespace llvm;

namespace {
	struct Spectre: public FunctionPass {
		static char ID;
		Spectre(): FunctionPass(ID) {}
		
		void getAnalysisUsage(AnalysisUsage &AU) {
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F) override {
			for (BasicBlock &BB: F) {
				for (Instruction &I: BB) {
					if (isa<CallInst>(I)) {
						I.print(errs());
						errs() << "\n";
						Value *func = I.getOperand(0);
						/* 
						 * TODO: check if the this basic block is NOT the same with the basic block that loads func
						 */
						// find the def of func

					}
				}
			}
			return false;
		}
	};
}

char Spectre::ID = 0;
static RegisterPass<Spectre> X("fp", "Function Pointer Pass",
		false,
		false
		);

static RegisterStandardPasses Y(
		PassManagerBuilder::EP_EarlyAsPossible,
		[](const PassManagerBuilder &Builder,
			legacy::PassManagerBase &PM) { PM.add(new Spectre()); });  
