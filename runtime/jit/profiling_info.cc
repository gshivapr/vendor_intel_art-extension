/*
 * Copyright (C) 2015 The Android Open Source Project
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
 */

#include "profiling_info.h"

#include "art_method-inl.h"
#include "dex_instruction.h"
#include "dex_instruction-inl.h"
#include "jit/jit.h"
#include "jit/jit_code_cache.h"
#include "scoped_thread_state_change.h"
#include "thread.h"

namespace art {

ProfilingInfo::ProfilingInfo(ArtMethod* method, 
                const std::vector<uint32_t>& entries,
                const std::vector<uint32_t>& dex_pcs)
      : number_of_inline_caches_(entries.size()),
        number_of_bb_counts_(dex_pcs.size()),
        method_(method),
        is_method_being_compiled_(false),
        is_osr_method_being_compiled_(false),
        current_inline_uses_(0),
        saved_entry_point_(nullptr) {
  memset(&cache_, 0, number_of_inline_caches_ * sizeof(InlineCache));
  for (size_t i = 0; i < number_of_inline_caches_; ++i) {
    cache_[i].dex_pc_ = entries[i];
  }
  BBCounts* counts = GetBBCounts();
  for (size_t i = 0; i < number_of_bb_counts_; ++i) {
    counts[i].dex_pc_ = dex_pcs[i];
    counts[i].count_ = 0;
  }
  if (method->IsCopied()) {
    // GetHoldingClassOfCopiedMethod is expensive, but creating a profiling info for a copied method
    // appears to happen very rarely in practice.
    holding_class_ = GcRoot<mirror::Class>(
        Runtime::Current()->GetClassLinker()->GetHoldingClassOfCopiedMethod(method));
  } else {
    holding_class_ = GcRoot<mirror::Class>(method->GetDeclaringClass());
  }
  DCHECK(!holding_class_.IsNull());
}

bool ProfilingInfo::Create(Thread* self, ArtMethod* method, bool retry_allocation) {
  // Walk over the dex instructions of the method and keep track of
  // instructions we are interested in profiling.
  DCHECK(!method->IsNative());

  const DexFile::CodeItem& code_item = *method->GetCodeItem();
  const uint16_t* code_ptr = code_item.insns_;
  const uint16_t* code_end = code_item.insns_ + code_item.insns_size_in_code_units_;

  uint32_t dex_pc = 0;
  std::vector<uint32_t> entries;
  std::set<uint32_t> dex_pc_bb_starts;
  // Method entry starts a block.
  dex_pc_bb_starts.insert(0);
  while (code_ptr < code_end) {
    const Instruction& instruction = *Instruction::At(code_ptr);
    switch (instruction.Opcode()) {
      case Instruction::INVOKE_VIRTUAL:
      case Instruction::INVOKE_VIRTUAL_RANGE:
      case Instruction::INVOKE_VIRTUAL_QUICK:
      case Instruction::INVOKE_VIRTUAL_RANGE_QUICK:
      case Instruction::INVOKE_INTERFACE:
      case Instruction::INVOKE_INTERFACE_RANGE:
        entries.push_back(dex_pc);
        break;

      case Instruction::GOTO:
      case Instruction::GOTO_16:
      case Instruction::GOTO_32:
        dex_pc_bb_starts.insert(dex_pc + instruction.GetTargetOffset());
        break;

      case Instruction::IF_EQ:
      case Instruction::IF_NE:
      case Instruction::IF_GT:
      case Instruction::IF_GE:
      case Instruction::IF_LT:
      case Instruction::IF_LE:
      case Instruction::IF_EQZ:
      case Instruction::IF_NEZ:
      case Instruction::IF_GTZ:
      case Instruction::IF_GEZ:
      case Instruction::IF_LTZ:
      case Instruction::IF_LEZ:
        // Need fall-through and target.
        dex_pc_bb_starts.insert(dex_pc + instruction.SizeInCodeUnits());
        dex_pc_bb_starts.insert(dex_pc + instruction.GetTargetOffset());
        break;

      case Instruction::PACKED_SWITCH: {
        const uint16_t* switch_data =
            reinterpret_cast<const uint16_t*>(&instruction) + instruction.VRegB_31t();
        uint16_t size = switch_data[1];
        // After the switch.
        dex_pc_bb_starts.insert(dex_pc + 3);
        const int32_t* targets = reinterpret_cast<const int32_t*>(&switch_data[4]);
        for (uint32_t i = 0; i < size; i++) {
          dex_pc_bb_starts.insert(dex_pc + targets[i]);
        }
        }
        break;

      case Instruction::SPARSE_SWITCH: {
        const uint16_t* switch_data =
            reinterpret_cast<const uint16_t*>(&instruction) + instruction.VRegB_31t();
        uint16_t size = switch_data[1];
        // After the switch.
        dex_pc_bb_starts.insert(dex_pc + 3);
        const int32_t* keys = reinterpret_cast<const int32_t*>(&switch_data[2]);
        const int32_t* switch_entries = keys + size;
        for (uint32_t i = 0; i < size; i++) {
          dex_pc_bb_starts.insert(dex_pc + switch_entries[i]);
        }
        }
        break;

      default:
        break;
    }
    dex_pc += instruction.SizeInCodeUnits();
    code_ptr += instruction.SizeInCodeUnits();
  }

  // Add in catch clauses.
  if (code_item.tries_size_ > 0) {
    const DexFile::TryItem* tries = DexFile::GetTryItems(code_item, 0);
    for (uint32_t i = 0, e = code_item.tries_size_; i < e; i++) {
      const DexFile::TryItem* try_item = &tries[i];
      for (CatchHandlerIterator it(code_item, *try_item); it.HasNext(); it.Next()) {
        dex_pc_bb_starts.insert(it.GetHandlerAddress());
      }
    }
  }

  // Create the list of BB targets from the set.
  std::vector<uint32_t> dex_pcs;
  for (auto i : dex_pc_bb_starts) {
    dex_pcs.push_back(i);
  }

  // We always create a `ProfilingInfo` object, even if there is no instruction we are
  // interested in. The JIT code cache internally uses it.

  // Allocate the `ProfilingInfo` object int the JIT's data space.
  jit::JitCodeCache* code_cache = Runtime::Current()->GetJit()->GetCodeCache();
  return code_cache->AddProfilingInfo(self, method, entries, dex_pcs, retry_allocation) != nullptr;
}

InlineCache* ProfilingInfo::GetInlineCache(uint32_t dex_pc) {
  InlineCache* cache = nullptr;
  // TODO: binary search if array is too long.
  for (size_t i = 0; i < number_of_inline_caches_; ++i) {
    if (cache_[i].dex_pc_ == dex_pc) {
      cache = &cache_[i];
      break;
    }
  }
  return cache;
}

void ProfilingInfo::AddInvokeInfo(uint32_t dex_pc, mirror::Class* cls) {
  InlineCache* cache = GetInlineCache(dex_pc);
  CHECK(cache != nullptr) << PrettyMethod(method_) << "@" << dex_pc;
  for (size_t i = 0; i < InlineCache::kIndividualCacheSize; ++i) {
    mirror::Class* existing = cache->classes_[i].Read();
    if (existing == cls) {
      // Receiver type is already in the cache, nothing else to do.
      return;
    } else if (existing == nullptr) {
      // Cache entry is empty, try to put `cls` in it.
      GcRoot<mirror::Class> expected_root(nullptr);
      GcRoot<mirror::Class> desired_root(cls);
      if (!reinterpret_cast<Atomic<GcRoot<mirror::Class>>*>(&cache->classes_[i])->
              CompareExchangeStrongSequentiallyConsistent(expected_root, desired_root)) {
        // Some other thread put a class in the cache, continue iteration starting at this
        // entry in case the entry contains `cls`.
        --i;
      } else {
        // We successfully set `cls`, just return.
        // Since the instrumentation is marked from the declaring class we need to mark the card so
        // that mod-union tables and card rescanning know about the update.
        // Note that the declaring class is not necessarily the holding class if the method is
        // copied. We need the card mark to be in the holding class since that is from where we
        // will visit the profiling info.
        if (!holding_class_.IsNull()) {
          Runtime::Current()->GetHeap()->WriteBarrierEveryFieldOf(holding_class_.Read());
        }
        return;
      }
    }
  }
  // Unsuccessfull - cache is full, making it megamorphic. We do not DCHECK it though,
  // as the garbage collector might clear the entries concurrently.
}

void ProfilingInfo::IncrementBBCount(uint32_t dex_pc) {
  BBCounts* counts = GetBBCounts();

  // Linear search for now.
  // TODO: replace with binary search if needed.
  for (size_t i = 0; i < number_of_bb_counts_; i++) {
    if (counts[i].dex_pc_ == dex_pc) {
      if (counts[i].count_ != std::numeric_limits<uint32_t>::max()) {
        counts[i].count_++;
      }
      return;
    }
  }
  DCHECK(false) << "Unable to locate BB Dex PC: 0x" << std::hex << dex_pc;
}

}  // namespace art
