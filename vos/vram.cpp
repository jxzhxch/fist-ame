#include "stdafx.h"
#include "vin64.h"
#include "../utils/bitmap.h"
#include <vector>

namespace vin { namespace vram {

	typedef RamBlock*					RamBlockPtr;
	typedef std::vector<RamBlockPtr>	RamBlockPtrs;
	typedef UTIL::sentry<UINT8*>		FlatRam;

	enum { VM_PAGE_SIZE = 0x1000 };

	class RamManager
	{
		RamBlockPtrs	mBlocks;
		FlatRam			mRamData;
		UINT8			mTemp[VM_PAGE_SIZE];
		HANDLE			mStoageFile;
		XBitMap			mBlockBitMap;
		SIZE_T			mGrowCount;
	public:
		RamManager( BOOL bCreateInTempDir = TRUE )
			: mStoageFile(NULL)
			, mGrowCount(0)
		{
			return;
			//if( bCreateInTempDir )
			//{
			//	DWORD chPath = GetTempPathA( 0, NULL );
			//	if( !chPath ) return ;
			//	std::vector<CHAR> aTempDir(chPath+1);
			//	GetTempPathA( (DWORD)aTempDir.size(), &aTempDir[0] );
			//	CHAR szFileName[64] = {};
			//	sprintf( szFileName, "\\Process-%08X.bsf", GetCurrentProcessId() );
			//	std::string aFileName = &aTempDir[0];
			//	aFileName.append( szFileName );
			//	Create( aFileName.c_str(), 0x4000 );
			//}
		}

		~RamManager()
		{
			TryReduce();
			Close();
		}
		FORCEINLINE VOID TryReduce()
		{
			return;
		}
		// 64M
		HRESULT Create( LPCSTR aFileName, SIZE_T aFlatPages = 0x2000, SIZE_T aGrowBlocks = 0x1000 )
		{
			if( !aFileName || !aFlatPages || !aGrowBlocks )
				return E_INVALIDARG;
			SIZE_T cbFlatMem = aFlatPages * VM_PAGE_SIZE;
			mRamData = new UINT8[cbFlatMem];
			if( !mRamData ) return E_OUTOFMEMORY;
			memset( mRamData.m_p, 0, cbFlatMem );
			mBlocks.resize( aFlatPages );
			HANDLE hFile = CreateFileA( aFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );
			if( hFile == INVALID_HANDLE_VALUE ) return HRESULT_FROM_WIN32(::GetLastError());
			mStoageFile = hFile;
			mGrowCount = aGrowBlocks;
			return S_OK;
		}

		VOID Close()
		{
			if( mStoageFile ) 
				CloseHandle( mStoageFile );
			mStoageFile = NULL;
			mBlockBitMap.clear();
			mGrowCount = (0);
			mRamData.dispose();
			mBlocks.clear();
		}

		SIZE_T	RamSize()
		{
			return mBlocks.size() * VM_PAGE_SIZE;
		}

		LPVOID	RamHostBase()
		{
			return mRamData;
		}

		HRESULT RamAllocAddress( RamBlockPtr pb, uint32_t flags )
		{
			flags &= BF_LOCKED;
			RamSizeT i = 0;

			RFAILED( FindFreeOrSwappableAddress(i,rand()) );
			RFAILED( SwapRamAndPageFile( i, 0 ) );

			pb->addr = i * VM_PAGE_SIZE;
			pb->flags = flags | BF_VALID;
			pb->swapid = 0;
			mBlocks[i] = pb;

			return S_OK;
		}

		//HRESULT RamInvalidAddress( RamBlockPtr pb )
		//{
		//	if( ! pb->flags ) return S_OK;

		//	if( pb->flags & BF_LOCKED )
		//		return E_UNEXPECTED;

		//	if( pb->flags & BF_SWAPPED )
		//	{
		//		PageFileMarkUsed(pb->swapid, FALSE);
		//		return S_OK;
		//	}

		//	if( pb->owner )
		//		pb->owner->onBlockInvalid( pb );

		//	pb->flags = 0;
		//}

