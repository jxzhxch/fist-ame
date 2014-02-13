#ifndef __PAGE_TABLE__
#define __PAGE_TABLE__

#include <vector>
#include "rammgr.h"
#include "x64.h"

namespace vm
{
	typedef std::vector<AddressRefPtr>	PgtBlocks;

	// PAE + 4K.only
	struct PageTable
	{
		enum 
		{
			PGT_LOADED = 1,
		};
		
		size_t		_attr;

		PgtBlocks	_pages;

		PageTable() : _attr(0)
		{}

		~PageTable()
		{
			destroy();
		}
		bool load()
		{
			_attr |= PGT_LOADED;

			// first, mark all ram-block locked,
			for( size_t i = 0; i < _pages.size(); ++ i )
				_pages[i]->flags |= BF_LOCKED;
			
			// next, swap-in all page-table blocks.
			for( size_t i = 0; i < _pages.size(); ++ i )
			{
				HRESULT hr = mgr.RamSwapIn( _pages[i] );
				if( hr < 0 ) return false;
			}
			return true;
		}
		bool unload()
		{
			_attr &= ~PGT_LOADED;
			for( size_t i = 0; i < _pages.size(); ++ i )
				_pages[i]->flags &= ~BF_LOCKED;
			return true;
		}
		void destroy()
		{
			for( size_t i = 0; i < _pages.size(); ++ i )
			{
				mgr.RamFreeAddress( _pages[i] );
				delete _pages[i];
				_pages[i] = 0;
			}
			_pages.clear();
			_attr = 0;
		}
		/////////////////////////////////////////////////////
		bool get_pma( uint64_t vma, uint64_t * pma )
		{
			x64::vmal4_t addr; addr.i = vma;
			x64::_4k::pte_t * pte = get_pte( addr, false );
			if( !pte ) return false;
			if( !pte->P ) return false;
			*pma = pte->BasePG;
			return true;
		}
		enum 
		{
			R = 1,
			W = 2,
			X = 4,
		};
		static void set_attr( x64::_4k::pte_t * pte, size_t f )
		{
			set_r( pte, (f&R)!=0 ); 
			set_w( pte, (f&W)!=0 ); 
			set_x( pte, (f&X)!=0 ); 
		}
		static void set_x( x64::_4k::pte_t * pte, bool f )
		{
			pte->NX = f ? 0 : 1;
		}
		static void set_w( x64::_4k::pte_t * pte, bool f )
		{
			pte->RW = f ? 1 : 0;
		}
		static void set_r( x64::_4k::pte_t * pte, bool f )
		{
			pte->P = f ? 1 : 0;
		}
		bool vmattr( uint64_t vma, size_t f )
		{
			x64::vmal4_t addr; addr.i = vma;
			x64::_4k::pte_t * pte = get_pte( addr, false );
			if( !pte ) return false;
			set_attr( pte, f );
			return true;
		}
		long vmmap( uint64_t vma, uint64_t pma, size_t attr )
		{
			if( (attr & R) == 0 ) return S_OK;
			x64::vmal4_t addr; addr.i = vma;
			x64::_4k::pte_t * pte = get_pte( addr, true );
			if( !pte ) return E_OUTOFMEMORY;
			if( pte->P ) return E_UNEXPECTED;
			pte->BasePG = pma;
			set_attr( pte, attr );
			return S_OK;
		}
		long vmunmap( uint64_t vma )
		{
			x64::vmal4_t addr; addr.i = vma;
			x64::_4k::pte_t * pte = get_pte( addr, false );
			if( !pte ) return E_OUTOFMEMORY;
			if( !pte->P ) return E_UNEXPECTED;
			pte->i = 0;
			return S_OK;
		}
		/////////////////////////////////////////////////////
		x64::_4k::pte_t * get_pte( x64::vmal4_t vma, bool alloc_new )
		{
			AddressRefPtr root = 0;
			if( _pages.size() )
			{
				root = _pages[0];
			}
			else
			{
				if( !alloc_new ) return 0;
				root = alloc_pgt_block();
			}
			////////////////////////////////////////////////////////////////////////
			size_t ipml4 = vma.u.ipml4;
			x64::pml4_t* pmlt = (x64::pml4_t*)mgr.GetRamBuffer(root->addr);
			if( !pmlt ) return 0;
			x64::pml4e_t & pmlte = pmlt->ent[ipml4];
			////////////////////////////////////////////////////////////////////////
			x64::pdp_t* pdp = 0;
			if( !pmlte.P )
			{
				if( !alloc_new ) return 0;
				AddressRefPtr pdp_page = alloc_pgt_block();
				if( !pdp_page ) return 0;
				pmlte.P = 1;
				pmlte.BasePDP = pdp_page->addr;
				pmlte.RW = 1;
				pdp = (x64::pdp_t*)mgr.GetRamBuffer(pdp_page->addr);
			}
			else
			{
				pdp = (x64::pdp_t*)mgr.GetRamBuffer(pmlte.BasePDP);
			}
			////////////////////////////////////////////////////////////////////////
			x64::pdpe_t& pdpe = pdp->ent[vma.u.ipdpe];
			x64::_4k::pd_t* pd = 0;
			if( !pdpe.P )
			{
				if( !alloc_new ) return 0;
				AddressRefPtr pd_page = alloc_pgt_block();
				if( !pd_page ) return 0;
				pdpe.P = 1;
				pdpe.BasePD = pd_page->addr;
				pdpe.RW = 1;
				pd = (x64::_4k::pd_t*)mgr.GetRamBuffer(pd_page->addr);
			}
			else
			{
				pd = (x64::_4k::pd_t*)mgr.GetRamBuffer(pdpe.BasePD);
			}
			////////////////////////////////////////////////////////////////////////
			x64::_4k::pde_t& pde = pd->ent[vma.u.ipde];
			x64::_4k::pt_t* pt = 0;
			if( ! pde.P )
			{
				if( !alloc_new ) return 0;
				AddressRefPtr pt_page = alloc_pgt_block();
				if( !pt_page ) return 0;
				pde.P = 1;
				pde.BasePT = pt_page->addr;
				pde.RW = 1;
				pt = (x64::_4k::pt_t*)mgr.GetRamBuffer(pt_page->addr);
			}
			else
			{
				pt = (x64::_4k::pt_t*)mgr.GetRamBuffer(pde.BasePT);
			}
			////////////////////////////////////////////////////////////////////////
			return &(pt->ent[vma.u.ipte]);
		}
	protected:
		AddressRefPtr alloc_pgt_block()
		{
			AddressRefPtr p = new AddressRef();
			if( !p ) return 0;
			HRESULT hr = mgr.RamAllocAddress( p, (_attr&PGT_LOADED) ? BF_LOCKED : 0 );
			if( hr < 0 ) 
			{ 
				delete p;
				return 0;
			}
			_pages.push_back( p );
			return p;
		}
	};
};

