#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

/*
 * A very specific pass that finds a gadget with the following pattern:
 * 
 * %1 = mul secret, 512 (or shl secret, 9)
 * %2 = add %1, array_addr 
 * %3 = inttoptr %2
 *
 *
 */
namespace {
	struct Spectre: public FunctionPass {
		static char ID;
		Spectre(): FunctionPass(ID) {}
		
		void getAnalysisUsage(AnalysisUsage &AU) {
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F) override {
			for (BasicBlock *BB: F) {
				for (Instruction *I: BB) {
					std::string op = std::string(I.getOpcodeName());
					if (op == "shl") {
						// TODO: check
						if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))) {
							errs() << "shl by a constant value\n";
						}
					} else if (op == "mul") {
						// TODO: check
					}
				}
			}
			return false;
		}
	};
}

char Spectre::ID = 0;
static RegisterPass<Spectre> X("s", "Spectre Pass",
		false,
		false
		);

static RegisterStandardPasses Y(
		PassManagerBuilder::EP_EarlyAsPossible,
		[](const PassManagerBuilder &Builder,
			legacy::PassManagerBase &PM) { PM.add(new Spectre()); });  