		// always save to page file.
		HRESULT RamFreeAddress( RamBlockPtr pb )
		{
			if( ! pb->flags & BF_VALID )
				return S_OK;

			if( pb->flags & BF_SWAPPED )
			{
				//
				//	当前页在PageFile中，则只需要释放掉PageFile中的页就行
				//

				RFAILED( PageFileMarkUsed( pb->swapid, FALSE ) );
			}
			else
			{
				//
				//	当前页在Ram中，则需要告知页面地址无效，并释放RAM地址
				//

				RamSizeT i = pb->addr/VM_PAGE_SIZE;

				if( pb->owner )
					pb->owner->onBlockInvalid( pb );

				mBlocks[i] = 0;
			}
			pb->flags = 0;
			pb->addr = 0;
			pb->swapid = 0;
			return S_OK;
		}

		// always save to page file.
		HRESULT RamSwapOut( RamBlockPtr pb, BOOL bForce )
		{
			if( pb->flags & BF_SWAPPED ) 
				return S_OK;

			// oops, can't save locked block
			if( (pb->flags & BF_LOCKED) && !bForce )
				return E_FAIL;

			RamSizeT indexPF = 0;
			RFAILED( PageFilePreAlloc( indexPF ) );
			RFAILED( StoreToStorage( indexPF, GetRamBuffer(pb->addr) ) );
			PageFileMarkUsed( indexPF, TRUE );

			if( pb->owner ) 
				pb->owner->onBlockInvalid( pb );

			pb->swapid = indexPF;
			pb->flags |= BF_SWAPPED;

			RamSizeT index = pb->addr/VM_PAGE_SIZE;
			mBlocks[index] = 0;

			return S_OK;
		}
		HRESULT RamSwapIn( RamBlockPtr pb )
		{
			if( ! (pb->flags & BF_SWAPPED) )
				return E_FAIL;

			RamSizeT indexRam = 0;
			if( pb->flags & BF_LOCKED )
			{
				// locked block must load to alloc-base
				indexRam = pb->addr >> VM_PAGE_SHIFT;
			}
			else
			{
				RFAILED( FindFreeOrSwappableAddress( indexRam, rand() ) );
			}

			RFAILED( SwapRamAndPageFile( indexRam, pb ) );

			return S_OK;
		}
		HRESULT RamDirectAccess( RamBlockPtr pb, PVOID* pp, PVOID ownerBuffer = 0 )
		{
			if( !pb->flags ) return E_UNEXPECTED;
			if( pb->flags & BF_SWAPPED )
			{
				if( ownerBuffer ) 
				{
					RFAILED( LoadFromStorage( pb->swapid, ownerBuffer ) );
					*pp = ownerBuffer;
					return S_OK;
				}
				RFAILED( RamSwapIn(pb) );
			}
			*pp = GetRamBuffer(pb->addr );
			return S_OK;
		}
		HRESULT RamDirectWrite( RamBlockPtr pb, SIZE_T off, PVOID p, SIZE_T cb )
		{
			if( off + cb > VM_PAGE_SIZE ) return E_FAIL;
			PVOID dataBuffer = 0;
			RFAILED( RamDirectAccess( pb, &dataBuffer, mTemp ) );
			memcpy( (char*)dataBuffer + off, p, cb );
			if( 0 == (pb->flags & BF_SWAPPED) )
				return S_OK;
			return StoreToStorage( pb->swapid, dataBuffer );
		}
		HRESULT RamDirectRead( RamBlockPtr pb, SIZE_T off, PVOID p, SIZE_T cb )
		{
			if( off + cb > VM_PAGE_SIZE ) return E_FAIL;
			PVOID dataBuffer = 0;
			RFAILED( RamDirectAccess( pb, &dataBuffer, mTemp ) );
			memcpy( p, (char*)dataBuffer + off, cb );
			return S_OK;
		}

		PVOID GetRamBuffer( RamSizeT addr )
		{
			return mRamData + addr;
		}

