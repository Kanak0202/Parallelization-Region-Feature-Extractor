// IRFeaturePass.cpp
#include <iostream>

#include "IRFeaturePass.h"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Support/raw_ostream.h>

namespace {

// Instructions belonging to a loop's own induction-variable bookkeeping:
// the phi itself, its latch increment, and the icmp that bounds it.
// These are excluded from arithmetic counts (see design note above).
void collectInductionVarInstrs(
    llvm::Loop *L,
    llvm::ScalarEvolution &SE,
    llvm::SmallPtrSetImpl<llvm::Instruction*> &excluded)
{
    // std::cerr << "Collecting loop header: "
        //   << L->getHeader()->getName().str()
        //   << "\n";

    llvm::PHINode *PrimaryIndVar = L->getInductionVariable(SE);

    for (auto &PN : L->getHeader()->phis())
    {
        if (!PN.getType()->isIntegerTy() || !SE.isSCEVable(PN.getType()))
            continue;
        std::string S;
        llvm::raw_string_ostream OS(S);
        OS << PN;
        
        // std::cerr << "\n=== PHI ===\n";
        // std::cerr << OS.str() << "\n";

        auto *AR = llvm::dyn_cast<llvm::SCEVAddRecExpr>(SE.getSCEV(&PN));
        // std::cerr << "Has AddRec: " << (AR ? "YES" : "NO") << "\n";
        // std::cerr << "Primary IV: " << ((&PN == PrimaryIndVar) ? "YES" : "NO") << "\n";
        if (!AR || AR->getLoop() != L || !AR->isAffine())
            continue; // not a simple per-iteration linear phi of this loop

        // A phi that drives address computation (GEP) is real indexing
        // logic and stays excluded only if it's ALSO the loop's own
        // primary induction variable. A phi with no GEP use is pure
        // bound/bookkeeping (e.g. an auxiliary "i+1" tracker feeding a
        // nested loop's exit check) -- exclude regardless.
        bool feedsAddress = false;
        for (auto *User : PN.users())
            if (llvm::isa<llvm::GetElementPtrInst>(User))
                feedsAddress = true;
        // std::cerr << "feedsAddress: " << (feedsAddress ? "YES" : "NO") << "\n";
        if (feedsAddress && PrimaryIndVar && &PN != PrimaryIndVar)
            continue; // real indexing phi belonging to some other role -- keep countable
        // NOTE: if PrimaryIndVar is null, LLVM couldn't determine a
        // canonical primary IV for this loop (common when the trip
        // count depends on a runtime value, e.g. a break condition or
        // a bound that depends on an outer loop's variable). That's
        // not evidence this phi belongs to "another role" -- it's
        // still this loop's own header phi driving its own address
        // computation, so don't use a null PrimaryIndVar as grounds
        // to skip exclusion.
    //     if (feedsAddress && &PN != PrimaryIndVar) {
    // std::cerr << "*** SKIPPING PHI ***\n";
    //     }
        if (!feedsAddress) {
            // auxiliary bookkeeping PHI
            excluded.insert(&PN);
        } else {
            // PHI participates in indexing.
            // Decide whether its increment escapes into real computation.
        }

        excluded.insert(&PN);

        for (unsigned i = 0; i < PN.getNumIncomingValues(); ++i)
        {
            auto *IncomingInst = llvm::dyn_cast<llvm::Instruction>(PN.getIncomingValue(i));
            if (!IncomingInst || !L->contains(IncomingInst))
                continue;

            bool allUsesAreLoopControl = true;
            for (auto *User : IncomingInst->users())
            {
                if (User == &PN) continue;                       // the phi back-edge itself
                if (llvm::isa<llvm::ICmpInst>(User)) continue;    // the exit comparison
                allUsesAreLoopControl = false;
                break;
            }

            if (allUsesAreLoopControl){
                excluded.insert(IncomingInst);
                std::string S;
                llvm::raw_string_ostream OS(S);
                OS << *IncomingInst;
                // std::cerr << "[excluded] " << OS.str() << "\n";
            }

            // else: leave it countable -- it's real kernel arithmetic that also
            // happens to double as this loop's increment.

            for (auto *User : IncomingInst->users())
            {
                if (auto *CmpI = llvm::dyn_cast<llvm::ICmpInst>(User))
                {
                    // The loop latch comparison is excluded separately using
                    // Loop::getLoopLatch(). Leave other comparisons alone.
                    for (unsigned opI = 0; opI < CmpI->getNumOperands(); ++opI)
                        if (auto *BoundInst = llvm::dyn_cast<llvm::Instruction>(CmpI->getOperand(opI)))
                            if (BoundInst->hasOneUse())
                                excluded.insert(BoundInst);
                }
            }
        }

        // ---------------------------------------------------------------------
        // Exclude the canonical loop-latch comparison.
        //
        // LLVM may compare either the current IV or the incremented IV depending
        // on the optimization and loop form (i++, i+=2, i--, etc.). Rather than
        // relying on the comparison being a user of the increment instruction,
        // explicitly exclude the latch branch condition.
        // ---------------------------------------------------------------------
        if (auto *Latch = L->getLoopLatch())
        {
            if (auto *Br = llvm::dyn_cast<llvm::BranchInst>(Latch->getTerminator()))
            {
                if (Br->isConditional())
                {
                    if (auto *Cond =
                            llvm::dyn_cast<llvm::Instruction>(Br->getCondition()))
                    {
                        excluded.insert(Cond);
                    }
                }
            }
        }
    }
}

bool isSpecialMathCall(llvm::StringRef Name)
{
    static const llvm::StringSet<> SpecialFns = {
        "sqrt", "sqrtf", "sin", "sinf", "cos", "cosf",
        "exp", "expf", "log", "logf", "pow", "powf",
        "tan", "tanf", "sincos"
    };
    return SpecialFns.count(Name) > 0;
}

bool isSpecialMathIntrinsic(llvm::Intrinsic::ID ID)
{
    switch (ID)
    {
        case llvm::Intrinsic::sqrt:
        case llvm::Intrinsic::sin:
        case llvm::Intrinsic::cos:
        case llvm::Intrinsic::exp:
        case llvm::Intrinsic::exp2:
        case llvm::Intrinsic::log:
        case llvm::Intrinsic::log2:
        case llvm::Intrinsic::log10:
        case llvm::Intrinsic::pow:
        case llvm::Intrinsic::fabs:
        case llvm::Intrinsic::floor:
        case llvm::Intrinsic::ceil:
        case llvm::Intrinsic::trunc:
        case llvm::Intrinsic::round:
            return true;
        default:
            return false;
    }
}

} // namespace

