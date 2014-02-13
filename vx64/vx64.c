#include "vx64.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>


void invalidate_and_set_dirty(hwaddr addr, hwaddr length);

#define X86_PDE_NUM 1024
int singlestep = 0;
CPUArchState *env;
rdtsc_helper_routine rdtsc_helper = 0;
void* rdtsc_helper_extra = 0;

void cpu_smm_update(CPUX86State *env)
{
}

uint64_t cpu_get_tsc(CPUX86State *env)
{
    if(rdtsc_helper)
    	return rdtsc_helper(rdtsc_helper_extra);
    return 0;
}

const char *qemu_get_version(void)
{
    return "1.4";
}

VXError vx64_init()
{
	qemu_set_log(0, true);
	use_icount = 1;
	module_call_init(MODULE_INIT_QOM);
	tcg_exec_init(0);
	cpu_exec_init_all();
	env = cpu_init("qemu64");
	if (!env) {
	    return VXERR_FAIL;
	}
	cpu_x86_set_cpl(env, 3);
	cpu_x86_update_cr0(env, CR0_PE_MASK );
	cpu_x86_update_cr4(env, CR4_PAE_MASK );
	cpu_load_efer(env, MSR_EFER_LME );
	cpu_x86_update_cr0(env, CR0_PG_MASK | CR0_WP_MASK );

	vx64_reset();
	return VXERR_OK;
}

VXError vx64_reset() {
	tb_flush(env);
	tlb_flush(env, 1);
	return VXERR_OK;
}

VXError vx64_setup_rdtsc_helper(rdtsc_helper_routine helper, void* extra)
{
	rdtsc_helper = helper;
	rdtsc_helper_extra = extra;
	return VXERR_OK;
}

void* vx64_setup_ram(vxpma_t guest_pa, uint32_t size, void* ptr)
{
	static int mr_index = 0;
	char mr_name_buf[128];
	sprintf(mr_name_buf, "%s%d", "raw_process_memory", mr_index++);
	MemoryRegion *mr = malloc(sizeof(MemoryRegion));
	memset(mr, 0, sizeof(*mr));
	memory_region_init_ram_ptr(mr, mr_name_buf, size, ptr);
	memory_region_add_subregion(get_system_memory(), guest_pa, mr);
	return mr;
}

void vx64_remove_ram(void* mr)
{
	memory_region_del_subregion(get_system_memory(), (MemoryRegion*)mr);
	memory_region_destroy(mr);
	free(mr);
}