struct KoSection;

struct KoSectionUser
{
	KoSection *	_section;
	KoSectionUser() : _section(0) { };
	virtual ~KoSectionUser() { close(); };
	virtual bool create( KoSection * section );
	virtual bool close();
	virtual void onSectionBlockInvalid( uint64_t off ) {};
	
};

struct KoSection : vm::IAddressOwner, vin::KoMountable
{
	typedef std::set<KoSectionUser*> SectionUsers;

	SectionUsers	_users;
	uint64_t		_size;
	enum Type
	{
		SecFile,
		SecImage,
		SecWkmFile,
	};
	enum { ACTIVE_BLOCKS = 32 };
	struct BlockRef : vm::AddressRef
	{
		uint64_t _off;
		BlockRef() : _off(0) {};
	};

	BlockRef _blocks[ACTIVE_BLOCKS];

	KoSection()
	{
		for( size_t i = 0; i < ACTIVE_BLOCKS; ++ i )
			_blocks[i].owner = this;
		_size = 0;
	}
	~KoSection()
	{
		destroy();
	}

	virtual size_t mainType()
	{
		return vin::TKoSection;
	}
	virtual void* asType( vin::KoType faceid )
	{
		if( faceid == vin::TKoSection ) return this;
		return 0;
	}

	void destroy()
	{
		// first notify all users the block invalid.
		for( size_t i = 0; i < ACTIVE_BLOCKS; ++ i )
			onAddressInvalid( &_blocks[i] );
		_users.clear();
		close();
	}
	bool close()
	{
		if( _users.size() ) return false;
		for( size_t i = 0; i < ACTIVE_BLOCKS; ++ i )
			mgr.RamFreeAddress( &_blocks[i] );
		return true;
	}
	bool create( uint64_t size, void * f )
	{
		_size = size;
		return true;
	}
	virtual void onAddressInvalid( vm::AddressRef* pref ) 
	{
		BlockRef* sbrp = (BlockRef*)pref;
		size_t index = sbrp - _blocks;
		if( index >= ACTIVE_BLOCKS ) return ;
		// notify users
		SectionUsers::iterator it = _users.begin();
		for( ; it != _users.end(); ++ it )
		{
			(*it)->onSectionBlockInvalid( sbrp->_off );
		}
	}
	virtual vm::AddressRef* get_block( uint64_t off )
	{
		if( off >= _size ) return 0;

		uint64_t index = (off >> 12)%ACTIVE_BLOCKS;
		size_t idx = (size_t)index;
		if( idx != index ) return 0;

		BlockRef& wanna = _blocks[idx];
		if( !(wanna.flags & vm::BF_VALID) )
		{
			HRESULT hr = mgr.RamAllocAddress( &wanna, 0 );
			if( hr < 0 ) return 0;
		}
		else
		{
			HRESULT hr = mgr.RamSwapIn(&wanna);
			if( hr < 0 ) return 0;
			if( wanna._off == off ) 
				return &wanna;
			// todo: if( _dirty ) 
			// how to detect page-dirty ?
			if( !save_to_file( wanna ) ) return 0;
		}
		if( !load_from_file( off, wanna ) ) return 0;
		wanna._off = off;
		return &wanna;
	}