// Returns the paired FMul instruction if I is a contract-flagged
// FAdd/FSub whose operand is a contract-flagged FMul -- i.e. an
// implicit (not-yet-backend-fused) FMA pair, as opposed to the
// explicit llvm.fmuladd/llvm.fma intrinsic form.
llvm::Instruction* getContractedFMulOperand(llvm::Instruction &I)
{
    if (I.getOpcode() != llvm::Instruction::FAdd &&
        I.getOpcode() != llvm::Instruction::FSub)
        return nullptr;
    if (!I.getFastMathFlags().allowContract())
        return nullptr;

    for (unsigned i = 0; i < I.getNumOperands(); ++i)
    {
        if (auto *MulI = llvm::dyn_cast<llvm::Instruction>(I.getOperand(i)))
        {
            if (MulI->getOpcode() == llvm::Instruction::FMul &&
                MulI->getFastMathFlags().allowContract())
                return MulI;
        }
    }
    return nullptr;
}

void extractIRFeatures(llvm::Function &F,
                        llvm::ScalarEvolution &SE,
                        llvm::LoopInfo &LI,
                        FeatureVector &FV)
{
    // IR-derived fields are authoritative once this pass runs --
    // discard whatever the AST-level heuristic guessed.
    FV.indirectAccesses = 0;
    FV.strideClass = 0;

    const llvm::DataLayout &DL = F.getParent()->getDataLayout();

    llvm::SmallPtrSet<llvm::Instruction*, 16> excludedFromArith;
    llvm::SmallPtrSet<llvm::BranchInst*, 16> excludedLoopBranches;

    for (llvm::Loop *L : LI)
    {
        for (llvm::Loop *SubL : llvm::depth_first(L))
        {
            auto *Latch = SubL->getLoopLatch();
            if (!Latch)
                continue;
            
            auto *Br = llvm::dyn_cast<llvm::BranchInst>(Latch->getTerminator());
            if (!Br || !Br->isConditional())
                continue;
            
            bool HasInsideSuccessor = false;
            bool HasOutsideSuccessor = false;
    
            for (unsigned i = 0; i < Br->getNumSuccessors(); ++i)
            {
                if (SubL->contains(Br->getSuccessor(i)))
                    HasInsideSuccessor = true;
                else
                    HasOutsideSuccessor = true;
            }
    
            if (HasInsideSuccessor && HasOutsideSuccessor)
                excludedLoopBranches.insert(Br);
        }
    }

    llvm::SmallPtrSet<llvm::Instruction*, 8> implicitFmaMuls;
    for (auto &BB : F)
        for (auto &I : BB)
            if (auto *MulI = getContractedFMulOperand(I))
                implicitFmaMuls.insert(MulI);

    llvm::SmallPtrSet<llvm::Value*, 8> classifiedIndirectPtrs;

    FV.basicBlocks = F.size();

    for (auto &BB : F)
    {
        for (auto &I : BB)
        {
            bool excluded = excludedFromArith.count(&I) > 0;
            if (I.getOpcode() == llvm::Instruction::Add) {
                std::string S;
                llvm::raw_string_ostream OS(S);
                OS << I;
                // std::cerr << "[add] "
                //           << (excludedFromArith.count(&I) ? "EXCLUDED " : "COUNTED ")
                //           << OS.str() << "\n";
            }
            switch (I.getOpcode())
            {
                case llvm::Instruction::Add:
                case llvm::Instruction::Sub:
                    if (!excluded) {
                        FV.intArithmetic++;
                        std::string s; llvm::raw_string_ostream(s) << I;
                        // std::cerr << "  [counted-arith] " << s << "\n";
                    }
                    break;
                case llvm::Instruction::FAdd:
                case llvm::Instruction::FSub: {
                    if (getContractedFMulOperand(I)) {
                        FV.fmaOperations++;
                        FV.floatMultiply++;
                        FV.floatArithmetic++;
                    } else {
                        FV.floatArithmetic++;
                    }
                    break;
                }
                case llvm::Instruction::Mul:
                    if (!excluded) FV.intMultiply++;
                    break;
                case llvm::Instruction::FMul:
                    if (!implicitFmaMuls.count(&I))
                        FV.floatMultiply++;
                    break;
                case llvm::Instruction::Shl:
                    if (!excluded) FV.intMultiply++;   // shl by constant == multiply by 2^k
                    break;
                case llvm::Instruction::LShr:
                case llvm::Instruction::AShr:
                    if (!excluded) FV.intDivision++;   // shr by constant == divide by 2^k
                    break;
                case llvm::Instruction::SDiv:
                case llvm::Instruction::UDiv:
                    FV.intDivision++;
                    break;
                case llvm::Instruction::FDiv:
                    FV.floatDivision++;
                    break;
                case llvm::Instruction::Load: {
                    auto *LI2 = llvm::cast<llvm::LoadInst>(&I);
                    FV.loads++;
                    uint64_t elemSize = DL.getTypeStoreSize(LI2->getType());
                    FV.bytesRead += elemSize;
                    classifyAccess(LI2->getPointerOperand(), SE, LI, elemSize,
                                    classifiedIndirectPtrs, FV);
                    break;
                }
                case llvm::Instruction::Store: {
                    auto *SI = llvm::cast<llvm::StoreInst>(&I);
                    FV.stores++;
                    uint64_t elemSize = DL.getTypeStoreSize(SI->getValueOperand()->getType());
                    FV.bytesWritten += elemSize;
                    classifyAccess(SI->getPointerOperand(), SE, LI, elemSize,
                                    classifiedIndirectPtrs, FV);
                    break;
                }
                case llvm::Instruction::Br: {
                    auto *BrI = llvm::cast<llvm::BranchInst>(&I);
                    if (!BrI->isConditional())
                        break;
                    // Loop latch/exit checks are structural loop control,
                    // not data-dependent branching -- same reasoning as
                    // excluding IV bookkeeping from arithmetic counts.
                    // Their condition is one of the ICmps already
                    // excluded above, so skip those; count everything
                    // else (if/else, ternaries lowered to branches, etc.)
                    // as real control-flow divergence within the region.
                    if (excludedLoopBranches.count(BrI))
                        break;
                    
                    FV.branchCount++;
                    break;
                }
                case llvm::Instruction::Switch:
                    FV.branchCount++;
                    break;
            }

            if (auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(&I))
{
    auto id = II->getIntrinsicID();
    if (id == llvm::Intrinsic::fmuladd || id == llvm::Intrinsic::fma)
    {
        FV.fmaOperations++;
        FV.floatMultiply++;
        FV.floatArithmetic++;
    }
    else if (isSpecialMathIntrinsic(id))
    {
        FV.specialFunctions++;
    }
}
else if (auto *CI = llvm::dyn_cast<llvm::CallInst>(&I))
{
    if (auto *Callee = CI->getCalledFunction())
        if (isSpecialMathCall(Callee->getName()))
            FV.specialFunctions++;
}
        }
    }
}

