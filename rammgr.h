// virtfile.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <map>
#include "os.h"
#include "osfop.h"

namespace vm
{
	struct XBitMap
	{
		typedef std::vector<ULONG>	ULONGVEC;
		ULONGVEC			_buf;
		size_t				_bits;

		enum { BITS_PER_LONG = sizeof(ULONG)*8 };

		XBitMap() : _bits(0) {}

		FORCEINLINE size_t bits()
		{
			return _bits;
		}

		FORCEINLINE void clear()
		{
			_buf.clear();
			_bits = 0;
		}

		FORCEINLINE bool resize( size_t bits )
		{
			size_t numl = bits/BITS_PER_LONG + ((bits%BITS_PER_LONG)?1:0);
			_buf.resize( numl );
			_bits = bits;
			return true;
		}

		FORCEINLINE BOOL testbit( size_t idx )
		{
			if( idx >= _bits ) return FALSE;
			size_t li = idx/BITS_PER_LONG;
			size_t bi = idx%BITS_PER_LONG;
			return _buf[li] & (1<<bi);
		}
		FORCEINLINE void setbit( size_t idx, BOOL f )
		{
			size_t li = idx/BITS_PER_LONG;
			size_t bi = idx%BITS_PER_LONG;
			if( f ) _buf[li] |= (1<<bi);
			else _buf[li] &= ~(1<<bi);
		}
		FORCEINLINE BOOL find0( bool f, size_t & idx )
		{
			size_t li = _bits/BITS_PER_LONG;
			size_t bi = _bits%BITS_PER_LONG;
			for( size_t l = 0; l < li; ++ l )
			{
				if( _buf[l] == (ULONG)-1 ) continue; 
				ULONG ul = _buf[l];
				for( size_t i = 0; i < BITS_PER_LONG; ++ i )
				{
					if( ul & (1<<i) ) continue;
					idx = l * BITS_PER_LONG + i;
					return true;
				}
			}
			if( bi )
			{
				ULONG ul = _buf[li];
				for( size_t i = 0; i < bi; ++ i )
				{
					if( ul & (1<<i) ) continue;
					idx = li * BITS_PER_LONG + i;
					return true;
				}
			}

			return false;
		}

		// FIXME!
		BOOL findLast1( SIZE_T & index, size_t piece = 2 )
		{
			piece = piece == 0 ? 1 : piece;
			size_t li = _buf.size();
			size_t bpp = li/piece;

			for( size_t l = 0; l < bpp; ++ l )
			{
				size_t idx = li - l - 1;
				if( _buf[idx] == 0 ) continue; 
				ULONG ul = _buf[l];
				for( size_t i = 0; i < BITS_PER_LONG; ++ i )
				{
					size_t ibit = BITS_PER_LONG - i - 1;
					if( ul & (1<<ibit) ) continue;
					index = l * BITS_PER_LONG + i;
					return true;
				}
			}
			return false;
		}
	};

	enum BLOCK_FLAGS
	{
		BF_VALID = 1,
		BF_SWAPPED = 2,
		BF_LOCKED = 4,
	};

	typedef uint32_t	RamAddress;
	typedef size_t		PageFileIndex;

	struct AddressRef;

	struct IAddressOwner
	{
		virtual void onAddressInvalid( AddressRef* pref ) = 0;
		//virtual void onBlockInvalid( AddressRef* pref ) = 0;
	};

	struct AddressRef
	{
		uint32_t		flags;	// BLOCK_FLAGS
		RamAddress		addr;	//8
		PageFileIndex	pfidx;	//4 8
		IAddressOwner *	owner;	//4	8
	};

	typedef AddressRef*					AddressRefPtr;
	typedef std::vector<AddressRefPtr>	AddressRefs;
	typedef UTIL::sentry<UINT8*>		FlatRam;

	enum { VM64_PAGE_SIZE = 0x1000 };

	class RamManager
	{
		ost::FastMutex	mMutex;
		AddressRefs		mAddressRefs;
		FlatRam			mRamData;
		UINT8			mTemp[VM64_PAGE_SIZE];
		hfile_t			mStoageFile;
		XBitMap			mBlockBitMap;
		SIZE_T			mGrowCount;
	public:
		RamManager( BOOL bCreateInTempDir = TRUE )
			: mStoageFile(INVALID_HFILE)
			, mGrowCount(0)
		{
			mMutex.init(0,0);
			if( bCreateInTempDir )
			{
				DWORD chPath = GetTempPathA( 0, NULL );
				if( !chPath ) return ;
				std::vector<CHAR> aTempDir(chPath+1);
				GetTempPathA( (DWORD)aTempDir.size(), &aTempDir[0] );
				CHAR szFileName[64] = {};
				sprintf( szFileName, "\\Process-%08X.bsf", GetCurrentProcessId() );
				std::string aFileName = &aTempDir[0];
				aFileName.append( szFileName );
				Create( aFileName.c_str(), 0x4000 );
			}
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
			ost::MutexScope _SYNC_(mMutex);
			if( !aFileName || !aFlatPages || !aGrowBlocks )
				return E_INVALIDARG;
			SIZE_T cbFlatMem = aFlatPages * VM64_PAGE_SIZE;
			mRamData = new UINT8[cbFlatMem];
			if( !mRamData ) return E_OUTOFMEMORY;
			memset( mRamData.m_p, 0, cbFlatMem );
			mAddressRefs.resize( aFlatPages );
			hfile_t hFile = create_file( aFileName, true, true );
			if( hFile == INVALID_HFILE ) return E_FAIL;
			mStoageFile = hFile;
			mGrowCount = aGrowBlocks;
			return S_OK;
		}

