
```


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


```