void classifyAccess(llvm::Value *Ptr,
                     llvm::ScalarEvolution &SE,
                     llvm::LoopInfo &LI,
                     uint64_t elemSizeBytes,
                     llvm::SmallPtrSetImpl<llvm::Value*> &classifiedIndirectPtrs,
                     FeatureVector &FV)
{
    auto *PtrInst = llvm::dyn_cast<llvm::Instruction>(Ptr);
    if (!PtrInst || !SE.isSCEVable(Ptr->getType()))
        return;

    llvm::Loop *L = LI.getLoopFor(PtrInst->getParent());
    if (!L)
        return;

    const llvm::SCEV *S = SE.getSCEV(Ptr);

    if (auto *AR = llvm::dyn_cast<llvm::SCEVAddRecExpr>(S))
    {
        if (AR->isAffine())
        {
            const llvm::SCEV *Step = AR->getStepRecurrence(SE);
            if (auto *ConstStep = llvm::dyn_cast<llvm::SCEVConstant>(Step))
            {
                int64_t strideBytes = ConstStep->getAPInt().getSExtValue();
                int cls = (std::abs(strideBytes) == (int64_t)elemSizeBytes) ? 1 : 2;
                std::string ptrStr;
                llvm::raw_string_ostream(ptrStr) << *Ptr;
                // std::cerr << "  [stride] " << ptrStr << " -> class " << cls
                    // << " (elemSize=" << elemSizeBytes << ")\n";
                FV.strideClass = std::max(FV.strideClass, cls);
                return;
            }
            FV.strideClass = std::max(FV.strideClass, 2);
            return;
        }
    }

    // Non-affine addressing -- classic gather/scatter pattern.
    // Dedupe: a load and store sharing the same computed address
    // (e.g. hist[A[i]]++) should count as one indirect access, not two.
    if (classifiedIndirectPtrs.insert(Ptr).second) {
        FV.strideClass = std::max(FV.strideClass, 3);
        FV.indirectAccesses++;
    }
}