		FORCEINLINE HRESULT EnsureRamBlock( RamBlockPtr pb )
		{
			if( !pb->flags ) return E_UNEXPECTED;

			if( !(pb->flags & BF_SWAPPED) ) return S_OK;

			RamSizeT addrIndex = 0;

			RFAILED( FindFreeOrSwappableAddress( addrIndex, rand() ) );
			
			return ( SwapRamAndPageFile( addrIndex, pb ) );				
		};


	protected:

		FORCEINLINE HRESULT PageFilePreAlloc( RamSizeT & rBlockIdx )
		{
			if( !mBlockBitMap.find0(false,rBlockIdx) )
			{
				rBlockIdx = mBlockBitMap.bits();
				mBlockBitMap.resize( rBlockIdx + mGrowCount );
			}
			return S_OK;
		}
		FORCEINLINE HRESULT PageFileMarkUsed( RamSizeT uBlockId, BOOL bUsed )
		{
			return MarkSwapBlockUsed( uBlockId, bUsed );
		}
		FORCEINLINE HRESULT MarkSwapBlockUsed( UINT64 rBlockIdx, BOOL bUsed )
		{
			if( rBlockIdx >= mBlockBitMap.bits() )
				return E_UNEXPECTED;
			mBlockBitMap.setbit((RamSizeT)rBlockIdx,bUsed);
			return S_OK;
		}
		FORCEINLINE VOID	CodeBlockPage( PVOID oBlock )
		{
			UINT64* pU64 = (UINT64*)(oBlock);
			for( RamSizeT i = 0; i < VM_PAGE_SIZE/sizeof(UINT64); ++ i )
			{
				pU64[i] ^= 0x1982120119821201LL;
			}
		}

		HRESULT SwapRamAndPageFile( RamSizeT idxRam, RamBlockPtr inPF )
		{
			RamBlockPtr inMem = mBlocks[idxRam];
			if( !inMem && !inPF ) return S_OK;

			//
			//	无法将锁定的页交换到页面文件中
			//

			if( inMem && inMem->flags & BF_LOCKED )
				return E_FAIL;

			//
			//	获得PageFile的页面Index
			//	如果PF不存在，则需要自动分配一个
			//

			RamSizeT idxPageFile = 0;
			if( inPF )
			{
				idxPageFile = inPF->swapid;
			}
			else
			{
				RFAILED( PageFilePreAlloc( idxPageFile ) );
			}

			PVOID dataBuffer = inMem ? mTemp : GetRamBuffer(idxRam*VM_PAGE_SIZE);
			PVOID ramBuffer = GetRamBuffer(idxRam<<VM_PAGE_SHIFT);

			if( inPF )
			{
				RFAILED( LoadFromStorage( idxPageFile, dataBuffer ) );
			}
			if( inMem )
			{
				RFAILED( StoreToStorage( idxPageFile, ramBuffer ) );
				PageFileMarkUsed( idxPageFile, TRUE );

				if( inMem->owner )
					inMem->owner->onBlockInvalid( inMem );

				inMem->swapid = idxPageFile;
				inMem->flags |= BF_SWAPPED;

				if( inPF )
					memcpy( ramBuffer, dataBuffer, VM_PAGE_SIZE );
				else
					memset( ramBuffer, 0, VM_PAGE_SIZE );
			}
			if( inPF )
			{
				inPF->addr = idxRam << VM_PAGE_SHIFT;
				inPF->flags &= ~BF_SWAPPED;
				inPF->swapid = 0;
			}
			mBlocks[idxRam] = inPF;
			return S_OK;
		}

