
```


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

```