		VOID Close()
		{
			ost::MutexScope _SYNC_(mMutex);
			if( mStoageFile != INVALID_HFILE ) 
				close_file( mStoageFile );
			mStoageFile = (INVALID_HFILE);
			mBlockBitMap.clear();
			mGrowCount = (0);
			mRamData.dispose();
			mAddressRefs.clear();
		}

		SIZE_T	RamSize()
		{
			return mAddressRefs.size() * VM64_PAGE_SIZE;
		}

		LPVOID	RamHostBase()
		{
			return mRamData;
		}

		HRESULT RamAllocAddress( AddressRef* pref, uint32_t flags )
		{
			flags &= BF_LOCKED;
			size_t i = 0;
			
			ost::MutexScope _SYNC_(mMutex);

			RFAILED( FindFreeOrSwappableAddress(i,rand()) );
			RFAILED( SwapRamAndPageFile( i, 0 ) );

			pref->addr = i * VM64_PAGE_SIZE;
			pref->flags = flags | BF_VALID;
			pref->pfidx = 0;
			mAddressRefs[i] = pref;

			return S_OK;
		}

		// always save to page file.
		HRESULT RamFreeAddress( AddressRef* pref )
		{
			if( ! pref->flags & BF_VALID )
				return S_OK;

			ost::MutexScope _SYNC_(mMutex);

			if( pref->flags & BF_SWAPPED )
			{
				//
				//	当前页在PageFile中，则只需要释放掉PageFile中的页就行
				//

				RFAILED( PageFileMarkUsed( pref->pfidx, FALSE ) );
			}
			else
			{
				//
				//	当前页在Ram中，则需要告知页面地址无效，并释放RAM地址
				//

				size_t i = pref->addr/VM64_PAGE_SIZE;

				if( pref->owner )
					pref->owner->onAddressInvalid( pref );

				mAddressRefs[i] = 0;
			}
			pref->flags = 0;
			pref->addr = 0;
			pref->pfidx = 0;
			return S_OK;
		}

		// always save to page file.
		HRESULT RamSwapOut( AddressRefPtr pref, BOOL bForce )
		{
			if( pref->flags & BF_SWAPPED ) 
				return S_OK;

			// oops, can't save locked block
			if( (pref->flags & BF_LOCKED) && !bForce )
				return E_FAIL;

			ost::MutexScope _SYNC_(mMutex);

			size_t indexPF = 0;
			RFAILED( PageFilePreAlloc( indexPF ) );
			RFAILED( StoreToStorage( indexPF, GetRamBuffer(pref->addr) ) );
			PageFileMarkUsed( indexPF, TRUE );

			if( pref->owner ) 
				pref->owner->onAddressInvalid( pref );

			pref->pfidx = indexPF;
			pref->flags |= BF_SWAPPED;

			size_t index = pref->addr/VM64_PAGE_SIZE;
			mAddressRefs[index] = 0;

			return S_OK;
		}
		HRESULT RamSwapIn( AddressRefPtr pref )
		{
			if( ! (pref->flags & BF_SWAPPED) )
				return S_FALSE;

			ost::MutexScope _SYNC_(mMutex);

			size_t indexRam = 0;
			if( pref->flags & BF_LOCKED )
			{
				// locked block must load to alloc-base
				indexRam = pref->addr / VM64_PAGE_SIZE;
			}
			else
			{
				RFAILED( FindFreeOrSwappableAddress( indexRam, rand() ) );
			}

			RFAILED( SwapRamAndPageFile( indexRam, pref ) );

			return S_OK;
		}
		HRESULT RamDirectAccess( AddressRefPtr pref, PVOID* pp, PVOID ownerBuffer = 0 )
		{
			ost::MutexScope _SYNC_(mMutex);
			size_t indexRam;
			if( !pref->flags ) return E_UNEXPECTED;
			if( pref->flags & BF_SWAPPED )
			{
				if( ownerBuffer ) 
				{
					RFAILED( LoadFromStorage( pref->pfidx, ownerBuffer ) );
					*pp = ownerBuffer;
					return S_OK;
				}
				RFAILED( RamSwapIn(pref) );
			}
			indexRam = pref->addr / VM64_PAGE_SIZE;
			*pp = GetRamBuffer(indexRam);
			return S_OK;
		}
		HRESULT RamDirectWrite( AddressRefPtr pref, SIZE_T off, PVOID p, SIZE_T cb )
		{
			if( off + cb > VM64_PAGE_SIZE ) return E_FAIL;
			ost::MutexScope _SYNC_(mMutex);
			PVOID dataBuffer = 0;
			RFAILED( RamDirectAccess( pref, &dataBuffer, mTemp ) );
			memcpy( (char*)dataBuffer + off, p, cb );
			if( 0 == (pref->flags & BF_SWAPPED) )
				return S_OK;
			return StoreToStorage( pref->pfidx, dataBuffer );
		}
		HRESULT RamDirectRead( AddressRefPtr pref, SIZE_T off, PVOID p, SIZE_T cb )
		{
			if( off + cb > VM64_PAGE_SIZE ) return E_FAIL;
			ost::MutexScope _SYNC_(mMutex);
			PVOID dataBuffer = 0;
			RFAILED( RamDirectAccess( pref, &dataBuffer, mTemp ) );
			memcpy( p, (char*)dataBuffer + off, cb );
			return S_OK;
		}

		FORCEINLINE VOID Lock() 
		{
			mMutex.lock();
		}
		FORCEINLINE VOID Unlock()
		{
			mMutex.unlock();
		}

		PVOID GetRamBuffer( size_t addr )
		{
			return mRamData + addr;
		}

