/*
 * Copyright (C) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef ART_COMPILER_OPTIMIZING_EXTENSIONS_INFRASTRUCTURE_CLONING_H_
#define ART_COMPILER_OPTIMIZING_EXTENSIONS_INFRASTRUCTURE_CLONING_H_

#include "graph_x86.h"
#include "nodes.h"
#include "nodes_x86.h"

namespace art {

/**
 * @brief Used to clone instructions.
 * @details Note that this does not copy users from source. Thus, instructions
 * are not valid until proper inputs are added.
 */
class HInstructionCloner : public HGraphVisitor {
 public:
  /**
   * Create an instruction cloner.
   * @param graph Graph containing the instructions to clone.
   * @param enable_cloning 'false' to see if cloning is possible, 'true' to clone.
   * @param use_cloned_inputs 'true' if cloned instructions should use already
   * cloned inputs.
   * @param allow_overwrite Allow one instruction to have multiple clones. It is deactivated
   * by default because it is safer to keep one single mapping, to prevent potential memory
   * leaks. However, there are certain situations where allowing such feature becomes handy.
   * It is the case, for example, of Loop Unrolling.
   */
  HInstructionCloner(HGraph_X86* graph,
                     bool enable_cloning = true,
                     bool use_cloned_inputs = true,
                     bool allow_overwrite = false) :
      HGraphVisitor(graph),
      cloning_enabled_(enable_cloning),
      all_cloned_okay_(true),
      use_cloned_inputs_(use_cloned_inputs),
      allow_overwrite_(allow_overwrite),
      arena_(graph->GetArena()),
      debug_name_failed_clone_(nullptr) {}

  void VisitInstruction(HInstruction* instruction) OVERRIDE {
    if (kIsDebugBuild) {
      // If the instruction cannot be cloned and we get here, it means that it was
      // non-intentionally not supported.
      LOG(FATAL) << "Found instruction that cannot be cloned " << instruction->DebugName();
    }
    // Mark instruction is not supported for cloning.
    UnsupportedInstruction(instruction);
  }

  void AddOrUpdateCloneManually(HInstruction* original, HInstruction* clone) {
    DCHECK(original != nullptr);
    DCHECK(clone != nullptr);
    orig_to_clone_.Overwrite(original, clone);
  }

  void AddCloneManually(HInstruction* original, HInstruction* clone) {
    DCHECK(original != nullptr);
    DCHECK(clone != nullptr);
    orig_to_clone_.Put(original, clone);
    if (kIsDebugBuild) {
      manual_clones_.insert(original);
    }
  }

  HInstruction* GetClone(HInstruction* source) const {
    if (orig_to_clone_.count(source) != 0) {
      HInstruction* clone = orig_to_clone_.Get(source);
      DCHECK(clone != nullptr);
      return clone;
    } else {
      return nullptr;
    }
  }

  bool AllOkay() const;

  const char* GetDebugNameForFailedClone() const {
    DCHECK(!all_cloned_okay_);
    return debug_name_failed_clone_;
  }

  HEnvironment* CloneEnvironment(HEnvironment* env, HInstruction* clone);