VXError vx64_get_reg(TransReg id, vxlong_t* v) {
	uint32_t r = VXERR_OK;
	switch(id) {
	case VXR_EAX:
		*v = env->regs[R_EAX];
		break;
	case VXR_ECX:
		*v = env->regs[R_ECX];
		break;
	case VXR_EDX:
		*v = env->regs[R_EDX];
		break;
	case VXR_EBX:
		*v = env->regs[R_EBX];
		break;
	case VXR_ESP:
		*v = env->regs[R_ESP];
		break;
	case VXR_EBP:
		*v = env->regs[R_EBP];
		break;
	case VXR_ESI:
		*v = env->regs[R_ESI];
		break;
	case VXR_EDI:
		*v = env->regs[R_EDI];
		break;
	case VXR_EFLAGS:
		*v = env->eflags;
		break;
	case VXR_EIP:
		*v = env->eip;
		break;
	case VXR_ES:
		*v = env->segs[R_ES].selector;
		break;
	case VXR_CS:
		*v = env->segs[R_CS].selector;
		break;
	case VXR_SS:
		*v = env->segs[R_SS].selector;
		break;
	case VXR_DS:
		*v = env->segs[R_DS].selector;
		break;
	case VXR_FS:
		*v = env->segs[R_FS].selector;
		break;
	case VXR_GS:
		*v = env->segs[R_GS].selector;
		break;
	case VXR_DR0:
		*v = env->dr[0];
		break;
	case VXR_DR1:
		*v = env->dr[1];
		break;
	case VXR_DR2:
		*v = env->dr[2];
		break;
	case VXR_DR3:
		*v = env->dr[3];
		break;
	case VXR_DR6:
		*v = env->dr[6];
		break;
	case VXR_DR7:
		*v = env->dr[7];
		break;
	case VXR_CR0:
		*v = env->cr[0];
		break;
	case VXR_CR2:
		*v = env->cr[2];
		break;
	case VXR_CR3:
		*v = env->cr[3];
		break;
	case VXR_CR4:
		*v = env->cr[4];
		break;
	default:
		r = VXERR_FAIL;
	}
	return r;
}
VXError vx64_set_reg(TransReg id, vxlong_t v) {
	uint32_t r = VXERR_OK;
	switch(id) {
	case VXR_EAX:
		env->regs[R_EAX] = v;
		break;
	case VXR_ECX:
		env->regs[R_ECX] = v;
		break;
	case VXR_EDX:
		env->regs[R_EDX] = v;
		break;
	case VXR_EBX:
		env->regs[R_EBX] = v;
		break;
	case VXR_ESP:
		env->regs[R_ESP] = v;
		break;
	case VXR_EBP:
		env->regs[R_EBP] = v;
		break;
	case VXR_ESI:
		env->regs[R_ESI] = v;
		break;
	case VXR_EDI:
		env->regs[R_EDI] = v;
		break;
	case VXR_EFLAGS:
		env->eflags = v;
		break;
	case VXR_EIP:
		env->eip = v;
		break;
	case VXR_CR3:
		cpu_x86_update_cr3(env, v);
		break;
	case VXR_DR0:
		helper_movl_drN_T0(env, 0, v);
		break;
	case VXR_DR1:
		helper_movl_drN_T0(env, 1, v);
		break;
	case VXR_DR2:
		helper_movl_drN_T0(env, 2, v);
		break;
	case VXR_DR3:
		helper_movl_drN_T0(env, 3, v);
		break;
	case VXR_DR6:
		helper_movl_drN_T0(env, 6, v);
		break;
	case VXR_DR7:
		helper_movl_drN_T0(env, 7, v);
		break;
	default:
		r = VXERR_FAIL;
	}
	return r;
}
VXError vx64_load_seg_cache( TransReg id, uint16_t sel, vxvma_t va_base, uint32_t limit, uint32_t flags )
{
	cpu_x86_load_seg_cache(env, id - VXR_ES, sel, va_base, limit, flags);
	return VXERR_OK;
}

VXError vx64_load_xdt( TransReg id, vxvma_t base, uint32_t limit)
{
	switch(id) {
	case VXR_GDT:
		env->gdt.base = base;
		env->gdt.limit = limit;
		break;
	case VXR_LDT:
		env->ldt.base = base;
		env->ldt.limit = limit;
		break;
	default:
		return VXERR_FAIL;
	}
	return VXERR_OK;
}

VXError vx64_go(VXResult* gr) {
	env->icount_extra = 1000 * 1000 * 10;
	int trapno = cpu_x86_exec(env);
	memset(gr, 0, sizeof(*gr));
	if(trapno < 256) {
		gr->result = VXR_TRAP_0 + trapno;
		gr->trapData.errorCode = env->error_code;
		gr->trapData.isINT = env->exception_is_int;
		gr->trapData.nextRIP = env->exception_next_eip;
	} else if(trapno == EXCP_DEBUG) {
		gr->result = VXR_DBG;
		gr->dbgData.isWatchPoint = (env->watchpoint_hit != 0);
		if(env->watchpoint_hit) {
			gr->dbgData.isWatchPoint = 1;
			gr->dbgData.watchPoint.gvma = env->watchpoint_hit->vaddr;
			gr->dbgData.watchPoint.size = ~env->watchpoint_hit->len_mask + 1;
			if(env->watchpoint_hit->flags & BP_MEM_READ)
				gr->dbgData.watchPoint.type |= VXWP_READ;
			if(env->watchpoint_hit->flags & BP_MEM_WRITE)
				gr->dbgData.watchPoint.type |= VXWP_WRITE;
			env->watchpoint_hit = 0;
		} else {
			gr->dbgData.isWatchPoint = 0;
			gr->dbgData.bwpAddress = env->eip;
		}
	} else if(trapno == EXCP_INTERRUPT) {
		gr->result = VXR_NONE;
	} else {
		return VXERR_FAIL;
	}
	env->old_exception = -1;
	env->exception_index = -1;
	return VXERR_OK;
}