		FORCEINLINE HRESULT EnsureRamBlock( AddressRefPtr pref )
		{
			if( !pref->flags ) return E_UNEXPECTED;

			if( !(pref->flags & BF_SWAPPED) ) return S_OK;

			size_t addrIndex = 0;

			RFAILED( FindFreeOrSwappableAddress( addrIndex, rand() ) );
			
			return ( SwapRamAndPageFile( addrIndex, pref ) );				
		};


	protected:

		FORCEINLINE HRESULT PageFilePreAlloc( size_t & rBlockIdx )
		{
			if( !mBlockBitMap.find0(false,rBlockIdx) )
			{
				rBlockIdx = mBlockBitMap.bits();
				mBlockBitMap.resize( rBlockIdx + mGrowCount );
			}
			return S_OK;
		}
		FORCEINLINE HRESULT PageFileMarkUsed( size_t uBlockId, BOOL bUsed )
		{
			ost::MutexScope _SYNC_(mMutex);
			return MarkSwapBlockUsed( uBlockId, bUsed );
		}
		FORCEINLINE HRESULT MarkSwapBlockUsed( UINT64 rBlockIdx, BOOL bUsed )
		{
			if( rBlockIdx >= mBlockBitMap.bits() )
				return E_UNEXPECTED;
			mBlockBitMap.setbit((size_t)rBlockIdx,bUsed);
			return S_OK;
		}
		FORCEINLINE VOID	CodeBlockPage( PVOID oBlock )
		{
			UINT64* pU64 = (UINT64*)(oBlock);
			for( size_t i = 0; i < VM64_PAGE_SIZE/sizeof(UINT64); ++ i )
			{
				pU64[i] ^= 0x1982120119821201LL;
			}
		}

		HRESULT SwapRamAndPageFile( size_t idxRam, AddressRefPtr inPF )
		{
			AddressRefPtr inMem = mAddressRefs[idxRam];
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

			size_t idxPageFile = 0;
			if( inPF )
			{
				idxPageFile = inPF->pfidx;
			}
			else
			{
				RFAILED( PageFilePreAlloc( idxPageFile ) );
			}

			PVOID dataBuffer = inMem ? mTemp : GetRamBuffer(idxRam*VM64_PAGE_SIZE);
			PVOID ramBuffer = GetRamBuffer(idxRam*VM64_PAGE_SIZE);

			if( inPF )
			{
				RFAILED( LoadFromStorage( idxPageFile, dataBuffer ) );
			}
			if( inMem )
			{
				RFAILED( StoreToStorage( idxPageFile, ramBuffer ) );
				PageFileMarkUsed( idxPageFile, TRUE );

				if( inMem->owner )
					inMem->owner->onAddressInvalid( inMem );

				inMem->pfidx = idxPageFile;
				inMem->flags |= BF_SWAPPED;

				if( inPF )
					memcpy( ramBuffer, dataBuffer, VM64_PAGE_SIZE );
				else
					memset( ramBuffer, 0, VM64_PAGE_SIZE );
			}
			if( inPF )
			{
				inPF->addr = idxRam * VM64_PAGE_SIZE;
				inPF->flags &= ~BF_SWAPPED;
				inPF->pfidx = 0;
			}
			mAddressRefs[idxRam] = inPF;
			return S_OK;
		}