  void VisitAbove(HAbove* instr) OVERRIDE;
  void VisitAboveOrEqual(HAboveOrEqual* instr) OVERRIDE;
  void VisitAdd(HAdd* instr) OVERRIDE;
  void VisitAddLHSMemory(HAddLHSMemory* instr) OVERRIDE {
    // Cloning makes sense, but this instruction typically generated by backend.
    // Can be supported in future if needed.
    UnsupportedInstruction(instr);
  }
  void VisitAddRHSMemory(HAddRHSMemory* instr) OVERRIDE  {
    // Cloning makes sense, but this instruction typically generated by backend.
    // Can be supported in future if needed.
    UnsupportedInstruction(instr);
  }
  void VisitAnd(HAnd* instr) OVERRIDE;
  void VisitArrayGet(HArrayGet* instr) OVERRIDE;
  void VisitArrayLength(HArrayLength* instr) OVERRIDE;
  void VisitArraySet(HArraySet* instr) OVERRIDE;
  void VisitBelow(HBelow* instr) OVERRIDE;
  void VisitBelowOrEqual(HBelowOrEqual* instr) OVERRIDE;
  void VisitBooleanNot(HBooleanNot* instr) OVERRIDE;
  void VisitBoundsCheck(HBoundsCheck* instr) OVERRIDE;
  void VisitBoundType(HBoundType* instr) OVERRIDE;
  void VisitCheckCast(HCheckCast* instr) OVERRIDE;
  void VisitClassTableGet(HClassTableGet* instr) OVERRIDE;
  void VisitClearException(HClearException* instr) OVERRIDE;
  void VisitClinitCheck(HClinitCheck* instr) OVERRIDE;
  void VisitCompare(HCompare* instr) OVERRIDE;
  void VisitCurrentMethod(HCurrentMethod* instr) OVERRIDE {
    // Cloning does not make sense - there is only one ArtMethod parameter.
    UnsupportedInstruction(instr);
  }
  void VisitDeoptimize(HDeoptimize* instr) OVERRIDE;
  void VisitDevirtGuard(HDevirtGuard* instr) OVERRIDE;
  void VisitDiv(HDiv* instr) OVERRIDE;
  void VisitDivRHSMemory(HDivRHSMemory* instr) OVERRIDE {
    // Cloning makes sense, but this instruction typically generated by backend.
    // Can be supported in future if needed.
    UnsupportedInstruction(instr);
  }
  void VisitDivZeroCheck(HDivZeroCheck* instr) OVERRIDE;
  void VisitDoubleConstant(HDoubleConstant* instr) OVERRIDE {
    // Cloning does not make sense - constants are only inserted once per method.
    UnsupportedInstruction(instr);
  }
  void VisitEqual(HEqual* instr) OVERRIDE;
  void VisitExit(HExit* instr) OVERRIDE {
    // Cloning does not make sense - there is only one exit per method.
    UnsupportedInstruction(instr);
  }
  void VisitFloatConstant(HFloatConstant* instr) OVERRIDE {
    // Cloning does not make sense - constants are only inserted once per method.
    UnsupportedInstruction(instr);
  }
  void VisitGoto(HGoto* instr) OVERRIDE;
  void VisitGreaterThan(HGreaterThan* instr) OVERRIDE;
  void VisitGreaterThanOrEqual(HGreaterThanOrEqual* instr) OVERRIDE;
  void VisitIf(HIf* instr) OVERRIDE;
  void VisitInstanceFieldGet(HInstanceFieldGet* instr) OVERRIDE;
  void VisitInstanceFieldSet(HInstanceFieldSet* instr) OVERRIDE;
  void VisitInstanceOf(HInstanceOf* instr) OVERRIDE;
  void VisitIntConstant(HIntConstant* instr) OVERRIDE {
    // Cloning does not make sense - constants are only inserted once per method.
    UnsupportedInstruction(instr);
  }
  void VisitInvokeInterface(HInvokeInterface* instr) OVERRIDE;
  void VisitInvokeStaticOrDirect(HInvokeStaticOrDirect* instr) OVERRIDE;
  void VisitInvokeVirtual(HInvokeVirtual* instr) OVERRIDE;
  void VisitInvokeUnresolved(HInvokeUnresolved* instr) OVERRIDE;
  void VisitLessThan(HLessThan* instr) OVERRIDE;
  void VisitLessThanOrEqual(HLessThanOrEqual* instr) OVERRIDE;
  void VisitLoadClass(HLoadClass* instr) OVERRIDE;
  void VisitLoadException(HLoadException* instr) OVERRIDE;
  void VisitLoadString(HLoadString* instr) OVERRIDE;
  void VisitLongConstant(HLongConstant* instr) OVERRIDE {
    // Cloning does not make sense - constants are only inserted once per method.
    UnsupportedInstruction(instr);
  }
  void VisitMemoryBarrier(HMemoryBarrier* instr) OVERRIDE;
  void VisitMonitorOperation(HMonitorOperation* instr) OVERRIDE;
  void VisitMul(HMul* instr) OVERRIDE;
  void VisitMulRHSMemory(HMulRHSMemory* instr) OVERRIDE {
    // Cloning makes sense, but this instruction typically generated by backend.
    // Can be supported in future if needed.
    UnsupportedInstruction(instr);
  }
  void VisitNativeDebugInfo(HNativeDebugInfo* instr) OVERRIDE;
  void VisitNeg(HNeg* instr) OVERRIDE;
  void VisitNewArray(HNewArray* instr) OVERRIDE;
  void VisitNewInstance(HNewInstance* instr) OVERRIDE;
  void VisitNot(HNot* instr) OVERRIDE;
  void VisitNotEqual(HNotEqual* instr) OVERRIDE;
  void VisitNullConstant(HNullConstant* instr) OVERRIDE {
    // Cloning does not make sense - constants are only inserted once per method.
    UnsupportedInstruction(instr);
  }
  void VisitNullCheck(HNullCheck* instr) OVERRIDE;
  void VisitOr(HOr* instr) OVERRIDE;
  void VisitPackedSwitch(HPackedSwitch* instr) OVERRIDE;
  void VisitParallelMove(HParallelMove* instr) OVERRIDE {
    // Cloning does not make sense - this is a register allocator construct.
    UnsupportedInstruction(instr);
  }
  void VisitParameterValue(HParameterValue* instr) OVERRIDE {
    // Cloning does not make sense - this is a parameter value.
    UnsupportedInstruction(instr);
  }
  void VisitPhi(HPhi* instr) OVERRIDE;
  void VisitRem(HRem* instr) OVERRIDE;
  void VisitReturn(HReturn* instr) OVERRIDE;
  void VisitReturnVoid(HReturnVoid* instr) OVERRIDE;
  void VisitRor(HRor* instr) OVERRIDE;
  void VisitSelect(HSelect* instr) OVERRIDE;
  void VisitShl(HShl* instr) OVERRIDE;
  void VisitShr(HShr* instr) OVERRIDE;
  void VisitStaticFieldGet(HStaticFieldGet* instr) OVERRIDE;
  void VisitStaticFieldSet(HStaticFieldSet* instr) OVERRIDE;
  void VisitSub(HSub* instr) OVERRIDE;
  void VisitSubRHSMemory(HSubRHSMemory* instr) OVERRIDE {
    // Cloning makes sense, but this instruction typically generated by backend.
    // Can be supported in future if needed.
    UnsupportedInstruction(instr);
  }
  void VisitSuspend(HSuspend* instr) OVERRIDE;
  void VisitSuspendCheck(HSuspendCheck* instr) OVERRIDE;
  void VisitTestSuspend(HTestSuspend* instr) OVERRIDE;
  void VisitThrow(HThrow* instr) OVERRIDE;
  void VisitTryBoundary(HTryBoundary* instr) OVERRIDE;
  void VisitTypeConversion(HTypeConversion* instr) OVERRIDE;
  void VisitUShr(HUShr* instr) OVERRIDE;
  void VisitUnresolvedInstanceFieldGet(HUnresolvedInstanceFieldGet* instr) OVERRIDE;
  void VisitUnresolvedInstanceFieldSet(HUnresolvedInstanceFieldSet* instr) OVERRIDE;
  void VisitUnresolvedStaticFieldGet(HUnresolvedStaticFieldGet* instr) OVERRIDE;
  void VisitUnresolvedStaticFieldSet(HUnresolvedStaticFieldSet* instr) OVERRIDE;
  void VisitXor(HXor* instr) OVERRIDE;
  void VisitX86SelectValue(HX86SelectValue* instr) OVERRIDE;
  void VisitX86ProfileInvoke(HX86ProfileInvoke* instr) OVERRIDE;
  void VisitX86IncrementExecutionCount(HX86IncrementExecutionCount* instr) OVERRIDE;
  void VisitX86ComputeBaseMethodAddress(HX86ComputeBaseMethodAddress* instr) OVERRIDE {
    // Cloning does not make sense - this should only be done once per method.
    UnsupportedInstruction(instr);
  }
  void VisitX86LoadFromConstantTable(HX86LoadFromConstantTable* instr) OVERRIDE;
  void VisitX86FPNeg(HX86FPNeg* instr) OVERRIDE;
  void VisitX86PackedSwitch(HX86PackedSwitch* instr) OVERRIDE;
  void VisitX86BoundsCheckMemory(HX86BoundsCheckMemory* instr) OVERRIDE;

