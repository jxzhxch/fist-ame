
```


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

```