		FORCEINLINE HRESULT FindSwappableAddress( RamSizeT& rFreeIndex )
		{
			RamSizeT i = 0;
			for( ; i < mBlocks.size(); ++ i )
			{
				RamBlockPtr pb = mBlocks[i];
				if( !pb )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT FindFreeAddress( RamSizeT& rFreeIndex )
		{
			RamSizeT i = 0;
			for( ; i < mBlocks.size(); ++ i )
			{
				RamBlockPtr pb = mBlocks[i];
				if( !pb )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}
		FORCEINLINE HRESULT FindSwappableAddress( RamSizeT & rFreeIndex, RamSizeT seed )
		{
			RamSizeT i = 0;
			for( ; i < mBlocks.size(); ++ i )
			{
				RamBlockPtr pb = mBlocks[i];
				if( !pb )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT FindFreeOrSwappableAddress( RamSizeT & rFreeIndex, RamSizeT seed )
		{
			srand( seed /*clock()*/ );
			RamSizeT maxRefs = mBlocks.size();
			RamSizeT swapid = maxRefs;
			RamSizeT check = 0;
			for( RamSizeT i = 0; i < maxRefs; ++ i, check = (rand()%100) )
			{
				RamBlockPtr pb = mBlocks[i];
				if( !pb )
				{
					// yeah, it's free!
					rFreeIndex = i;
					return S_OK;
				}

				// oops, can't swap LOCKED address
				if( pb->flags & BF_LOCKED )
					continue;

				// let's ramdon
				if( check < 50 ) swapid = i;
			}

			// just find a swappable address
			if( swapid < maxRefs )
			{
				rFreeIndex = swapid;
				return S_OK;
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT StoreToStorage( RamSizeT index, PVOID oBlock )
		{
			UINT64 uOffset =  index; uOffset <<= VM_PAGE_SHIFT;
			LARGE_INTEGER liDist; liDist.QuadPart = uOffset;
			RASSERT( SetFilePointerEx( mStoageFile, liDist, NULL, SEEK_SET ), E_FAIL );
			CodeBlockPage( oBlock );
			DWORD dwWritten = 0;
			RASSERT( WriteFile( mStoageFile, (oBlock), VM_PAGE_SIZE, &dwWritten, NULL ), E_FAIL );
			if( dwWritten != VM_PAGE_SIZE ) return E_FAIL;
			//mStoreCount ++;
			return S_OK;
		}
		FORCEINLINE HRESULT LoadFromStorage( RamSizeT index, PVOID oBlock )
		{
			UINT64 uOffset =  index; uOffset *= VM_PAGE_SIZE;
			LARGE_INTEGER liDist; liDist.QuadPart = uOffset;
			RASSERT( SetFilePointerEx( mStoageFile, liDist, NULL, SEEK_SET ), E_FAIL );
			DWORD dwRead = 0;
			RASSERT( ReadFile( mStoageFile, (oBlock), VM_PAGE_SIZE, &dwRead, NULL ), E_FAIL );
			if( dwRead != VM_PAGE_SIZE ) return E_FAIL;
			CodeBlockPage( oBlock );
			//mLoadCount ++;
			return S_OK;
		}
	}; 

	static RamManager GlobalRamManager;

	HRESULT Init( RamSizeT size, LPCSTR aPageFileName, RamSizeT growCountOfPageFile )
	{
		return GlobalRamManager.Create( aPageFileName, size / VM_PAGE_SIZE, growCountOfPageFile );
	}
	
	HRESULT Destroy()
	{
		GlobalRamManager.Close();
		return S_OK;
	}
	
	HRESULT Alloc( RamBlock* pb, UInt32 initFlags )
	{
		return GlobalRamManager.RamAllocAddress( pb, initFlags );
	}
	
	HRESULT Free( RamBlock* pb )
	{
		return GlobalRamManager.RamFreeAddress( pb );
	}
	
	HRESULT SwapOut( RamBlock* pb, BOOL bForce )
	{
		return GlobalRamManager.RamSwapOut( pb, bForce );
	}
	
	HRESULT SwapIn( RamBlock* pb )
	{
		return GlobalRamManager.RamSwapIn( pb );
	}
	
	HRESULT Ensure( RamBlock* pb )
	{
		return GlobalRamManager.EnsureRamBlock( pb );
	}

	PVOID Buffer( RamBlock* pb )
	{
		if( 0 == (pb->flags & BF_VALID) )
			return NULL;
		if( pb->flags & BF_SWAPPED )
			return NULL;
		return GlobalRamManager.GetRamBuffer(pb->addr);
	}

}; // namespace vram 
}; //namespace vin