VXError vx64_go_step(VXResult* gr) {
	cpu_single_step(env, 1);
	vx64_go(gr);
	cpu_single_step(env, 0);
	if(gr->result == VXR_DBG && !gr->dbgData.isWatchPoint) {
		gr->result = VXR_NONE;
	}
	return VXERR_OK;
}

VXError vx64_dirty(vxpma_t guest_pa, vxpma_t len)
{
	invalidate_and_set_dirty(guest_pa, len);
	return VXERR_OK;
}

VXError vx64_flush_tlb(vxpma_t guest_pa)
{
	tlb_flush_page(env, guest_pa);
	return VXERR_OK;
}

VXError vx64_insert_bp(vxpma_t vmaGuest)
{
	return cpu_breakpoint_insert(env, vmaGuest, BP_GDB, 0) == 0 ? VXERR_OK : VXERR_FAIL;
}
VXError vx64_insert_wp(vxvma_t vmaGuest, vxvma_t size, VXWatchPointType type)
{
	target_ulong flags = 0;
	if(type & VXWP_READ)
		flags |= BP_MEM_READ;
	if(type & VXWP_WRITE)
		flags |= BP_MEM_WRITE;
	if(!flags)
		return VXERR_FAIL;
	flags |= BP_GDB;
	return cpu_watchpoint_insert(env, vmaGuest, size, flags, 0) == 0 ? VXERR_OK : VXERR_FAIL;
}
VXError vx64_remove_bp(vxvma_t vmaGuest)
{
	return cpu_breakpoint_remove(env, vmaGuest, BP_GDB) == 0 ? VXERR_OK : VXERR_FAIL;
}
VXError vx64_remove_wp(vxvma_t vmaGuest, vxvma_t size, VXWatchPointType type)
{
	target_ulong flags = 0;
	if(type & VXWP_READ)
		flags |= BP_MEM_READ;
	if(type & VXWP_WRITE)
		flags |= BP_MEM_WRITE;
	if(!flags)
		return VXERR_FAIL;
	flags |= BP_GDB;
	return cpu_watchpoint_remove(env, vmaGuest, size, flags) == 0 ? VXERR_OK : VXERR_FAIL;
}
VXError vx64_remove_all_bps()
{
	cpu_breakpoint_remove_all(env, BP_GDB);
	return VXERR_OK;
}
VXError vx64_remove_all_wps()
{
	cpu_watchpoint_remove_all(env, BP_GDB);
	return VXERR_OK;
}
VXError vx64_query_bps(vxvma_t* bps, int* len)
{
	int num_copied = 0;
	int total = 0;
	CPUBreakpoint *bp;
	QTAILQ_FOREACH(bp, &env->breakpoints, entry)
	{
		if (bp->flags & BP_GDB)
		{
			++total;
			if(num_copied < *len) bps[num_copied++] = bp->pc;
		}
	}
	*len = total;
	return num_copied == total ? VXERR_OK : VXERR_INSUF_BUF;
}
VXError vx64_query_wps(VXWatchPoint* wps, int* len) {
	int num_copied = 0;
	int total = 0;
	CPUWatchpoint *watchPoint;
	QTAILQ_FOREACH(watchPoint, &env->watchpoints, entry)
	{
	    if (watchPoint->flags & BP_GDB)
	    {
	        ++total;
	        if(num_copied < *len)
	        {
	        	VXWatchPoint twp;
	        	twp.gvma = watchPoint->vaddr;
	        	twp.size = ~watchPoint->len_mask + 1;
	        	twp.type = 0;
	        	if(watchPoint->flags & BP_MEM_READ)
	        		twp.type |= VXWP_READ;
	        	if(watchPoint->flags & BP_MEM_WRITE)
	        		twp.type |= VXWP_WRITE;
	        	wps[num_copied++] = twp;
	        }
	    }
	}
	*len = total;
	return num_copied == total ? VXERR_OK : VXERR_INSUF_BUF;
}