 private:
  void GetInputsForUnary(HInstruction* instr, HInstruction** input_ptr) const;
  void GetInputsForBinary(HInstruction* instr,
                          HInstruction** lhs_ptr,
                          HInstruction** rhs_ptr) const;
  void GetInputsForTernary(HInstruction* instr,
                           HInstruction** input0_ptr,
                           HInstruction** input1_ptr,
                           HInstruction** input2_ptr) const;
  void GetInputsForQuaternary(HInstruction* instr,
                              HInstruction** input0_ptr,
                              HInstruction** input1_ptr,
                              HInstruction** input2_ptr,
                              HInstruction** input3_ptr) const;
  void GetInputsForMany(HInstruction* instr,
                        std::vector<HInstruction*>& inputs) const;
  void CloneEnvironment(HInstruction* instr, HInstruction* clone);
  void FinishInvokeCloning(HInvoke* instr, HInvoke* clone);

  /**
   * @brief Used to commit clone.
   * @param instr The instruction being cloned.
   * @param clone The clone of instr.
   */
  void CommitClone(HInstruction* instr, HInstruction* clone) {
    if (!allow_overwrite_) {
      DCHECK(orig_to_clone_.find(instr) == orig_to_clone_.end());
    }
    orig_to_clone_.Overwrite(instr, clone);
  }

  /**
   * @brief Used to mark instruction is not supported by cloner.
   * @param instruction The instruction that is not supported.
   */
  void UnsupportedInstruction(HInstruction* instruction) {
    all_cloned_okay_ = false;
    debug_name_failed_clone_ = instruction->DebugName();
  }

  const bool cloning_enabled_;
  bool all_cloned_okay_;
  const bool use_cloned_inputs_;
  const bool allow_overwrite_;
  SafeMap<HInstruction*, HInstruction*> orig_to_clone_;
  std::set<HInstruction*> manual_clones_;
  ArenaAllocator* arena_;
  const char* debug_name_failed_clone_;
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_EXTENSIONS_INFRASTRUCTURE_CLONING_H_
