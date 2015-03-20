
```
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
```