	void insertUser( KoSectionUser* pu )
	{
		_users.insert( pu );
	}
	void removeUser( KoSectionUser* pu )
	{
		_users.erase( pu );
	}

protected:
	bool save_to_file( BlockRef& blk )
	{
		return true;
	}
	bool load_from_file( uint64_t off, BlockRef& blk )
	{
		char * p = (char*)mgr.GetRamBuffer(blk.addr);
		sprintf( p, "off=%I64d", off );
		return true;
	}
};

bool KoSectionUser::create( KoSection * section )
{
	if( !section ) return false;
	if( _section ) return false;
	_section = section;
	_section->insertUser( this );
	return true;
};
bool KoSectionUser::close()
{
	if( _section ) _section->removeUser( this );
	_section = 0;
	return true;
};

struct UoPrivate : vm::IAddressOwner
{
	struct PrivBlock : vm::AddressRef
	{
		// u can't change it!
		const uint64_t	_vma;
		PrivBlock( uint64_t vma ) : _vma(vma)
		{
			flags = 0; addr = 0;
			pfidx = 0; owner = 0;	//4	8
		}

		bool operator < ( const PrivBlock& right ) const 
		{
			return _vma < right._vma;
		}
	};

	typedef std::set<PrivBlock>	PrivBlocks;

	PrivBlocks	_blocks;

	virtual void onAddressInvalid( vm::AddressRef* pref ) 
	{
		PrivBlock* pbp = (PrivBlock*)pref;
		printf( "private block invalid at %I64X\n", pbp->_vma );
	}

	virtual vm::AddressRef* get_block( uint64_t off )
	{
		return get_priv_block( off, false );
	}

	PrivBlock* get_priv_block( uint64_t vma, bool force )
	{
		PrivBlock key(vma);
		PrivBlocks::iterator it = _blocks.find( key );
		if( it == _blocks.end() )
		{
			if( !force ) return 0;
			key.owner = this;
			HRESULT hr = mgr.RamAllocAddress( &key, 0 );
			if( hr < 0 ) 
				return 0;
			PrivBlocks::_Pairib ib = _blocks.insert( key );
			if( !ib.second )
			{
				mgr.RamFreeAddress( &key );
				return 0;
			}
			it = ib.first;
		}
		return (PrivBlock*)&(*it);
	}

};

struct UoImage : KoSectionUser
{
	virtual void onSectionBlockInvalid( uint64_t off )
	{
		// notify page-table
	}
};

struct UoView : KoSectionUser
{
	uint64_t _off;
	uint64_t _size;
	virtual bool create( KoSection * section, uint64_t off, uint64_t size )
	{
		uint64_t vend = off + size;
		if( vend >= section->_size ) return false;
		if( !KoSectionUser::create( section ) ) return false;
		_off = off;
		_size = size;
		return true;
	}
	virtual bool close()
	{
		_off = 0;
		_size = 0;
		return KoSectionUser::close();
	}
	virtual void onSectionBlockInvalid( uint64_t off )
	{
		if( off < _off ) return ;
		uint64_t voff = off - _off;
		printf( "view block invalid at %I64X\n", voff );
		// notify page-table
	}
};

