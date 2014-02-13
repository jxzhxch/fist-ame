#include "stdafx.h"
#include "vin64.h"
#include "../utils/dlinklist.h"

namespace vin { 

enum {
	WVM_PROTECT_MASK = 0xFFF,
	WVM_TYPE_MASK = 0xF0000,
	WVM_STATE_MASK = 0xF000,
	WVM_EXT_FLAGS_MASK = 0xFFF00000,
};

static inline uint64_t __align_big( uint64_t v, size_t a )
{
	bool pad = ( v % a ) != 0;
	return v/a*a + (pad?a:0);
}

struct VMRegion;

union VMAttr
{
	struct {
		uint32_t		prot:12;
		uint32_t		state:4;
		uint32_t		type:4;
		uint32_t		extend:12;
	} u;
	uint32_t			u32;
};

struct VMBlock : dllist<VMBlock>::node
{
	VMRegion*		_parent;
	VMAttr			attr;
	uint64_t		base;
	uint64_t		size;
	VMBlock( VMRegion* p ) : _parent(p)
	{}
};

struct AddressSpace;

struct VMRegion  : dllist<VMRegion>::node, dllist<VMBlock>
{
	AddressSpace*	space;
	uint64_t		base;				// alloc base
	uint64_t		size;				// alloc size
	VMAttr			attr;				// alloc attr
	void*			storage;
	VMRegion(AddressSpace*sp) : space(sp)
	{
	}
	VMBlock* merge( VMBlock* first, VMBlock* last )
	{
		// merge all next 
		for( ; _first->_next != _last; )
		{
			_first->size += _first->_next->size;
			unlink( _first->_next );
		}
		return first;
	}
	void mergeSameProtect( VMBlock * p )
	{
		// merge all next 
		for( ; p->_next ; )
		{
			uint32_t rp = p->_next->attr.u32 & WVM_PROTECT_MASK;
			uint32_t lp = p->attr.u32 & WVM_PROTECT_MASK;
			if( rp != lp ) break;
			p->size += p->_next->size;
			unlink( p->_next );
		}
		// merge all prev
		for( ; p->_prev ; )
		{
			uint32_t rp = p->_next->attr.u32 & WVM_PROTECT_MASK;
			uint32_t lp = p->attr.u32 & WVM_PROTECT_MASK;
			if( rp != lp ) break;
			p->size += p->_prev->size;
			p->base = p->_prev->base;
			unlink( p->_prev );
		}
	}
	long split( VMBlock* fit, uint64_t want, VMBlock** vb2 )
	{
		vb2[0] = fit;
		vb2[1] = 0;
		if( want == fit->size )
			return 0;
		VMBlock* ret  = 0;
		VMBlock* cut = new VMBlock(this);
		if( !cut ) return false;
		*cut = *fit;
		// this -> p -> o
		cut->base = fit->base + want;
		cut->size = fit->size - want;
		fit->size = want;
		// link
		linkNext( fit, cut );
		vb2[1] = cut;
		return true;
	}
	VMBlock* findBlockL2R( uint64_t base, bool equl, VMBlock * first = 0 )
	{
		VMBlock * fit = first ? first : _first;
		for( ; fit; fit = fit->_next )
		{
			if( fit->base > base ) break;
			if( equl ) 
			{
				if( fit->base != base ) continue;
			}
			else
			{
				uint64_t fitend = fit->base + fit->size;
				if( fitend <= base ) continue;
			}
			return fit;
		}
		return NULL;
	}
	VMBlock* findBlockR2L( uint64_t base, bool equl, VMBlock * last = 0 )
	{
		VMBlock * fit = last ? last : _last;
		for( ; fit; fit = fit->_prev )
		{
			if( fit->base > base ) break;
			if( equl ) 
			{
				if( fit->base != base ) continue;
			}
			else
			{
				uint64_t fitend = fit->base + fit->size;
				if( fitend <= base ) continue;
			}
			return fit;
		}
		return NULL;
	}
	void initBlock()
	{
		if( !_first )
		{
			VMBlock* p = new VMBlock(this);
			p->base = base;
			p->attr = attr;
			p->size = size;
			setFirst(p);
		}
	}
	long vmprotect( uint64_t base, uint64_t size, uint32_t prot, uint32_t * oldprot )
	{
		prot &= WVM_PROTECT_MASK;
		initBlock();
		uint64_t tbase = base + size - 1;
		VMBlock* first = findBlockL2R( base, false );
		if( !first ) return -1;
		*oldprot = first->attr.u32 & WVM_PROTECT_MASK;
		uint64_t gap = base - first->base;
		if( gap )
		{
			VMBlock* vm2[2] = {};
			split( first, gap, vm2);
			first = vm2[1];
		}
		VMBlock* last = findBlockL2R( tbase, false, first );
		if( !last ) return -1;
		gap = last->base + last->size - tbase - 1;
		if( gap )
		{
			VMBlock* vm2[2] = {};
			split( last, last->size - gap, vm2 );
			last = vm2[0];
		}
		if( first != last ) 
			first = merge( first, last->_next );

		first->attr.u32 &= ~WVM_PROTECT_MASK;
		first->attr.u32 |= prot;

		mergeSameProtect(first);

		return 0;
	}
	long vmfree( uint64_t base, uint64_t size )
	{
		initBlock();
		uint64_t tbase = base + size - 1;
		VMBlock* first = findBlockL2R( base, false );
		VMBlock* last = findBlockL2R( tbase, false, first );
		uint64_t gap = base - first->base;
		if( gap )
		{
			VMBlock* vm2[2] = {};
			split( first, gap, vm2);
			first = vm2[1];
		}
		gap = tbase - last->base;
		if( gap )
		{
			VMBlock* vm2[2] = {};
			split( first, gap, vm2);
			last = vm2[0];
		}
		if( first != last ) 
			first = merge( first, last->_next );

		first->attr.u32 &= ~WVM_STATE_MASK;
		first->attr.u32 |= MEM_RESERVE;
		return 0;
	}
};

struct AddressSpace : dllist<VMRegion>
{
	virtual long init( uint64_t room )
	{
		VMRegion* blk = new VMRegion(this);
		blk->base = 0;
		blk->size = room;
		blk->attr.u32 = MEM_FREE|PAGE_NOACCESS;
		setFirst(blk);
		return S_OK;
	};
	enum { VMBLOCK_AT_ALIGN = 0x10000 };
	enum { VMBLOCK_SIZE_ALIGN = 0x1000 };
public:

public:
	bool split( VMRegion* fit, uint64_t want, bool top, VMRegion** vm2 )
	{
		VMRegion* ret  = 0;
		VMRegion* cut = new VMRegion(this);
		if( !cut ) return false;
		if( top ) want = fit->size - want;
		// this -> p -> o
		cut->base = fit->base + want;
		cut->size = fit->size - want;
		cut->attr.u32 = fit->attr.u32;
		cut->storage = fit->storage;
		fit->size = want;
		// link
		linkNext( fit, cut );
		vm2[0] = fit;
		vm2[1] = cut;
	
		return true;
	}
	VMRegion* xfrom( bool top )
	{
		return top ? _last : _first;
	}
	VMRegion* xnext( bool top, VMRegion* p )
	{
		return top ? p->_prev : p->_next;
	}

