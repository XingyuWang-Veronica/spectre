#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/Constants.h"
#include <unordered_map>
#include <deque>

using namespace llvm;

/*
 * A very specific pass that finds a gadget with the following pattern:
 * 
 * %1 = mul nsw %secret, 512 (or %1 = shl %secret, 9)
 * %2 = sext i32 %1 to i64
 * %3 = getelementptr inbounds i32, i32* getelementptr inbounds (...), i64 %2
 *
 * NOTE: there can be multiple store + load between 1 and 2, and between 2 and 3
 */

/* 
 * valid state diagram:
 * 		mul -> (storeAfterMul -> mul) -> sext -> (storeAfterSext -> sext) -> getelementptr
 * 
 * mul: mul / shl / load (after store)
 * storeAfterMul: store
 * sext: sext / load (after store)
 * storeAfterSext: store
 * getelementptr: getelementptr
 */
enum Status {mul, storeAfterMul, sext, storeAfterSext, getelementptr};

namespace {
	struct Spectre: public FunctionPass {
		static char ID;
		Spectre(): FunctionPass(ID) {}
		
		void getAnalysisUsage(AnalysisUsage &AU) {
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F) override {
			std::unordered_map<Instruction *, Status> val2status;
			
			for (BasicBlock &BB: F) {
				for (Instruction &I: BB) {
					std::string op = std::string(I.getOpcodeName());
					bool find_mul = false;
					if (op == "shl") {
						// TODO: check
						if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))) {
							// errs() << "shl by a constant value\n";
							if (CI->getSExtValue() == 9) {
								errs() << "shl by 9\n";
								find_mul = true;
							}
						}
					} else if (op == "mul") {
						// TODO: check
						if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))) { 
							// errs() << "mul by a constant value\n";
							// errs() << "mul by " << CI->getSExtValue() << '\n';
							if (CI->getSExtValue() == 512) {
								errs() << "mul by 512\n";
								find_mul = true;
							}
						}
					}
					if (find_mul) {
						val2status[&I] = mul;
						// state diagram, initial stage is mul
						std::deque<Instruction *> values;
						values.push_back(&I);
						while (!values.empty()) {
							Instruction *front_val = values.front();
							values.pop_front();
							Status front_status = val2status[front_val];
							std::string op = std::string(front_val->getOpcodeName());
							// 
						}
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
