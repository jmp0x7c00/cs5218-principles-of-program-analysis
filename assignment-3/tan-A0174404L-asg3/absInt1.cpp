/*
	Tan Lai Chian Alan
	A0174404L
	e0210437@u.nus.edu
*/

#include <cstdio>
#include <iostream>
#include <utility>
#include <map>
#include <stack>
#include <algorithm>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"

using namespace llvm;
using namespace std;

typedef std::map<Value*,std::pair<int,int> > BBANALYSIS;

std::map<std::string,BBANALYSIS > analysisMap;

std::string getSimpleNodeLabel(const BasicBlock *Node);

//======================================================================
// Check fixpoint reached
//======================================================================
bool fixPointReached(std::map<std::string,BBANALYSIS> oldAnalysisMap) {
	if (oldAnalysisMap.empty())
		return false;
	for ( auto it = analysisMap.begin();it != analysisMap.end(); ++it)
	{
		if(oldAnalysisMap[it->first] != it->second)
			return false;
	}
	return true;
}

// Performs pair union
std::pair<int,int> union_pairs(std::pair<int,int> A, std::pair<int,int> B)
{
	// cout << "A: " << A.first << ", " << A.second << "\n";
	// cout << "B: " << B.first << ", " << B.second << "\n";

	if(A.first == A.second && A.first != 0) {
		if((B.first == B.second && B.first != 0)
			|| (B.first != -10000 && B.second != 10000)) {
			A.first = (A.first < B.first) ? A.first : B.first;
			A.second = (A.second > B.second) ? A.second : B.second;
		}
	}

	if(A.first == 0 && A.second == 0) {
		A.first = B.first;
		A.second = B.second;
	}

	if(A.first == -10000 && A.second == 10000
		&& B.first == B.second && B.first != 0) {
		A.first = B.first;
		A.second = B.second;
	}

	// cout << "RET: " << A.first << ", " << A.second << "\n\n";

    return A;
}

// Performs analysis union
BBANALYSIS union_analysis(BBANALYSIS A, BBANALYSIS B)
{
	// cout << "\tunion_analysis\n";
	for ( auto it = A.begin();it != A.end(); ++it)
	{
		A[it->first] = union_pairs(it->second,B[it->first]);
	}

	for ( auto it = B.begin();it != B.end(); ++it)
	{
		A[it->first] = union_pairs(it->second,A[it->first]);
	}

    return A;
}

//======================================================================
// update Basic Block Analysis
//======================================================================

// Processing Alloca Instruction
std::pair<int,int> processAlloca()
{
	return std::make_pair<int,int>(-10000, 10000);
}

// Processing Store Instruction
std::pair<int,int> processStore(llvm::Instruction* I, BBANALYSIS analysis)
{
	Value* op1 = I->getOperand(0);
	Value* op2 = I->getOperand(1);
	if(isa<ConstantInt>(op1)){
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		return std::make_pair<int,int>(op1Int, op1Int);
	}else if (analysis.find(op1) != analysis.end() ){
		return analysis[op1];
	} else {
		return std::make_pair<int,int>(-10000, 10000);
	}
}

// Processing Load Instruction
std::pair<int,int> processLoad(llvm::Instruction* I, BBANALYSIS analysis)
{
	Value* op1 = I->getOperand(0);
	if(isa<ConstantInt>(op1)){
		llvm::ConstantInt *CI = dyn_cast<ConstantInt>(op1);
		int64_t op1Int = CI->getSExtValue();
		return std::make_pair<int,int>(op1Int, op1Int);
	}else if (analysis.find(op1) != analysis.end() ){
		return analysis[op1];
	} else {
		return std::make_pair<int,int>(-10000, 10000);
	}
}

// Processing Mul Instructions
std::pair<int,int> processMul(llvm::Instruction* I, BBANALYSIS analysis)
{
	return std::make_pair<int,int>(-10000, 10000);
}

// Processing Div Instructions
std::pair<int,int> processDiv(llvm::Instruction* I, BBANALYSIS analysis)
{
	return std::make_pair<int,int>(-10000, 10000);
}

