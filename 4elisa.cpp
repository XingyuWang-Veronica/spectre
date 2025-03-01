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
 * %4 = load i32, i32* %3, align 4
 *
 * NOTES:
 * (1) there can be multiple store + load between 1 and 2, between 2 and 3, and between 3 and 4
 * (2) for now, assume that the depth of store + load is only one
 */

/* 
 * valid state diagram:
 * 		init -> mul / shl -> (storeAfterMul / storeAfterShl -> mul / shl) -> sext / zext -> (storeAfterSext / storeAfterZext -> sext / zext) -> getelementptr -> (storeAfterGet -> getelementptr) -> load
 * 
 * init: 
 * mul: mul / shl / load (after store)
 * storeAfterMul: store
 * sext: sext / load (after store)
 * storeAfterSext: store
 * getelementptr: getelementptr / load (after store)
 * storeAfterGet: store
 * load: load
 */
enum Status {init, mul, shl, storeAfterMul, storeAfterShl, sext, zext, storeAfterSext, storeAfterZext, getelementptr, storeAfterGet, load, invalid};
std::unordered_map<Status, std::string> statusString;

namespace {
	struct Spectre: public FunctionPass {
		static char ID;
		Spectre(): FunctionPass(ID) {}
		
		void getAnalysisUsage(AnalysisUsage &AU) {
			AU.setPreservesAll();
		}

		bool runOnFunction(Function &F) override {
			std::unordered_map<Instruction *, Status> val2status;
			statusString[init] = "init";
			statusString[mul] = "mul"; 
			statusString[shl] = "shl";
			statusString[storeAfterMul] = "storeAfterMul";
			statusString[storeAfterShl] = "storeAfterShl";
			statusString[sext] = "sext";
			statusString[zext] = "zext";
			statusString[storeAfterSext] = "storeAfterSext";
			statusString[storeAfterZext] = "storeAfterZext";
			statusString[getelementptr] = "getelementptr";
			statusString[storeAfterGet] = "storeAfterGet";
			statusString[load] = "load";
			statusString[invalid] = "invalid";
			for (BasicBlock &BB: F) {
				for (Instruction &I: BB) {
					std::string op = std::string(I.getOpcodeName());
					bool find_mul = false;
					bool is_mul = false;
					if (op == "shl") {
						// TODO: check
						if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))) {
							// errs() << "shl by a constant value\n";
							if (CI->getSExtValue() >= 8) {
								errs() << "shl by " << CI->getSExtValue() << "\n";
								find_mul = true;
							}
						}
					} else if (op == "mul") {
						// TODO: check
						if (ConstantInt *CI = dyn_cast<ConstantInt>(I.getOperand(1))) { 
							// errs() << "mul by a constant value\n";
							// errs() << "mul by " << CI->getSExtValue() << '\n';
							if (CI->getSExtValue() >= 256) {
								errs() << "mul by " << CI->getSExtValue() << "\n";
								find_mul = true;
								is_mul = true;
							}
						}
					}
					if (find_mul) {
						errs() << "find mul is true\n";
						val2status[&I] = is_mul? mul: shl;
						bool started = false;
						// state diagram, initial stage is init
						std::deque<Instruction *> values;
						values.push_back(&I);
						while (!values.empty()) {
							Instruction *front_val = values.front();
							errs() << "front_val is ";
							front_val->print(errs());
							errs() << "\n";
							values.pop_front();
							Status front_status = val2status[front_val];
							errs() << "front_status is " << statusString[front_status] << '\n';
							std::string op = std::string(front_val->getOpcodeName());
							errs() << "op is " << op << '\n';
							Status cur_status;
							if ((op == "mul" || op == "shl") && !started) {
								started = true;
								cur_status = front_status;
							} else if (op == "store" && front_status == mul) {
								cur_status = storeAfterMul;
							} else if (op == "load" && front_status == storeAfterMul) {
								cur_status = mul;
							} else if (op == "store" && front_status == shl) {
								cur_status = storeAfterShl;
							} else if (op == "load" && front_status == storeAfterShl) {
								cur_status = shl;
							} else if (op == "sext" && (front_status == mul || front_status == shl)) {
								cur_status = sext;
							} else if (op == "zext" && (front_status == mul || front_status == shl)) {
								cur_status = zext;
							} else if (op == "store" && front_status == sext) {
								cur_status = storeAfterSext;
							} else if (op == "load" && front_status == storeAfterSext) {
								cur_status = sext;
							} else if (op == "store" && front_status == zext) {
								cur_status = storeAfterZext;
							} else if (op == "load" && front_status == storeAfterZext) {
								cur_status = zext;
							} else if (op == "getelementptr" && (front_status == sext || front_status == zext || front_status == shl || front_status == mul)) {
								cur_status = getelementptr;
								// TODO: restriction
								// errs() << "Found the gadget!\n";
								// return false;
							} else if (op == "store" && front_status == getelementptr) {
								cur_status = storeAfterGet;
							} else if (op == "load" && front_status == storeAfterGet) {
								cur_status = getelementptr;
							} else if (op == "load" && front_status == getelementptr) {
								cur_status = load;
								errs() << "Found the gadget!\n";
								return false;
							} else {
								cur_status = invalid;
								continue;
							}
							errs() << "cur_status is " << statusString[cur_status] << '\n';
							Value *valInLoop = (op == "store")? front_val->getOperand(1): front_val;
							for (auto user: valInLoop->users()) {
								if (isa<Instruction>(user)) {
									Instruction *ii = cast<Instruction>(user);
									val2status[ii] = cur_status;
									values.push_back(ii);
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
static RegisterPass<Spectre> X("s", "Spectre Pass",
		false,
		false
		);

static RegisterStandardPasses Y(
		PassManagerBuilder::EP_EarlyAsPossible,
		[](const PassManagerBuilder &Builder,
			legacy::PassManagerBase &PM) { PM.add(new Spectre()); });  
