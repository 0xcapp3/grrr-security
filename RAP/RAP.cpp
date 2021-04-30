//=============================================================================
// FILE:
//    RAP.cpp
//
// DESCRIPTION:
//    GRRR
// template inspired from: https://github.com/banach-space/llvm-tutor
//
// License: MIT
//=============================================================================

#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>

#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/Statistic.h>

#include <llvm/Support/Regex.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Analysis/LoopInfo.h>

#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Scalar.h>

#include <llvm/IR/InstIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

using namespace llvm;

//-----------------------------------------------------------------------------
// RAP implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {
// Legacy PM implementation
class LegacyRAP : public ModulePass {

  public:
    static char ID;
    LegacyRAP() : ModulePass(ID) {}

    // This method implements what the pass does
    void visitor(Module &M, Function &F) {

        if (F.getName() == "__rap_init") return;

        GlobalVariable* rapCookie = M.getNameGlobal("__rap_cookie")
        assert(rapCookie);

        Function* getAddrOfRetAddr = Intrinsic::getDeclaration(&M, Intrinsic::addressofreturnaddress, { Type::getInt64PtrTy(M.getContext()) });
        Function* trapFunction     = Intrinsic::getDeclaration(&M, Intrinsic::trap);


        IRBuilder<> EntryBuilder(&*F.getEntryBlock().getFirstInsertionPt());
        Value* addrOfRetAddr = EntryBuilder.CreateCall(getAddrOfRetAddr, {}, "addrofretaddr");
        Value* retAddr = EntryBuilder.CreateLoad(addrOfRetAddr, "retaddr");
        
        Value* rapKey = EntryBuilder.CreateLoad(rapCookie);
        Value* encRet = EntryBuilder.CreateXor(retAddr, rapKey);

        std::set<ReturnInst*> rets;
        for (BasicBlock &BB: F) {
            for (Instruction &I : BB) {
                if (ReturnInst* ret = dyn_cast<ReturnInst>(&I)) {
                    rets.insert(ret);
                }
            }
        }

        BasicBlock* err_bb = BasicBlock::Create(F.getContext(), "err_bb", &F);
        IRBuilder<> errBuilder(err_bb);
        errBuilder.CreateCall(trapFunction);
        errBuilder.CreateUnreachable();

        for (ReturnInst* ret: rets) {
            BasicBlock* retBB = ret->getParent();

            Value *retVal = ret->getReturnValue();
            ret->eraseFromParent();

            BasicBlock* ok_bb = BasicBlock::Create(F.getContext(), "ok_bb", &F);
            IRBuilder<> okBuilder(ok_bb);
            okBuilder.CreateRet(retVal);

            IRBuilder<> retBuilder(retBB);
            Value* addrOfRetAddr = retBuilder.CreateCall(getAddrOfRetAddr, {}, "check_addrofretaddr");
            Value* newRetAddr    = retBuilder.CreateLoad(addrOfRetAddr, "check_retaddr");

            // Value* rapKey = retBuilder.getInt64(0x112233445566uL);
            Value* rapKey = EntryBuilder.CreateLoad(rapCookie);
            Value* decRet = retBuilder.CreateXor(encRet, rapKey);

            Value* is_ok = retBuilder.CreateICmpEQ(decRet, newRetAddr);
            retBuilder.CreateCondBr(is_ok, ok_bb, err_bb);
        }

        return;
    }

    // Main entry point - the name conveys what unit of IR this is to be run on.
    bool runOnModule(Module &M) override {

        for (Function &F : M) {
            // Skip if it is just a declaration
            if (F.isDeclaration())
                continue;

            // apply the pass to each function
            visitor(M, F);
        }
        // Does modify the input unit of IR, hence 'true'
        return true;
    }
};
} // namespace


//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyRAP::ID = 0;

// This is the core interface for pass plugins. It guarantees that 'opt' will
// recognize the pass when added to the pass pipeline on the command
// line, i.e.  via '--rap'
static RegisterPass<LegacyRAP>
X("rap", "RAP Pass",
    false, // This pass does modify the CFG => false
    false // This pass is not a pure analysis pass => false
);