// Processing Add & Sub Instructions
std::pair<int,int> processAddSub(llvm::Instruction* I, BBANALYSIS analysis)
{
	return std::make_pair<int,int>(-10000, 10000);
}

// Processing Rem Instructions
std::pair<int,int> processRem(llvm::Instruction* I, BBANALYSIS analysis)
{
	return std::make_pair<int,int>(-10000, 10000);
}

// update Basic Block Analysis
BBANALYSIS updateBBAnalysis(BasicBlock* BB,BBANALYSIS analysis)
{
	// Loop through instructions in BB
	for (auto &I: *BB)
	{
	    if (isa<AllocaInst>(I)){
			analysis[&I] = processAlloca();
	    }else if (isa<StoreInst>(I)){
			Value* op2 = I.getOperand(1);
			analysis[op2] = processStore(&I, analysis);
	    }else if(isa<LoadInst>(I)){
			analysis[&I] = processLoad(&I, analysis);
	    }else if(I.getOpcode() == BinaryOperator::SDiv){
			analysis[&I] = processDiv(&I, analysis);
	    }else if(I.getOpcode() == BinaryOperator::Mul){
			analysis[&I] = processMul(&I, analysis);
	    }else if(I.getOpcode() == BinaryOperator::Add || I.getOpcode() == BinaryOperator::Sub){
			analysis[&I] = processAddSub(&I, analysis);
	    }else if(I.getOpcode() == BinaryOperator::SRem){
	    		analysis[&I] = processRem(&I, analysis);
	    }
	}
	return analysis;
}

//======================================================================
// update Graph Analysis
//======================================================================

BBANALYSIS applyCond_aux(BBANALYSIS predPair, Instruction* I, std::pair<int,int> pair){
	if (isa<AllocaInst>(I)){
    	predPair[I] = pair;
    }else if(isa<LoadInst>(I)){
	cout << I->getOperand(0) << "\n";
    	predPair[I] = pair;
    	predPair = applyCond_aux(predPair,dyn_cast<Instruction>(I->getOperand(0)),pair);
    }
	return predPair;
}

// Apply condition to the predecessor pair
BBANALYSIS applyCond(BBANALYSIS predPair, BasicBlock* predecessor, BasicBlock* BB)
{
	for (auto &I: *predecessor)
	{
		if (isa<BranchInst>(I)){
			BranchInst* br = dyn_cast<BranchInst>(&I);
			if(!br->isConditional())
				return predPair;
			llvm::CmpInst *cmp = dyn_cast<llvm::CmpInst>(br->getCondition());

			Value* op1 = cmp->getOperand(0);
			Value* op2 = cmp->getOperand(1);

			bool flag;
			if(BB == br->getOperand(2)) flag = true;
			if(BB == br->getOperand(1)) flag = false;

			int cmpValue = 0;
			
			switch (cmp->getPredicate()) {
			  case llvm::CmpInst::ICMP_SGT:{ // x < 0
				  if(flag == true) cmpValue = 1;
				  else { cmpValue = 0;
				  }
				  break;
			  }
			  case llvm::CmpInst::ICMP_SLT:{ // x >= 0
				  if(flag == true) cmpValue = 1;
				  else { cmpValue = 0;
				  }
				  break;
			  }
			}

			// std::cout << "NEW BRANCH INSTRUCTION:\n";
			// I.dump();
			// std::cout << "\top1: \n";
			// op1->dump();
			// std::cout << "\top2: \n";
			// op2->dump();
			// std::cout << "\tflag: " << flag << "\n";
			// std::cout << "\tPredicate: " << cmp->getPredicate() << "\n";
			// std::cout << "\tcmpValue: " << cmpValue << "\n";

			if(isa<LoadInst>(dyn_cast<Instruction>(cmp->getOperand(0)))){
				std::pair<int,int> tempPair;
				Instruction* l =  dyn_cast<Instruction>(cmp->getOperand(0));
			    if(cmpValue == 1){
					tempPair = std::make_pair<int,int>(1, 10000);
			    	predPair[l] = tempPair;
					// predPair = applyCond_aux(predPair,dyn_cast<Instruction>(l->getOperand(0)),tempPair);
			    }
			    else{
					tempPair = std::make_pair<int,int>(-10000, 0);
			    	predPair[l] = tempPair;
					// predPair = applyCond_aux(predPair,dyn_cast<Instruction>(l->getOperand(0)),tempPair);
			    }
			}
			std::cout << "\n";
		}
	}
	return predPair;
}

