// SPDX-FileCopyrightText: Copyright 2024-2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/libraries/kernel/threads/pthread.h"
#include "core/libraries/kernel/threads/thread_state.h"
#include "core/memory/kernel.h"

namespace Libraries::Kernel {

static constexpr size_t RoundUp(size_t size) {
    if (size % ThrPageSize != 0) {
        size = ((size / ThrPageSize) + 1) * ThrPageSize;
    }
    return size;
}

int ThreadState::CreateStack(PthreadAttr* attr) {
    if ((attr->stackaddr_attr) != nullptr) {
        attr->guardsize_attr = 0;
        attr->flags |= PthreadAttrFlags::StackUser;
        return 0;
    }

    /*
     * Round up stack size to nearest multiple of _thr_page_size so
     * that mmap() * will work.  If the stack size is not an even
     * multiple, we end up initializing things such that there is
     * unused space above the beginning of the stack, so the stack
     * sits snugly against its guard.
     */
    size_t stacksize = RoundUp(attr->stacksize_attr);
    size_t guardsize = RoundUp(attr->guardsize_attr);

    attr->stackaddr_attr = nullptr;
    attr->flags &= ~PthreadAttrFlags::StackUser;

    /*
     * Use the garbage collector lock for synchronization of the
     * spare stack lists and allocations from usrstack.
     */
    thread_list_lock.lock();

    /*
     * If the stack and guard sizes are default, try to allocate a stack
     * from the default-size stack cache:
     */
    if (stacksize == ThrStackDefault && guardsize == ThrGuardDefault) {
        if (!dstackq.empty()) {
            /* Use the spare stack. */
            Stack* spare_stack = dstackq.top();
            dstackq.pop();
            attr->stackaddr_attr = spare_stack->stackaddr;
        }
    }
    /*
     * The user specified a non-default stack and/or guard size, so try to
     * allocate a stack from the non-default size stack cache, using the
     * rounded up stack size (stack_size) in the search:
     */
    else {
        const auto it = std::ranges::find_if(mstackq, [&](Stack* stack) {
            return stack->stacksize == stacksize && stack->guardsize == guardsize;
        });
        if (it != mstackq.end()) {
            attr->stackaddr_attr = (*it)->stackaddr;
            mstackq.erase(it);
        }
    }

    /* A cached stack was found.  Release the lock. */
    if (attr->stackaddr_attr != nullptr) {
        std::memset(attr->stackaddr_attr, 0, stacksize);
        thread_list_lock.unlock();
        return 0;
    }

    /* Allocate a stack from usrstack. */
    if (last_stack == 0) {
        static constexpr VAddr UsrStack = 0x7EFFF8000ULL;
        last_stack = UsrStack - ThrStackInitial - ThrGuardDefault;
    }

    /* Allocate a new stack. */
    VAddr stackaddr = last_stack - stacksize;

    /* Add additional stack space for HLE */
    static constexpr size_t AdditionalStack = 128_KB;
    VAddr reserve_addr = stackaddr - AdditionalStack - guardsize;

    /*
     * Even if stack allocation fails, we don't want to try to
     * use this location again, so unconditionally decrement
     * last_stack.  Under normal operating conditions, the most
     * likely reason for an mmap() error is a stack overflow of
     * the adjacent thread stack.
     */
    last_stack -= (stacksize + AdditionalStack + guardsize);

    /* Release the lock before mmap'ing it. */
    thread_list_lock.unlock();

    /* Map the stack page */
    auto* memory = Core::Memory::Instance();
    s32 ret =
        memory->MapMemory(&stackaddr, stacksize, Core::MemoryProt::CpuReadWrite,
                          Core::MemoryMapFlags::Stack | Core::MemoryMapFlags::Anon, -1, 0, "");
    ASSERT_MSG(ret == 0, "Unable to map stack memory");

    /* Map the additional stack space as system memory, we don't want
     * our additional space taking from the game's limited flexible size.
     */
    VAddr additional_addr = reserve_addr + guardsize;
    ret = memory->MapMemory(&additional_addr, AdditionalStack, Core::MemoryProt::CpuReadWrite,
                            Core::MemoryMapFlags::Fixed | Core::MemoryMapFlags::NoOverwrite |
                                Core::MemoryMapFlags::Anon | Core::MemoryMapFlags::System,
                            -1, 0, "");
    ASSERT_MSG(ret == 0, "Unable to map additional stack space");

    if (guardsize != 0) {
        /* Map the guard page */
        ret = memory->MapMemory(&reserve_addr, guardsize, Core::MemoryProt::NoAccess,
                                Core::MemoryMapFlags::Fixed | Core::MemoryMapFlags::NoOverwrite |
                                    Core::MemoryMapFlags::Void,
                                -1, 0, "stack guard");
        ASSERT_MSG(ret == 0, "Unable to map guard page");
    }
    attr->stackaddr_attr = (void*)stackaddr;

    if (attr->stackaddr_attr != nullptr) {
        std::memset(attr->stackaddr_attr, 0, stacksize);
        return 0;
    }
    return -1;
}

void* AllocateSystemStack(size_t size) {
    auto* memory = Core::Memory::Instance();
    VAddr addr = memory->GetAddressSpace().SystemReservedVirtualBase();
    const s32 ret =
        memory->MapMemory(&addr, size, Core::MemoryProt::CpuReadWrite,
                          Core::MemoryMapFlags::Anon | Core::MemoryMapFlags::System, -1, 0, "");
    ASSERT_MSG(ret == 0, "Unable to map system stack memory");
    return reinterpret_cast<void*>(addr);
}

void FreeSystemStack(void* addr, size_t size) {
    Core::Memory::Instance()->UnmapMemory(reinterpret_cast<VAddr>(addr), size);
}

void ThreadState::FreeStack(PthreadAttr* attr) {
    if (!attr || True(attr->flags & PthreadAttrFlags::StackUser) || !attr->stackaddr_attr) {
        return;
    }

    auto* stack_base = static_cast<char*>(attr->stackaddr_attr);
    auto* spare_stack = reinterpret_cast<Stack*>(stack_base + attr->stacksize_attr - sizeof(Stack));
    spare_stack->stacksize = RoundUp(attr->stacksize_attr);
    spare_stack->guardsize = RoundUp(attr->guardsize_attr);
    spare_stack->stackaddr = attr->stackaddr_attr;

    if (spare_stack->stacksize == ThrStackDefault && spare_stack->guardsize == ThrGuardDefault) {
        /* Default stack/guard size. */
        dstackq.push(spare_stack);
    } else {
        /* Non-default stack/guard size. */
        mstackq.push_back(spare_stack);
    }
    attr->stackaddr_attr = nullptr;
}

} // namespace Libraries::Kernel
