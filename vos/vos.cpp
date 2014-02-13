// vos.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "vin64.h"
#include "space.h"
#include "kmo.h"

using namespace vin;

int _tmain(int argc, _TCHAR* argv[])
{
	RFAILED( vram::Init( 32*1024*1024, "swapfile.bin" ) );
	vram::RamBlock tmp = {};
	vram::Alloc( &tmp, 0 );

	KmoSection section;
	section.getRamBlock( 0 );

	AddressSpace space;
	space.init( 1LL<<46 );
	size_t i = 0;
	for( ; i < 100000; ++i )
	{
		VMRegion* rgn1 = space.allocSectionAddress( 0, 0x1000, false );
		if( !rgn1 ) break;
		rgn1->attr.u32 |= MEM_PRIVATE|MEM_RESERVE;
		uint32_t oldprot;
		rgn1->vmprotect( rgn1->base, 0x1000, PAGE_READWRITE, &oldprot );

	}
	//for( size_t j = 0; j < i; ++j )
	//{
	//	space.freeSectionAddress( j*0x100000 );
	//}
	space.dump();
	return 0;
}

