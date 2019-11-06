#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

/* find a gadget that looks like the following:
 *
 * secret = load(secret_addr)
 * if (condition) {
 * 		if (secret) {
 * 			// fetch X
 * 		} else {
 * 			// fetch Y
 * 		}
 * }
 *
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
			// check for the branch condition at the end of each basic block
			for (BasicBlock &BB: F) {
				Instruction *branch = BB.getTerminator();
				if (auto i = dyn_cast<BranchInst>(branch)) {
					// Instruction *branchInst = cast<BranchInst>(branch);
					if (i->isConditional()) {
						errs() << "The branch instruction is ";
						branch->print(errs());
						errs() << "\n";
						for (BasicBlock *bb: i->successors()) {
							Instruction *front = bb->getFirstNonPHI();
							if (auto ii = dyn_cast<ICmpInst>(front)) {
								errs() << "The instruction at front is ";
								front->print(errs());
								errs() << "\n";
								// if this icmp instruction is used in the 
								// branch of the basic block
								Instruction *branch2 = bb->getTerminator();
								if (auto iii = dyn_cast<BranchInst>(branch2)) {
									if (iii->getCondition() == ii) {
										// if any use in icmp is the secret
										for (Use &U: ii->operands()) {
											Value *def = U.get();
											if (isa<Instruction>(def)) {
												if (auto loadInst = dyn_cast<LoadInst>(def)) {
													// if this instruction is defined in BB
													if (loadInst->getParent() == &BB) {
														errs() << "Found the gadget!\n";
														return false;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return false;
		}
	};
}

char Spectre::ID = 0;
static RegisterPass<Spectre> X("ic", "I-cache Pass",
		false,
		false
		);

static RegisterStandardPasses Y(
		PassManagerBuilder::EP_EarlyAsPossible,
		[](const PassManagerBuilder &Builder,
			legacy::PassManagerBase &PM) { PM.add(new Spectre()); });  