// update Graph Analysis
void updateGraphAnalysis(Function *F) {
    for (auto &BB: *F){
	// Print instructions for this BB
	// BB.dump();

    	BBANALYSIS predUnion;
        
	// Load the current stored analysis for all predecessor nodes
    	for (auto it = pred_begin(&BB), et = pred_end(&BB); it != et; ++it)
    	{
    		BasicBlock* predecessor = *it;
    		BBANALYSIS predPair = applyCond(analysisMap[predecessor->getName()],predecessor, &BB);
    		predUnion = union_analysis(predUnion,predPair);
    	}

    	BBANALYSIS BBAnalysis = updateBBAnalysis(&BB,predUnion);
    	BBANALYSIS OldBBAnalysis = analysisMap[BB.getName()];
	if(OldBBAnalysis != BBAnalysis){
		cout << "Label: " << getSimpleNodeLabel(&BB) << "\n";
		analysisMap[BB.getName()] = union_analysis(BBAnalysis,OldBBAnalysis);
	}

	// Print interval analysis for this BB
	for(auto it1 : analysisMap[BB.getName()])
	{
		// Get and print pair
		std::pair<int, int> pair = it1.second;
		std::cout << "\t" << it1.first << ": [" << pair.first << ", " << pair.second << "]\n";
	}

	// Print difference analysis for this BB
	cout << "\n\tDiff\t";
	for(auto it2 : analysisMap[BB.getName()])
	{
		std::cout << "\t" << it2.first;
	}
	cout << "\n";
	for(auto it1 : analysisMap[BB.getName()])
	{
		cout << "\t" << it1.first;

		for(auto it2 : analysisMap[BB.getName()])
		{
			std::cout << "\t\t" << std::max(it1.second.second, it2.second.second) - std::min(it1.second.first, it2.second.first);
		}
		cout << "\n";
	}
	cout << "\n\n";
    }
}

//======================================================================
// main function
//======================================================================

int main(int argc, char **argv)
{
    // Read the IR file.
    LLVMContext &Context = getGlobalContext();
    SMDiagnostic Err;

    // Extract Module M from IR (assuming only one Module exists)
    Module *M = ParseIRFile(argv[1], Err, Context);
    if (M == nullptr)
    {
      fprintf(stderr, "error: failed to load LLVM IR file \"%s\"", argv[1]);
      return EXIT_FAILURE;
    }

    // 1.Extract Function main from Module M
    Function *F = M->getFunction("main");

    // 2.Define analysisMap as a mapping of basic block labels to empty pair (of instructions):
    // For example: Assume the input LLVM IR has 4 basic blocks, the map
    // would look like the following:
    // entry -> {}
    // if.then -> {}
    // if.else -> {}
    // if.end -> {}
    for (auto &BB: *F){
    	BBANALYSIS emptyPair;
	analysisMap[BB.getName()] = emptyPair;
    }
    // Note: All variables are of type "alloca" instructions. Ex.
    // Variable a: %a = alloca i32, align 4

    // Keeping a snapshot of the previous ananlysis
    std::map<std::string,BBANALYSIS> oldAnalysisMap;

    // Fixpoint Loop
    int i = 0;
    bool loop = false;
    while(!fixPointReached(oldAnalysisMap)){
        oldAnalysisMap.clear();
        oldAnalysisMap.insert(analysisMap.begin(), analysisMap.end());
        updateGraphAnalysis(F);

	if(!loop) break;
    }

    return 0;
}

// Printing Basic Block Label
std::string getSimpleNodeLabel(const BasicBlock *Node) {
	if(!Node->getName().empty()) {
		return Node->getName().str();
	}
	std::string Str;
	raw_string_ostream OS(Str);
	Node->printAsOperand(OS, false);
	return OS.str();
}