HRESULT main()
{
	///////////////////////////////
	vm::PageTable pgt;
	pgt.vmmap( 0x1000, 0x9000, vm::PageTable::R );
	pgt.vmmap( 0x2000, 0x9000, vm::PageTable::R );
	pgt.vmmap( 0x7FFB0000000LL, 0x9000, vm::PageTable::R );

	pgt.load();

	KoSection section;
	section.create( 0x1000*100, 0 );

	UoView view;
	view.create( &section, 0x1000, 0x2000 );
	for( size_t addr = 0; addr < 0x1000*100; addr += 0x1000 )
	{
		vm::AddressRef* pref = section.get_block( addr );
		printf( "off: %X, pma: %X\n", addr, pref->addr );
	}

	UoPrivate priv;
	for( size_t i = 0; i < 1000000; ++ i )
		priv.get_priv_block( i<<12, true );

	///////////////////////////////
	pgt.load();
	uint64_t rma = 0;
	pgt.get_pma( 0x2000, &rma );
	///////////////////////////////

	///////////////////////////////
	vin::kvfs obfs;

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\System32\\drivers\\etc" );
	// i put all loaded image section to .image directory, 
	// when i load
	obfs.createPath( "\\.Images\\Device\\HarddiskVolume1\\Windows\\System32\\kernel32.dll", true, new KoSection() );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\System32\\kernel32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\ntdll.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\advapi32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\user32.dll", true, new vin::KoWkmImage() );
	obfs.createPath( "\\Device\\HarddiskVolume1\\windows\\System32\\gdi32.dll", true, new vin::KoWkmImage() );

	obfs.createPath( "\\BaseNamedObjects\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\BaseNamedObjects\\d:", true, new vin::KoSymbolLink("\\Device\\harddiskvolume2") );

	obfs.createPath( "\\Global", true, new vin::KoSymbolLink("\\BaseNamedObjects") );
	obfs.createPath( "\\Local", true, new vin::KoSymbolLink("\\BaseNamedObjects") );
	obfs.createPath( "\\Session", true, new vin::KoSymbolLink("\\Session\\BNOLINKS") );

	obfs.createPath( "\\SystemRoot", true, new vin::KoSymbolLink("\\Device\\BootDevice") );
	obfs.createPath( "\\??", true, new vin::KoSymbolLink("\\BaseNamedObjects") );

	// all dos device need put at root, 
	obfs.createPath( "\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\d:", true, new vin::KoSymbolLink("\\Device\\harddiskvolume2") );

	obfs.createPath( "\\DosDevice", true, new vin::KoSymbolLink("\\") );

	// all dos drives put to \\.DosDrives, help converting UNC style path to Dos style path, 
	obfs.createPath( "\\.DosDrives\\c:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );
	obfs.createPath( "\\.DosDrives\\d:", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume2") );

	obfs.createPath( "\\Device\\BootDevice", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1") );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\SysWOW64", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1\\windows\\system32") );

	obfs.createPath( "\\Device\\HarddiskVolume1\\Windows\\SysWOW64", true, new vin::KoSymbolLink("\\Device\\HarddiskVolume1\\windows\\system32") );

	//vin::vfs::node_t * kernel32 = obfs.getNodeByPath( "\\\\knowndlls\\lsp.dll" );
	//vin::KoSymbolLink * x =  kernel32->mounted()->instantiate<vin::KoSymbolLink>();

	vin::vfs::node_t * system32 = obfs.getNodeByPath( "\\??\\c:\\windows\\system32\\" );
	vin::vfs::node_t * user32 = obfs.getNodeByPath( "\\\\.\\c:\\windows\\SysWOW64\\kernel32.dll\\" );
	vin::KoWkmImage* of = ( user32->mounted()->instantiate<vin::KoWkmImage>() );
	if( of )
	{
		of->setAccess( FILE_READ_DATA );
	}
	vin::vfs::node_t * drives = obfs.getNodeByPath( "\\.DosDrives" );

	//vin::vfs::node_t * vol_c = obfs.getNodeByPath( "c:", false );
	std::string fname;
	user32->getFullPath( fname );
	printf( "%s\n", fname.c_str() );

	vin::vfs::node_t * file = 0;
	while( file = drives->next( file ) )
	{
		std::string tmp;
		file->getFullPath(tmp);
		printf( "%s\n", tmp.c_str() );
	}

	// vfs ok
	// ram ok
	// amd64 ok
	// address space ok
	// loader X
	// thread X
	// process X
	// process environment X
	// 

	//face = (vin::KoSymbolLink*)->mounted()->asType(vin::KoSymbolLink::ObId );
	//refp<vfs::node_t> vol = new vfs::node_t();
	//root->addChild( "c:", vol );
	//root->getNode("c:")->mount = new vin::KoSymbolLink("d:");

	//HANDLE hFile = CreateFileA( "f:\\conscan.exe\\", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
	//HANDLE hMap = CreateFileMappingA( hFile, 0, PAGE_READONLY, 0, 0, 0 );
	//PVOID vp = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0x10000 );
	//{
	//	RFAILED( mgr.Create( "abc.bin", 99 ) );
	//	Private<100> oPrivate;
	//	oPrivate.Commit();
	//	mgr.RamDirectWrite( oPrivate._Refs, 0, "Hello", 5 );
	//	mgr.RamSwapOut( oPrivate._Refs, FALSE );
	//	mgr.RamDirectWrite( oPrivate._Refs, 5, "World", 5 );
	//	CHAR szText[10] = {};
	//	mgr.RamDirectRead( oPrivate._Refs, 0, szText, 10 );
	//	mgr.RamSwapIn( oPrivate._Refs );
	//}
	system("pause");
}


#endif