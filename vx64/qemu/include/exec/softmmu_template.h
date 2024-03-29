/*
 *  Software MMU support
 *
 * Generate helpers used by TCG for qemu_ld/st ops and code load
 * functions.
 *
 * Included from target op helpers and exec.c.
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "exec/memory.h"

#define DATA_SIZE (1 << SHIFT)

#if DATA_SIZE == 8
#define SUFFIX q
#define USUFFIX q
#define DATA_TYPE uint64_t
#elif DATA_SIZE == 4
#define SUFFIX l
#define USUFFIX l
#define DATA_TYPE uint32_t
#elif DATA_SIZE == 2
#define SUFFIX w
#define USUFFIX uw
#define DATA_TYPE uint16_t
#elif DATA_SIZE == 1
#define SUFFIX b
#define USUFFIX ub
#define DATA_TYPE uint8_t
#else
#error unsupported data size
#endif

#ifdef SOFTMMU_CODE_ACCESS
#define READ_ACCESS_TYPE 2
#define ADDR_READ addr_code
#else
#define READ_ACCESS_TYPE 0
#define ADDR_READ addr_read
#endif

static DATA_TYPE glue(glue(slow_ld, SUFFIX), MMUSUFFIX)(CPUArchState *env,
                                                        target_ulong addr,
                                                        int mmu_idx,
                                                        uintptr_t retaddr);
static inline DATA_TYPE glue(io_read, SUFFIX)(CPUArchState *env,
                                              hwaddr physaddr,
                                              target_ulong addr,
                                              uintptr_t retaddr)
{
    DATA_TYPE res;
    MemoryRegion *mr = iotlb_to_region(physaddr);

    physaddr = (physaddr & TARGET_PAGE_MASK) + addr;
    env->mem_io_pc = retaddr;
    if (mr != &io_mem_ram && mr != &io_mem_rom
        && mr != &io_mem_unassigned
        && mr != &io_mem_notdirty
            && !can_do_io(env)) {
        cpu_io_recompile(env, retaddr);
    }

    env->mem_io_vaddr = addr;
#if SHIFT <= 2
    res = io_mem_read(mr, physaddr, 1 << SHIFT);
#else
#ifdef TARGET_WORDS_BIGENDIAN
    res = io_mem_read(mr, physaddr, 4) << 32;
    res |= io_mem_read(mr, physaddr + 4, 4);
#else
    res = io_mem_read(mr, physaddr, 4);
    res |= io_mem_read(mr, physaddr + 4, 4) << 32;
#endif
#endif /* SHIFT > 2 */
    return res;
}

