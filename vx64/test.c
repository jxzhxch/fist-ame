#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include "vx64.h"

enum { RAM_SIZE = 16 * 1024 * 1024 };

int main()
{
	vx64_init();

	void * ram_base = (uint8_t*)_aligned_malloc(4096, RAM_SIZE);

	memset( ram_base, 0, sizeof(RAM_SIZE) );

	vx64_setup_ram(0, RAM_SIZE, ram_base);

	vxlong_t eip = 0x123456789LL;

	vx64_set_reg( VXR_EIP, eip );

	vx64_insert_wp( 0x123450000LL, 0x1000, VXWP_READ );

	VXResult gr;
	vx64_go_step(&gr);
	return 0;
}