		FORCEINLINE HRESULT FindSwappableAddress( size_t& rFreeIndex )
		{
			size_t i = 0;
			for( ; i < mAddressRefs.size(); ++ i )
			{
				AddressRefPtr pref = mAddressRefs[i];
				if( !pref )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT FindFreeAddress( size_t& rFreeIndex )
		{
			size_t i = 0;
			for( ; i < mAddressRefs.size(); ++ i )
			{
				AddressRefPtr pref = mAddressRefs[i];
				if( !pref )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}
		FORCEINLINE HRESULT FindSwappableAddress( size_t & rFreeIndex, size_t seed )
		{
			size_t i = 0;
			for( ; i < mAddressRefs.size(); ++ i )
			{
				AddressRefPtr pref = mAddressRefs[i];
				if( !pref )
				{
					rFreeIndex = i;
					return S_OK;
				}
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT FindFreeOrSwappableAddress( size_t & rFreeIndex, size_t seed )
		{
			srand( clock() * seed );
			const size_t maxRefs = mAddressRefs.size();
			size_t swapid = maxRefs;
			for( size_t i = 0; i < maxRefs; ++ i/*, check = (rand()%100)*/ )
			{
				AddressRefPtr pref = mAddressRefs[i];
				if( !pref )
				{
					// yeah, it's free!
					rFreeIndex = i;
					return S_OK;
				}

				// oops, can't swap LOCKED address
				if( pref->flags & BF_LOCKED )
					continue;

				// let's ramdon
				if( swapid == maxRefs )
				{
					swapid = i;
				}
				else 
				{
					double change = i; change = 1 / change; change *= maxRefs;
					if( change > rand()%maxRefs )
						swapid = i;
				}
			}
			// just find a swappable address
			if( swapid < maxRefs )
			{
				rFreeIndex = swapid;
				return S_OK;
			}
			return E_FAIL;
		}

		FORCEINLINE HRESULT StoreToStorage( size_t index, PVOID oBlock )
		{
			UINT64 uOffset =  index; uOffset *= VM64_PAGE_SIZE;
			RFAILED( seek_file( mStoageFile, uOffset, SEEK_SET ) );
			CodeBlockPage( oBlock );
			size_t cbw = write_file( mStoageFile, (oBlock), VM64_PAGE_SIZE );
			if( cbw != VM64_PAGE_SIZE ) return E_FAIL;
			//mStoreCount ++;
			return S_OK;
		}
		FORCEINLINE HRESULT LoadFromStorage( size_t index, PVOID oBlock )
		{
			UINT64 uOffset =  index; uOffset *= VM64_PAGE_SIZE;
			RFAILED( seek_file( mStoageFile, uOffset, SEEK_SET ) );
			size_t cbr = read_file( mStoageFile, (oBlock), VM64_PAGE_SIZE );
			if( cbr != VM64_PAGE_SIZE ) return E_FAIL;
			CodeBlockPage( oBlock );
			//mLoadCount ++;
			return S_OK;
		}
	};
};

vm::RamManager mgr;

struct MyAddressRef : vm::AddressRef
{
	MyAddressRef()
	{
		flags = 0;
		owner = 0;
	}
	~MyAddressRef()
	{
		mgr.RamFreeAddress( this );
	}
};

template < size_t PAGES >
class Private : vm::IAddressOwner
{
public:
	MyAddressRef _Refs[PAGES];
	enum { PageNum = PAGES };
	Private()
	{
		for( size_t i = 0; i < PAGES; ++ i )
		{
			_Refs[i].owner = this;
		}
	}
	~Private()
	{
	}
	virtual void onAddressInvalid( vm::AddressRef* pref )
	{
		size_t index = pref - (vm::AddressRef*)_Refs;
		printf( "_Refs[%d] invalid.\n", index );
	}
	long Commit()
	{
		for( size_t i = 0; i < PAGES; ++ i )
		{
			mgr.RamAllocAddress( _Refs + i, vm::BF_LOCKED );
		}
		return 0;
	}
};



namespace vin
{
	namespace vfs
	{
		struct node_t;
	};

	enum _KoType
	{
		TKoSymbolLink,		// KoSymbolLink
		TKoProcess,
		TKoThread,
		TKoMutex,
		TKoEvent,
		TKoSection,
		TKoWkmImage,
		TKoRegistry,
		TKoDevice,
		TKoPipe,
		TKoMailSlot,
		TKoFile,
		TKoVolume,
		TKoVirtFS,
	};

	typedef size_t	KoType;

	struct KoBase : RefalbeImp
	{
		size_t access;
		virtual ~KoBase() {};
		virtual size_t mainType() = 0;
		virtual void* asType( KoType faceid ) = 0 ;
		virtual uint32_t getAccess() { return access; };
		virtual void setAccess( size_t n ) { access = n; };
		template < class T >
		T* instantiate() { return (T*)asType(T::ObId); };
	};

	struct KoMountable : KoBase
	{
		vfs::node_t* _node;
		KoMountable() : _node(0) {};
		virtual void bindNode( vfs::node_t* node ) { _node = node; };
		virtual vfs::node_t* bindingNode() { return _node; }
	};

	struct KoSymbolLink : KoMountable
	{
		enum { ObId = TKoSymbolLink };
		std::string target;
		KoSymbolLink( const char * p ) 
		{
			target = p;
		}
		virtual size_t mainType()
		{
			return ObId;
		}
		virtual void* asType( KoType faceid )
		{
			if( faceid != ObId ) return 0;
			return this;
		}
	};

	struct KoDevice : KoMountable
	{
		enum { ObId = TKoDevice };
		KoDevice() 
		{
		}
		virtual size_t mainType()
		{
			return ObId;
		}
		virtual void* asType( KoType faceid )
		{
			if( faceid != ObId ) return 0;
			return this;
		}
		long control( void * )
		{
			return 0;
		}
	};

	namespace vfs
	{
		struct dir_t
		{
		};

		static bool operator < ( const std::string & l, const std::string& r )
		{
			return stricmp( l.c_str(), r.c_str() ) < 0;
		}

		struct node_t 
		{
		protected:
			typedef std::set<node_t> children_t;
			const std::string		_name;
			children_t				_nodes;
			node_t*					_parent;
			refp<vin::KoMountable>	_mount;
		public:
			bool operator < ( const node_t & right ) const
			{
				return _name < right._name;
			}
		public:
			node_t( const char* name, node_t* p = 0 ) 
				: _name(name)
				, _parent(p)
			{};
			node_t( const std::string& name, node_t* p ) : _name(name), _parent(p)
			{};
			const char * name()
			{
				return _name.c_str();
			}
			node_t* parent()
			{
				return _parent;
			}
			bool getFullPath( std::string & rName ) 
			{
				node_t * p = this;
				for( ; p ; p = p->parent() )
				{
					rName.insert( 0, p->name() );
					rName.insert( rName.begin(), '\\' );
				}
				return true;
			}
			long mount( vin::KoMountable* ob ) 
			{
				if( !ob ) return E_INVALIDARG;
				if( _mount ) return E_UNEXPECTED;
				_mount = ob; 
				ob->bindNode( (node_t*)this );
				return S_OK;
			}
			long unmount() 
			{
				if( !_mount ) return E_UNEXPECTED;
				_mount->bindNode(0);
				_mount.dispose();
				return S_OK;
			}
			vin::KoMountable* mounted() const
			{
				return (vin::KoMountable*)(refp<vin::KoMountable>)_mount;
			}
			node_t* getNode( const std::string& name, bool newIfNotExist = false ) 
			{
				node_t tmp( name, this );
				children_t::iterator it = _nodes.find( tmp );
				if( it == _nodes.end() ) return 0;
				return (node_t*)&(*it);
			}
			node_t* getNode( const char* name, bool newIfNotExist = false ) 
			{
				std::string tmp( name );
				return getNode( tmp, newIfNotExist );
			};
			// if ppnode exist, do replace operation
			node_t* addChild( const char* name ) 
			{
				if( !name ) return 0;
				node_t tmp( name, this );
				children_t::iterator it = _nodes.find( tmp );
				if( it == _nodes.end() )
				{
					children_t::_Pairib ib = _nodes.insert( tmp );
					it = ib.first;
				}
				return (node_t*)&(*it);
			};
			// feedback existing node
			long eraseChild( const char* name, vin::KoMountable** pp ) 
			{
				if( !name ) return E_INVALIDARG;

				node_t tmp( name, this );
				children_t::iterator it = _nodes.find( tmp );

				if( it == _nodes.end() )
					return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

				if( pp ) 
				{
					refp<vin::KoMountable> ob = it->mounted();
					ob->bindNode(0);
					*pp = ob.detach();
				}
				else
				{
					unmount();
				}
				_nodes.erase( it );
				return S_OK;
			};
			node_t * next( node_t * p )
			{
				if( _nodes.empty() ) return NULL;

				children_t::iterator it = _nodes.begin();
				if( !p ) return (node_t *)&(*it);

				it = _nodes.upper_bound( *p );
				if( it == _nodes.end() ) return NULL;
				
				return (node_t *)&(*it); 
			}
		};

		size_t skip_slash( const char * & p )
		{
			const char * k = p;
			for( ; *p && *p == '\\'; ++ p );
			return  p - k;
		}
		const char * skip_not_slash( const char *& p )
		{
			for( ; *p && *p != '\\'; ++ p );
			return p;
		}

		struct pather
		{
			long parse( const char * & p )
			{
				size_t c = skip_slash( p );
				//if( c )
				//{
				//	long lr = onPathPart( std::string("") );
				//	if( lr < 0 ) return lr;
				//}
				const char * nb = p;
				while( *p )
				{
					const char * ne = skip_not_slash(p);
					std::string tmp( nb, ne );
					long lr = onPathPart( tmp );
					if( lr < 0 ) return lr;
					skip_slash(p);
					nb = p;
				}
				return S_OK;
			}
			//virtual long onRoot( std::string& dir ) { return E_FAIL; };
			virtual long onPathPart( std::string& dir ) { return E_FAIL; };
		};

		enum
		{
			PATH_OK = 0,
			PATH_REPARSE_POINT_REARCHED = -1,
			PATH_NOT_FOUND = -2,
		};

		struct creator : pather
		{
		protected:
			node_t *	_current;
			bool		_force;
		public:
			creator( vfs::node_t * current, bool force = true ) 
				: _current(current)
				, _force(force)
			{
			}
			node_t * current()
			{
				return _current;
			}
			void reset( node_t * p )
			{
				_current = p;
			}
			virtual long onPathPart( std::string& dir )
			{
				node_t * update = _current;
				if( dir == "." )
				{
				}
				else if( dir == ".." )
				{
					update = _current->parent();
				}
				else
				{
					update = _current->getNode( dir );
				}

				if( !update )
				{
					if( !_force ) return PATH_NOT_FOUND;

					update = _current->addChild( dir.c_str() );

					if( !update ) return PATH_NOT_FOUND;
				}

				_current = update;

				refp<vin::KoMountable> ob = update->mounted();
				if( ob && ob->asType(vin::TKoSymbolLink) )
					return PATH_REPARSE_POINT_REARCHED;

				return PATH_OK;
			}
		};

		struct finder : pather
		{
		protected:
			node_t * _current;
			bool	 _reparse;
		public:
			finder( vfs::node_t * current, bool reparse = true ) 
				: _current(current)
				, _reparse(reparse)
			{
			}
			node_t * current()
			{
				return _current;
			}
			void reset( node_t * p )
			{
				_current = p;
			}
			virtual long onPathPart( std::string& dir )
			{
				if( dir == "." )
				{
				}
				else if( dir == ".." )
				{
					_current = _current->parent();
				}
				else
				{
					_current = _current->getNode( dir );
				}

				if( !_current )
					return PATH_NOT_FOUND;

				if( _reparse )
				{
					refp<vin::KoMountable> ob = _current->mounted();
					if( ob && ob->asType(vin::TKoSymbolLink) )
						return PATH_REPARSE_POINT_REARCHED;
				}

				return PATH_OK;
			}
		};
	};

	struct kvfs
	{
	protected:
		vfs::node_t _root;
	public:

		kvfs() : _root("")
		{}

		vfs::node_t * getRoot()
		{
			return &_root;
		}
		vfs::node_t * createPath( const char * path, bool force = true, KoMountable* ob = 0, vfs::node_t * root = 0)
		{
			vfs::creator mkpath( root?root:getRoot() );
			for( ;; )
			{
				long lr = mkpath.parse( path );
				if( lr == vfs::PATH_REPARSE_POINT_REARCHED )
				{
					refp<vin::KoSymbolLink> lnk = (vin::KoSymbolLink*)
						(mkpath.current()->mounted()->asType( vin::KoSymbolLink::ObId ));
					mkpath.reset(getRoot());
					path = lnk->target.c_str();
					continue;
				}
				if( lr < 0 ) return NULL;
				vfs::node_t * cur = mkpath.current();
				if( !cur ) return NULL;
				if( ob ) 
				{
					lr = cur->mount( ob );
					if( lr < 0 ) 
						return NULL;
				}
				return cur;
			}
			return NULL;
		}
		vfs::node_t * getNodeByPath( const char * path, bool reparse = true, vfs::node_t * root = 0)
		{
			vfs::finder fdpath( root?root:getRoot(), reparse );
			for( ; *path; )
			{
				long lr = fdpath.parse( path );
				if( lr == vfs::PATH_REPARSE_POINT_REARCHED )
				{
					refp<vin::KoSymbolLink> lnk = (vin::KoSymbolLink*)
						(fdpath.current()->mounted()->asType( vin::KoSymbolLink::ObId ));
					vfs::node_t * node = getNodeByPath( lnk->target.c_str() );
					if( !node ) return NULL;
					fdpath.reset(node);
					vfs::skip_slash(path);
					continue;
				}
				else if( lr < 0 ) 
				{
					return NULL;
				}
			}
			return fdpath.current();
		}
		KoMountable * getMountedByPath( const char * path, vfs::node_t * root )
		{
			vfs::node_t * p = getNodeByPath( path, true, root );
			if( !p ) return NULL;
			return p->mounted();
		}
		vfs::node_t * getNext( vfs::node_t * parent, vfs::node_t * current )
		{
			return parent->next( current );
		}
	};

	typedef size_t	handle_t;

	struct Handle
	{
		uint32_t		hacc;
		refp<KoBase>	object;
	};
	struct KsHandleManager
	{
		enum { MAX_HANDLERS = 1000 };

		typedef std::map<uint32_t, Handle> HandleMap;

		HandleMap	_handles;

		long allocHandle( handle_t * ph, KoBase* ob, uint32_t hacc )
		{
			if( !ph || !ob ) return E_INVALIDARG;
			uint32_t hv = 4;
			if( _handles.size() )
			{
				hv = _handles.rbegin()->first;
				hv += 4;
			}
			Handle tmp;
			tmp.object = ob;
			tmp.hacc = hacc;
			_handles[hv] = tmp;
			return 0;
		}
		long freeHandle( handle_t h )
		{
			if( h % 4 ) 
				return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);

			HandleMap::iterator it = _handles.find( h );
			if( it == _handles.end() ) 
				return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);

			_handles.erase( it );

			return S_OK;
		}
	};

	//namespace exec
	//{

	//	struct AddressSpace
	//	{};

	//	struct KoProcess : KoBase, KsHandleManager, AddressSpace
	//	{
	//		enum { ObId = TKoProcess };
	//		virtual ~KoProcess() {};
	//		virtual size_t mainType() { return ObId; };
	//		virtual void* asType( KoType faceid )
	//		{
	//			if( faceid == ObId ) return this;
	//			return 0;
	//		}
	//		refp<KoBase>	_mainImage;

	//		readvm();
	//		writevm();
	//		long loadIamge( const char * filename )
	//		{

	//		}

	//	};

	//};

	struct KoFile : vin::KoMountable
	{
		enum { ObId = TKoFile };
		virtual size_t mainType() 
		{ 
			return ObId; 
		};
		virtual void* asType( KoType faceid ) 
		{ 
			if( faceid == ObId ) return this;
			return 0;
		}
		void readp() {};
		void writep() {};
		void close() {};
	};

	struct KsWkmDesc
	{
		size_t	offPath;		// BootDevice\{path}
		size_t	offData;
		size_t	sizData;
		size_t	sizImage;
		size_t	sizImagePlace;	// 
	};

	struct KoWkmImage : KoFile
	{
		enum { ObId = TKoWkmImage };
		virtual void* asType( KoType faceid ) 
		{ 
			if( faceid == ObId ) return this;
			return KoFile::asType(faceid);
		}
	};

	namespace sync
	{
		struct KoMutex
		{
			uint32_t	_flags;
			enum { ObId = TKoMutex };
			KoMutex() : _flags(0)
			{}
			virtual void* asType( KoType faceid ) 
			{ 
				if( faceid == ObId ) return this;
				return 0;
			}
			long init( uint32_t flags )
			{
				return -1;
			}
			long queueThread( void * p )
			{
				return -1;
			}
		};

		struct KoEvent
		{
			enum { ObId = TKoMutex };
			virtual void* asType( KoType faceid ) 
			{ 
				if( faceid == ObId ) return this;
				return 0;
			}
		};
	};

	namespace ipc
	{
		struct KoPipe : KoFile
		{
			
		};
		struct KoMailSlot : KoFile
		{

		};
	};

	struct kernel
	{
		kvfs	root;
	};



};

// vfs 实现了windows下的对象命名系统，整个内核有一个名字空间，文件/注册表/对象都挂载在这个名字空间中
// 打开指定路径的对象时，如果目标对象为符号链接，则按照符号链接中的路径重新寻找
// 命名系统的节点上可以挂接真实的内核对象(KoXxx)，例如：符号链接对象、Section对象，文件对象，注册表对象等等
// 目前已规划的内核对象有:
//	KoSymbolLink
//	KoFile
//	KoWkmImage
//	KoImage
//	KoSection
//	KoRegistry
//	KoMutex
//	KoEvent
//	KoThread
//	KoProcess
//	KoDevice
//	


//namespace vin
//{
//	namespace kernel 
//	{
//		vfs::node_t root;
//	};
//
//	struct kernel32
//	{
//		HANDLE CreateFileA( 
//			LPCSTR lpFileName, 
//			DWORD dwDesiredAccess, 
//			DWORD dwShareMode, 
//			LPSECURITY_ATTRIBUTES lpSecurityAttributes, 
//			DWORD dwCreationDisposition, 
//			DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
//		{
//			traveller finder( &kernel::root, 0 );
//
//			if( !finder.parsePath( lpFileName ) )
//				return ERROR_PATH_NOT_FOUND;
//
//			vin::KoMountable* p = finder.current()->mounted();
//			if( !p ) return ERROR_PATH_NOT_FOUND;
//
//			vin::KoFile* bp = p->asType( vin::TKoFile );
//			CurrentProcess.AllocHandle( bp, dwDesiredAccess );
//		}
//	};
//};



//
//struct IKmoSectionUser
//{
//	virtual void onBlockInvalid( uint64_t offset ) = 0;
//};
//
//struct Umo
//{
//	enum Type { 
//		UMO_VIEW,
//		UMO_IMAGE,
//	};
//	virtual Type getType() = 0;
//	virtual vm::AddressRef* getRamBlock( uint64_t offset ) = 0;
//};
//
//typedef Umo *	UmoPtr;
//
//struct UasSpace
//{
//	UmoPrivate	_privates;
//	void m2munmap( uint64_t vma );
//	void m2mmap( uint64_t vma, uint64_t rma, uint64_t attr );
//	void m2mupdate( uint64_t vma, uint64_t attr );
//};
//
//struct UasRegion
//{
//	uint64_t	_base;
//	uint64_t	_size;
//	UmoPtr		_umo;
//};
//
//struct KmoSection : vm::IAddressOwner
//{
//	struct SectionRamBlock : vm::AddressRef
//	{
//		uint64_t	offset;
//	};
//	typedef std::set<IKmoSectionUser*> users_t;
//
//	SectionRamBlock	_wnd[64];
//	uint64_t		_size;
//	uint32_t		_acc;
//	uint32_t		_type;
//	VOID*			_back;
//	users_t			_users;
//
//	void insertUser( IKmoSectionUser* p )
//	{
//		_users.insert( p );
//	}
//	void removeUser( IKmoSectionUser* p )
//	{
//		_users.erase( p );
//	}
//	vm::AddressRef* getRamBlock( uint64_t offset )
//	{
//		if( offset >= _size ) return NULL;
//		uint64_t index = offset >> 12;
//		size_t index32 = index % 256;
//		SectionRamBlock& rb = _wnd[index32];
//		if( rb->flags & vm::BF_VALID )
//		{
//			if( _acc & SECTION_MAP_WRITE )
//			{
//				// Write Access
//				if( _type == SEC_FILE )
//				{
//					RFAILED( mgr.RamSwapIn( &rb ) );
//					seek_file( _back, offset, SEEK_SET );
//					write_file( _back, mgr.GetRamBuffer(rb.addr) );
//					mgr.RamFreeAddress( &rb );
//				}
//				else
//				{
//					// Only File can write back
//					return E_UNEXPECTED;
//				}
//			}
//			else
//			{
//				// Read Only, just free it
//				mgr.RamFreeAddress( &rb );
//			}
//		}
//
//		RFAILED( mgr.RamAllocAddress( &rb, 0 ) );
//
//		seek_file( _back, offset, SEEK_SET );
//		read_file( _back, mgr.GetRamBuffer(rb.addr) );
//
//		return &rb;
//	}
//	virtual void onBlockInvalid( vm::AddressRef* pb )
//	{
//		SectionRamBlock* psrb = (SectionRamBlock*)pb;
//		if( !psrb ) return ;
//		users_t::iterator it = _users.begin();
//		for( ; it != _users.end(); ++ it )
//		{
//			(*it)->onBlockInvalid( psrb->offset );
//		}
//	}
//};
//
//// Vms is a window of KmoSection
//struct UmoView : Umo, IKmoSectionUser
//{
//	UasRegion&	_region;
//	uint64_t	_offset;
//	uint64_t	_size;
//	uint32_t	_access;
//	KmoSection&	_section;
//	UmoView(UasRegion& ur, KmoSection& sec ) : _section(sec), _region(ur)
//	{
//		_section->insertUser( this );
//	}
//	~UmoView()
//	{
//		_section->removeUser( this );
//	}
//	uint32_t		getAccess();
//	vm::AddressRef* getRamBlock( uint64_t offset )
//	{
//		if( offset >= _size ) return NULL;
//		return _section->getRamBlock( _offset + offset );
//	}
//	virtual void onBlockInvalid( uint64_t offset )
//	{
//		if( offset < _offset ) return ;
//		uint64_t delta = offset - _offset;
//		if( delta < _size ) return ;
//		uint64_t vma = _region._base + delta;
//		// invalid PageTable(vma)
//	}
//};
//
//// Vms is whole KmoSection
//struct UmoImage : Umo, IKmoSectionUser
//{
//	UasRegion&	_region;
//	KmoSection&	_section;
//	UmoView( UasRegion& rgn, KmoSection& sec ) 
//		: _region(rgn)
//		, _section(sec)
//	{
//	}
//	vm::AddressRef* getRamBlock( uint64_t offset )
//	{
//		return _section->getRamBlock( offset );
//	}
//	virtual void onBlockInvalid( uint64_t offset )
//	{
//		uint64_t vma = _region._base + offset;
//		// invalid PageTable(vma)
//	}
//};
//
//struct UmoPrivate : vm::IAddressOwner
//{
//	UasSpace&	_space;
//	struct PrivateBlock : vm::AddressRef
//	{
//		uint64_t	vma;
//	};
//	typedef std::map<uint64_t,PrivateBlock> PrivatePages;
//	PrivatePages	_pages;
//
//	// all private pages manged by UmoPrivate,
//	// one process, one UmoPrivate
//	UmoPrivate( UasSpace& sp ) : _space(sp)
//	{
//		
//	}
//	vm::AddressRef* newRamBlock( uint64_t offset )
//	{
//		vm::AddressRef tmp = {};
//		tmp.owner = this;
//		PrivatePages::_Pairib ib = _pages.insert( PrivatePages::value_type(offset, tmp) );
//		if( !ib.second ) return NULL;
//		PrivatePages::iterator it  = ib.first;
//		vm::AddressRef& r = it->second;
//		mgr.RamAllocAddress( &r );
//		return &(it->second);
//	}
//	vm::AddressRef* getRamBlock( uint64_t offset )
//	{
//		PrivatePages::iterator it = _pages.find( offset );
//		if( it == _pages.end() ) return NULL;
//		return &(it->second);
//	}
//	vm::AddressRef* getRamBlock( uint64_t offset, bool force )
//	{
//		PrivatePages::iterator it = _pages.find( offset );
//		if( it == _pages.end() )
//		{
//			if( !force ) return NULL;
//			vm::AddressRef tmp = {};
//			tmp.owner = this;
//			PrivatePages::_Pairib ib = _pages.insert( PrivatePages::value_type(offset, tmp) );
//			if( !ib.second ) return NULL;
//			it = ib.first;
//			vm::AddressRef& r = it->second;
//			mgr.RamAllocAddress( &r );
//		}
//		return &(it->second);
//	}
//	virtual void onBlockInvalid( vm::AddressRef* pb )
//	{
//		uint64_t vma = ((PrivateBlock*)pb)->vma;
//		_space.m2munmap( vma );
//		// invalid PageTable(vma)
//	}
//};
//
//struct KboImageFile			// it's PEArchive & runtime information
//{
//	UTIL::com_ptr<I12PEArchive> _12pe;
//	uint64_t					_expbase;
//};
//
//struct KWM_DESC
//{
//	uint32_t	size;		// in bytes
//	uint32_t	room;
//	uint32_t	hdrcb;		// header size
//};
//
//struct KboWkmImageFile		// it's PEArchive & runtime information
//{
//	KWM_DESC*	_desc;
//};
//
//struct KboDataFile
//{
//	
//};
//
//enum 
//{
//	TRAP_EAT,
//	RAISE_EXCEPTION,
//};
//
//long handle_nx_page( UasSpace& space, uint64_t vma )
//{
//	// if NX
//	UasRegion* rg = findRegion( vma );
//	if( !rg ) return RAISE_EXCEPTION;
//	UasBlock* blk = rg->findBlock( vma );
//	if( !blk ) return RAISE_EXCEPTION; 
//	if( blk->attr & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY) )
//	{
//		// let's executable
//		space.m2mupdate( vma, 1LL<<63 );
//		return TRAP_EAT;
//	}
//	else
//	{
//		return RAISE_EXCEPTION;
//	}
//}
//
//// 处理写拷贝
////	如果已经在Private中，则直接将页修改为可写
////	如果在View|Image中，则为当前页分配Private内存，并将其加入到PrivateMap中
////	
//long handle_write_page( UasSpace& space, uint64_t vma )
//{
//	vm::AddressRef* pref = space._privates.find( vma );
//	if( pref ) return space.m2mupdate( vma, 1L<<3);
//	UasRegion* rg = findRegion( vma );
//	if( !rg ) return RAISE_EXCEPTION;
//	if( !rg->_umo ) return RAISE_EXCEPTION;
//	UasBlock* blk = rg->findBlock( vma );
//	if( !blk ) return RAISE_EXCEPTION;
//	if( blk->attr & (PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY) )
//	{
//		// let's create private page
//		vm::AddressRefPtr pref = space._privates.newRamBlock( vma );
//		space.m2munmap( vma );
//		uint32_t newAttr = blk->attr & ~(PAGE_EXECUTE_WRITECOPY|PAGE_WRITECOPY);
//		if( blk->attr & PAGE_EXECUTE_WRITECOPY )
//			newAttr |= PAGE_EXECUTE_READWRITE;
//		else if( blk->attr & PAGE_WRITECOPY )
//			newAttr |= PAGE_READWRITE;
//		space.m2mmap( vma, pref->addr, attrUAS2PTE(newAttr) );
//		blk->attr = newAttr;
//	}
//}
//
//long handle_no_page( UasSpace& space, uint64_t vma )
//{
//	vm::AddressRef* g = UmoPrivate::getRamBlock( vma );
//	if( g ) 
//	{
//		mgr.RamSwapIn( g );
//		// page table map( vma, g->addr );
//		return TRAP_EAT;
//	}
//	else
//	{
//		UasRegion* rg = findRegion( vma );
//		if( !rg ) return RAISE_EXCEPTION;
//		UasBlock* blk = rg->findBlock( vma );
//		if( !blk ) return RAISE_EXCEPTION; 
//		if( !rg->_umo )
//		{
//			// it's private!
//			vm::AddressRef* pref = UmoPrivate::newRamBlock( vma );
//			if( !pref ) return RAISE_EXCEPTION;
//			uint64_t attrPTE = attrUAS2PTE(blk->attr);
//			space.m2mmap( vma, pref->addr, attrPTE );
//			return TRAP_EAT;
//		}
//		else
//		{
//			// it's view/image
//			vm::AddressRef* pref = rg->_umo->getRamBlock( vma - rg->_base );
//			if( !pref ) return RAISE_EXCEPTION;
//			uint64_t attrPTE = attrUAS2PTE(blk->attr);
//			space.m2mmap( vma, pref->addr, pte_attr );
//			return TRAP_EAT;
//		};
//	}
//}
//
//struct Umo;
//
//typedef uint64_t		guest_vma;
//typedef uint64_t		guest_size;
//typedef guest_vma		gvma_t;
//typedef guest_size		gsize_t;
//
//struct WasBlock
//{
//	WasBlock*	prev;
//	WasBlock*	next;
//	gvma_t		base;
//	gsize_t		size;
//	uint32_t	attr;
//};
//
//struct WasRegion
//{
//	gvma_t		base;
//	gsize_t		initSize;
//	uint32_t	initAttr;
//	WasBlock*	first;
//	WasBlock*	last;
//	refp<Umo>	umo;
//};
//
//namespace obspace
//{
//
//};
//
//
//
//namespace vin
//{
//
//
//
//};
//

class kvfs
{
public:
	long createPath( const char * path, vin::KoBase* omount );
	long removePath( const char * path, vin::KoBase* omount );
};