/* handle all cases except unaligned access which span two pages */
DATA_TYPE
glue(glue(helper_ld, SUFFIX), MMUSUFFIX)(CPUArchState *env, target_ulong addr,
                                         int mmu_idx)
{
    DATA_TYPE res;
    int index;
    target_ulong tlb_addr;
    hwaddr ioaddr;
    uintptr_t retaddr;

    /* test if there is match for unaligned or IO access */
    /* XXX: could done more in memory macro in a non portable way */
    index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
 redo:
    tlb_addr = env->tlb_table[mmu_idx][index].ADDR_READ;
    if ((addr & TARGET_PAGE_MASK) == (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
        if (tlb_addr & ~TARGET_PAGE_MASK) {
            /* IO access */
            if ((addr & (DATA_SIZE - 1)) != 0)
                goto do_unaligned_access;
            retaddr = GETPC_EXT();
            ioaddr = env->iotlb[mmu_idx][index];
            res = glue(io_read, SUFFIX)(env, ioaddr, addr, retaddr);
        } else if (((addr & ~TARGET_PAGE_MASK) + DATA_SIZE - 1) >= TARGET_PAGE_SIZE) {
            /* slow unaligned access (it spans two pages or IO) */
        do_unaligned_access:
            retaddr = GETPC_EXT();
#ifdef ALIGNED_ONLY
            do_unaligned_access(env, addr, READ_ACCESS_TYPE, mmu_idx, retaddr);
#endif
            res = glue(glue(slow_ld, SUFFIX), MMUSUFFIX)(env, addr,
                                                         mmu_idx, retaddr);
        } else {
            /* unaligned/aligned access in the same page */
            uintptr_t addend;
#ifdef ALIGNED_ONLY
            if ((addr & (DATA_SIZE - 1)) != 0) {
                retaddr = GETPC_EXT();
                do_unaligned_access(env, addr, READ_ACCESS_TYPE, mmu_idx, retaddr);
            }
#endif
            addend = env->tlb_table[mmu_idx][index].addend;
            res = glue(glue(ld, USUFFIX), _raw)((uint8_t *)(intptr_t)
                                                (addr + addend));
        }
    } else {
        /* the page is not in the TLB : fill it */
        retaddr = GETPC_EXT();
#ifdef ALIGNED_ONLY
        if ((addr & (DATA_SIZE - 1)) != 0)
            do_unaligned_access(env, addr, READ_ACCESS_TYPE, mmu_idx, retaddr);
#endif
        tlb_fill(env, addr, READ_ACCESS_TYPE, mmu_idx, retaddr);
        goto redo;
    }
    return res;
}

/* handle all unaligned cases */
static DATA_TYPE
glue(glue(slow_ld, SUFFIX), MMUSUFFIX)(CPUArchState *env,
                                       target_ulong addr,
                                       int mmu_idx,
                                       uintptr_t retaddr)
{
    DATA_TYPE res, res1, res2;
    int index, shift;
    hwaddr ioaddr;
    target_ulong tlb_addr, addr1, addr2;

    index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
 redo:
    tlb_addr = env->tlb_table[mmu_idx][index].ADDR_READ;
    if ((addr & TARGET_PAGE_MASK) == (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
        if (tlb_addr & ~TARGET_PAGE_MASK) {
            /* IO access */
            if ((addr & (DATA_SIZE - 1)) != 0)
                goto do_unaligned_access;
            ioaddr = env->iotlb[mmu_idx][index];
            res = glue(io_read, SUFFIX)(env, ioaddr, addr, retaddr);
        } else if (((addr & ~TARGET_PAGE_MASK) + DATA_SIZE - 1) >= TARGET_PAGE_SIZE) {
        do_unaligned_access:
            /* slow unaligned access (it spans two pages) */
            addr1 = addr & ~(DATA_SIZE - 1);
            addr2 = addr1 + DATA_SIZE;
            res1 = glue(glue(slow_ld, SUFFIX), MMUSUFFIX)(env, addr1,
                                                          mmu_idx, retaddr);
            res2 = glue(glue(slow_ld, SUFFIX), MMUSUFFIX)(env, addr2,
                                                          mmu_idx, retaddr);
            shift = (addr & (DATA_SIZE - 1)) * 8;
#ifdef TARGET_WORDS_BIGENDIAN
            res = (res1 << shift) | (res2 >> ((DATA_SIZE * 8) - shift));
#else
            res = (res1 >> shift) | (res2 << ((DATA_SIZE * 8) - shift));
#endif
            res = (DATA_TYPE)res;
        } else {
            /* unaligned/aligned access in the same page */
            uintptr_t addend = env->tlb_table[mmu_idx][index].addend;
            res = glue(glue(ld, USUFFIX), _raw)((uint8_t *)(intptr_t)
                                                (addr + addend));
        }
    } else {
        /* the page is not in the TLB : fill it */
        tlb_fill(env, addr, READ_ACCESS_TYPE, mmu_idx, retaddr);
        goto redo;
    }
    return res;
}

#ifndef SOFTMMU_CODE_ACCESS

static void glue(glue(slow_st, SUFFIX), MMUSUFFIX)(CPUArchState *env,
                                                   target_ulong addr,
                                                   DATA_TYPE val,
                                                   int mmu_idx,
                                                   uintptr_t retaddr);

static inline void glue(io_write, SUFFIX)(CPUArchState *env,
                                          hwaddr physaddr,
                                          DATA_TYPE val,
                                          target_ulong addr,
                                          uintptr_t retaddr)
{
    MemoryRegion *mr = iotlb_to_region(physaddr);

    physaddr = (physaddr & TARGET_PAGE_MASK) + addr;
    if (mr != &io_mem_ram && mr != &io_mem_rom
        && mr != &io_mem_unassigned
        && mr != &io_mem_notdirty
            && !can_do_io(env)) {
        cpu_io_recompile(env, retaddr);
    }

    env->mem_io_vaddr = addr;
    env->mem_io_pc = retaddr;
#if SHIFT <= 2
    io_mem_write(mr, physaddr, val, 1 << SHIFT);
#else
#ifdef TARGET_WORDS_BIGENDIAN
    io_mem_write(mr, physaddr, (val >> 32), 4);
    io_mem_write(mr, physaddr + 4, (uint32_t)val, 4);
#else
    io_mem_write(mr, physaddr, (uint32_t)val, 4);
    io_mem_write(mr, physaddr + 4, val >> 32, 4);
#endif
#endif /* SHIFT > 2 */
}

void glue(glue(helper_st, SUFFIX), MMUSUFFIX)(CPUArchState *env,
                                              target_ulong addr, DATA_TYPE val,
                                              int mmu_idx)
{
    hwaddr ioaddr;
    target_ulong tlb_addr;
    uintptr_t retaddr;
    int index;

    index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
 redo:
    tlb_addr = env->tlb_table[mmu_idx][index].addr_write;
    if ((addr & TARGET_PAGE_MASK) == (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
        if (tlb_addr & ~TARGET_PAGE_MASK) {
            /* IO access */
            if ((addr & (DATA_SIZE - 1)) != 0)
                goto do_unaligned_access;
            retaddr = GETPC_EXT();
            ioaddr = env->iotlb[mmu_idx][index];
            glue(io_write, SUFFIX)(env, ioaddr, val, addr, retaddr);
        } else if (((addr & ~TARGET_PAGE_MASK) + DATA_SIZE - 1) >= TARGET_PAGE_SIZE) {
        do_unaligned_access:
            retaddr = GETPC_EXT();
#ifdef ALIGNED_ONLY
            do_unaligned_access(env, addr, 1, mmu_idx, retaddr);
#endif
            glue(glue(slow_st, SUFFIX), MMUSUFFIX)(env, addr, val,
                                                   mmu_idx, retaddr);
        } else {
            /* aligned/unaligned access in the same page */
            uintptr_t addend;
#ifdef ALIGNED_ONLY
            if ((addr & (DATA_SIZE - 1)) != 0) {
                retaddr = GETPC_EXT();
                do_unaligned_access(env, addr, 1, mmu_idx, retaddr);
            }
#endif
            addend = env->tlb_table[mmu_idx][index].addend;
            glue(glue(st, SUFFIX), _raw)((uint8_t *)(intptr_t)
                                         (addr + addend), val);
        }
    } else {
        /* the page is not in the TLB : fill it */
        retaddr = GETPC_EXT();
#ifdef ALIGNED_ONLY
        if ((addr & (DATA_SIZE - 1)) != 0)
            do_unaligned_access(env, addr, 1, mmu_idx, retaddr);
#endif
        tlb_fill(env, addr, 1, mmu_idx, retaddr);
        goto redo;
    }
}

/* handles all unaligned cases */
static void glue(glue(slow_st, SUFFIX), MMUSUFFIX)(CPUArchState *env,
                                                   target_ulong addr,
                                                   DATA_TYPE val,
                                                   int mmu_idx,
                                                   uintptr_t retaddr)
{
    hwaddr ioaddr;
    target_ulong tlb_addr;
    int index, i;

    index = (addr >> TARGET_PAGE_BITS) & (CPU_TLB_SIZE - 1);
 redo:
    tlb_addr = env->tlb_table[mmu_idx][index].addr_write;
    if ((addr & TARGET_PAGE_MASK) == (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK))) {
        if (tlb_addr & ~TARGET_PAGE_MASK) {
            /* IO access */
            if ((addr & (DATA_SIZE - 1)) != 0)
                goto do_unaligned_access;
            ioaddr = env->iotlb[mmu_idx][index];
            glue(io_write, SUFFIX)(env, ioaddr, val, addr, retaddr);
        } else if (((addr & ~TARGET_PAGE_MASK) + DATA_SIZE - 1) >= TARGET_PAGE_SIZE) {
        do_unaligned_access:
            /* XXX: not efficient, but simple */
            /* Note: relies on the fact that tlb_fill() does not remove the
             * previous page from the TLB cache.  */
            /* modify by fshooter */
        	/* 添加了以下6行，首先做一次无害的操作（将原值读出来再写回去）来确保目标地址是可以写入的并且不会中间出发退出tb */
            /* 因为如果非对齐的写入在被拆分成 按字节的几次写入操作之后，某个字节的写入可能会触发当前tb失效导致退出当前块，但是前边的几次写入可能已经成功了，造成状态不一致 */
            /* x86下可写入的页面一定可读，所以读操作不会触发不期望的异常 */
            /* 但无害的读操作可能会触发硬件读断点,FIXME */
            /* added：如果是跨页面访问可能会导致期望的写异常变成读异常 ,FIXME */
        	for(i = DATA_SIZE - 1; i >= 0; i--) {
        		uint8_t origin_val = glue(slow_ldb, MMUSUFFIX)(env, addr + i, mmu_idx, retaddr);
        		glue(slow_stb, MMUSUFFIX)(env, addr + i,
        									origin_val,
        		               				mmu_idx, retaddr);
        	}
            for(i = DATA_SIZE - 1; i >= 0; i--) {
#ifdef TARGET_WORDS_BIGENDIAN
                glue(slow_stb, MMUSUFFIX)(env, addr + i,
                                          val >> (((DATA_SIZE - 1) * 8) - (i * 8)),
                                          mmu_idx, retaddr);
#else
                glue(slow_stb, MMUSUFFIX)(env, addr + i,
                                          val >> (i * 8),
                                          mmu_idx, retaddr);
#endif
            }
        } else {
            /* aligned/unaligned access in the same page */
            uintptr_t addend = env->tlb_table[mmu_idx][index].addend;
            glue(glue(st, SUFFIX), _raw)((uint8_t *)(intptr_t)
                                         (addr + addend), val);
        }
    } else {
        /* the page is not in the TLB : fill it */
        tlb_fill(env, addr, 1, mmu_idx, retaddr);
        goto redo;
    }
}

#endif /* !defined(SOFTMMU_CODE_ACCESS) */

#undef READ_ACCESS_TYPE
#undef SHIFT
#undef DATA_TYPE
#undef SUFFIX
#undef USUFFIX
#undef DATA_SIZE
#undef ADDR_READ
