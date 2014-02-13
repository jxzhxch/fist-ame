#ifndef __VIR_WIN32__
#define __VIR_WIN32__

namespace vin
{
	enum { VM_PAGE_SIZE = 0x1000 };
	enum { VM_PAGE_SHIFT = 12 };

	typedef uint64_t		VirtualAddress;
	typedef VirtualAddress	vma_t;

	namespace vram
	{
		typedef size_t		RamAddress;
		typedef size_t		RamSizeT;
		typedef uint32_t	UInt32;

		enum RamBlockFlags
		{
			BF_VALID	= 1,
			BF_SWAPPED	= 2,
			BF_LOCKED	= 4,
		};

		struct RamBlock;
		
		struct RamBlockOwner
		{
			virtual void onBlockInvalid( RamBlock* pb ) = 0;
		};
		
		struct RamBlock 
		{
			UInt32			flags;
			RamAddress		addr;
			RamAddress		swapid;
			RamBlockOwner*	owner;
		};

		HRESULT Init( RamSizeT size, LPCSTR aPageFileName, RamSizeT growCountOfPageFile = 0x1000 );
		
		HRESULT Destroy();
		
		HRESULT Alloc( RamBlock* pb, UInt32 initFlags );
		
		HRESULT Free( RamBlock* pb );
		
		HRESULT SwapOut( RamBlock* pb, BOOL bForce );
		
		HRESULT SwapIn( RamBlock* pb );
		
		HRESULT Ensure( RamBlock* pb );

		PVOID	Buffer( RamBlock* pb );
	};

	namespace space
	{

		
	};

};


#endif