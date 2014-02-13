#ifndef LIB_TRANS__H_
#define LIB_TRANS__H_

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif

typedef uint64_t 		vxvma_t;
typedef uint64_t		vxpma_t;
typedef vxvma_t			vxlong_t;

typedef enum _VXError {
	VXERR_INSUF_BUF = -2,
	VXERR_FAIL = -1,
	VXERR_OK = 0,
} VXError;

typedef enum _TransReg {
	VXR_EAX = 0,
	VXR_ECX,
	VXR_EDX,
	VXR_EBX,
	VXR_ESP,
	VXR_EBP,
	VXR_ESI,
	VXR_EDI,

	VXR_EFLAGS,
	VXR_EIP,

	VXR_ES = 0x10,
	VXR_CS,
	VXR_SS,
	VXR_DS,
	VXR_FS,
	VXR_GS,

	VXR_DR0 = 0x20,
	VXR_DR1,
	VXR_DR2,
	VXR_DR3,
	VXR_DR6,
	VXR_DR7,

	VXR_CR0 = 0x30,
	VXR_CR2 = 0x32,
	VXR_CR3,
	VXR_CR4,

	VXR_GDT = 0x100,
	VXR_LDT = 0x101
} TransReg;

typedef enum _VXSegDescMask {
	// 和x86段描述符的高32位各位一一对应
	TSEG_DESC_G_MASK = (1 << 23),
	TSEG_DESC_B_MASK = (1 << 22),
	TSEG_DESC_L_MASK = (1 << 21), /* x64 only */
	TSEG_DESC_AVL_MASK = (1 << 20),
	TSEG_DESC_P_MASK = (1 << 15),
	TSEG_DESC_DPL_MASK = (3 << 13),
	TSEG_DESC_S_MASK = (1 << 12),
	TSEG_DESC_TYPE_MASK = (15 << 8),
	TSEG_DESC_A_MASK = (1 << 8),
	TSEG_DESC_CS_MASK = (1 << 11), /* 1=code segment 0=data segment */
	TSEG_DESC_C_MASK = (1 << 10), /* code: conforming */
	TSEG_DESC_R_MASK = (1 << 9),  /* code: readable */
	TSEG_DESC_E_MASK = (1 << 10), /* data: expansion direction */
	TSEG_DESC_W_MASK = (1 << 9)  /* data: writable */
} VXSegDescMask;

typedef enum _VXWatchPointType {
	VXWP_READ = 0x1,
	VXWP_WRITE = 0x2,
} VXWatchPointType;

typedef struct _VXWatchPoint
{
	VXWatchPointType	type;
	vxvma_t				gvma;
	vxlong_t			size;
} VXWatchPoint;

typedef struct _VXResult {
	enum {
		VXR_TRAP_0 = 0,
		VXR_TRAP_1,
		VXR_TRAP_2,
		VXR_TRAP_3,
		VXR_TRAP_4,
		VXR_TRAP_5,
		VXR_TRAP_6,
		VXR_TRAP_7,
		VXR_TRAP_8,
		VXR_TRAP_9,
		VXR_TRAP_A,
		VXR_TRAP_B,
		VXR_TRAP_C,
		VXR_TRAP_D,
		VXR_TRAP_E,
		VXR_TRAP_F,
		VXR_DBG = 256,
		VXR_NONE,
	} result;
	union {
		struct
		{
			uint32_t			errorCode;
			int					isINT;
			vxvma_t				nextRIP;
		} trapData;
		struct
		{
			int					isWatchPoint;
			union
			{
				VXWatchPoint	watchPoint;
				vxvma_t			bwpAddress;
			};
		} dbgData;
	};
} VXResult;


typedef uint64_t (*rdtsc_helper_routine)(void* extra);

VXError vx64_init();
VXError vx64_reset();
// can't safely destroy yet!
// VXError vx64_destroy, void)();

VXError vx64_setup_rdtsc_helper(rdtsc_helper_routine helper, void* extra);

void* 	vx64_setup_ram(vxpma_t gpma, uint32_t size, void* ptr);

void	vx64_remove_ram(void*);

VXError vx64_get_reg(TransReg id, vxlong_t* v);
VXError vx64_set_reg(TransReg id, vxlong_t v);

VXError vx64_load_seg_cache(TransReg id, uint16_t sel, vxvma_t va_base, uint32_t limit, uint32_t flags);

VXError vx64_load_xdt(TransReg id, vxvma_t base, uint32_t limit);

VXError vx64_go(VXResult*);
VXError vx64_go_step(VXResult*);

VXError vx64_dirty(vxpma_t gpma, vxpma_t len);

VXError vx64_flush_tlb(vxpma_t gpma);

VXError vx64_insert_bp(vxvma_t gvma);
VXError vx64_insert_wp(vxvma_t gvma, vxvma_t size, VXWatchPointType type);
VXError vx64_remove_bp(vxvma_t gvma);
VXError vx64_remove_all_bps();
VXError vx64_remove_wp(vxvma_t gvma, vxvma_t size, VXWatchPointType type);
VXError vx64_remove_all_wps();
VXError vx64_query_bps(vxvma_t* bps, int* len);
VXError vx64_query_wps(VXWatchPoint* wps, int* len);


#endif // LIB_TRANS__H_