	void dump( )
	{
		VMRegion * fit = xfrom(false);
		for( ; fit; fit = xnext(false, fit) )
		{
			printf( "base=%I64X, size=%I64X, attr=%08X, state=%08X, protect=%08X\n", fit->base, fit->size, fit->attr.u32 );
			VMBlock* block = fit->_first;
			for( ; block; block = block->_next )
			{
				printf( "\tbase=%I64X, size=%I64X, attr=%08X, protect=%08X\n", block->base, block->size, block->attr.u32 );
			}
		}
	}

	VMRegion * findSectionAddress( uint64_t base, bool equl = false )
	{
		bool top = false;
		VMRegion * fit = xfrom(top);
		for( ; fit; fit = xnext(top, fit) )
		{
			if( fit->base > base ) break;
			if( equl ) 
			{
				if( fit->base != base ) continue;
			}
			else
			{
				uint64_t fitend = fit->base + fit->size;
				if( fitend <= base ) continue;
			}
			return fit;
		}
		return NULL;
	}

	long freeSectionAddress( uint64_t base )
	{
		if( base & (VMBLOCK_AT_ALIGN-1) ) return -1;
		VMRegion * fit = findSectionAddress( base, true );
		if( !fit ) return -2;
		return freeSectionAddress(fit);
	}

	long freeSectionAddress( VMRegion * sec )
	{
		uint32_t type = sec->attr.u32 & WVM_TYPE_MASK;
		uint32_t state = sec->attr.u32 & WVM_STATE_MASK;
		if( !type || state == MEM_FREE ) 
			return -1;
		// merge all next 
		for( ; sec->_next ; )
		{
			if( 0 == (sec->_next->attr.u32 & MEM_FREE) ) break;
			sec->size += sec->_next->size;
			unlink( sec->_next );
		}
		// merge all prev
		for( ; sec->_prev ; )
		{
			if( 0 == (sec->_next->attr.u32 & MEM_FREE) ) break;
			if( sec->_prev->base & (VMBLOCK_AT_ALIGN-1) ) break;
			sec->size += sec->_prev->size;
			sec->base = sec->_prev->base;
			unlink( sec->_prev );
		}
		sec->attr.u32 = PAGE_NOACCESS|MEM_FREE;
		sec->destroy();
		return 0;
	}

	VMRegion * allocSectionAddress( uint64_t base, uint64_t size, bool top )
	{
		if( base & (VMBLOCK_AT_ALIGN-1) ) return NULL;
		size = __align_big(size,VMBLOCK_SIZE_ALIGN);
		uint64_t required = __align_big( size, VMBLOCK_AT_ALIGN );
		if( !_first ) return NULL;
		top = base ? false : top;
		VMRegion * fit = xfrom(top);
		for( ; fit; fit = xnext(top, fit) )
		{
			if( (fit->attr.u32 & WVM_TYPE_MASK) != MEM_FREE ) continue;
			if( fit->base & 0xFFFF ) continue;	// it's gap!
			if( fit->size < required ) continue;
			
			if( base )
			{
				if( fit->base > base ) break;
				uint64_t fitend = fit->base + fit->size;
				if( fitend < base ) continue;
				uint64_t wantend = base + required;
				if( fitend <= wantend ) continue;
				uint64_t baseoff = base - fit->base;
				if( baseoff )
				{
					VMRegion* splited[2];
					if( !split( fit, baseoff, false, splited ) ) return NULL;
					fit = splited[1];
				}
			}

			VMRegion* splited[2];
			if( !split( fit, required, top, splited ) ) return NULL;
			fit = top ? splited[1] : splited[0];

			if( required > size )
			{
				VMRegion* splited[2];
				if( !split( fit, size, false, splited ) ) return NULL;
				fit = splited[0];
			}
			return fit;
		}
		return NULL;
	}
	void destroy()
	{
		VMRegion* p = _first;
		VMRegion* tmp = 0;
		for( ; p; p = tmp )
		{
			tmp = p->_next;
			delete p;
		}
		_first = 0;
		_last = 0;
	}
};


};

