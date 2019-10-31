#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <unordered_map>
#include <deque>
#include <cstring>
#include <cassert>

using namespace llvm;

enum Status {init, shl, add, inttoptr};

namespace {
	struct Spectre: public FunctionPass {
		static char ID;
		Spectre(): FunctionPass(ID) {}
		
		void getAnalysisUsage(AnalysisUsage &AU) {
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F) override {
			std::unordered_map<Value *, Status> val2status;
			bool has_branch = false;
			bool finish_chain = false;
			for (BasicBlock &B: F) {
				for (Instruction &I: B) {
					I.print(errs());
					errs() << '\n';
					if (isa<LoadInst>(I) && !finish_chain) {
						errs() << "This instruction is a load\n";
						// get the current status
						Value *addr = I.getOperand(0);
						Status *status;
						// TODO: alias analysis
						if (!val2status.count(addr)) {
							val2status[addr] = init;
						} 
						status = &val2status[addr];
						// track to the very end or to a store
						for (auto U: I.users()) {
							// for simplicity, only consider mul, shl
							if (auto i = dyn_cast<Instruction>(U)) {
								errs() << "The user is an instruction\n";
								// std::string op = std::string(i->getOpcodeName());
								// Status cur_status = init;
								/*if (isa<StoreInst>(i) && val2status[addr] == add) {
									// almost done!
									finish_chain = true;
									// only need to consider branch afterwards
									if (has_branch) {
										errs() << "found the gadget!\n";
										return false;
									}
									break;
								} else if (isa<StoreInst(i*/
								if (isa<StoreInst>(i)) {
									Value *newAddr = i->getOperand(1);
									val2status[newAddr] = status;
								} /*(else if (op == "mul") {
									// assert(*status == init || *status == shl);
									if (status == init || status == mul) {
										// now we perform further analysis
									}
								}*/
								else {
									// further analysis (DFS)
									Status cur_status = *status;
									Instruction *curInst = i;
									// TODO: use the deque instead
									while (!isa<BranchInst>(curInst)) {
										std::string op = std::string(i->getOpcodeName());
										if (op == "mul" || op == "shl") {
											if (cur_status == init) {
												cur_status = mul;
											} else if (cur_status == "add") {
												// assuming we are not multiplying by 1
												break;
											}
										} else if (op == "add") {
											// TODO: check if the other operand is an array address
											bool is_addr = true;
											if (cur_status == mul && is_addr) {
												cur_status = add;
											} else {
												break;
											}
										} else if (op == "inttoptr") {
											if (cur_status == add) {
												cur_status = inttoptr;
											} else {
												break;
											}
										} else if (op == "load") {
											if (cur_status == "inttoptr") {
												finish_chain = true;
											}
											break;
										} else if (op == "store") {
											Value *newValue = curInst->getOperand(1);
											val2status[newValue] = cur_status;
											break;
										} else {
											// TODO: branch?
										}

									}
								}
							}
						}
					} else if (isa<BranchInst>(I)) {
						// TODO: add another conditions such as IndirectBrInst or CallBrInst
						// TODO: check if this is indeed a branch
						if (true) {
							has_branch = true;
						}
						if (finish_chain) {
							errs() << "found the gadget!\n";
							return false;
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
