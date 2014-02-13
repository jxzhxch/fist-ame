#include "stdafx.h"
#include "kmo.h"


namespace vin
{
	KmoSection::KmoSection() 
		: _size(0)
		, _acc(0)
		, _file(0)
	{
		for( size_t i = 0; i < WINDOW_BLOCKS; ++ i )
		{
			_wnd[i].owner = this;
		}
	}

	HRESULT KmoSection::create( KboFileIO* f, uint64_t size, uint32_t acc )
	{
		uint32_t facc = 0;
		RFAILED( f->access( &facc ) );
		uint32_t fw = facc & FILE_WRITE_ACCESS;
		uint32_t sw = acc & SECTION_MAP_WRITE;
		if( !fw && sw ) return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
		uint64_t fs = 0;
		RFAILED( f->length( &fs ) );
		if( size == (uint64_t)-1 )
			size = fs;

		if( fw && fs < size )
		{
			RFAILED( f->resize( size ) );
		}
		else
		{
			
		}
		return S_OK;
	};

	void KmoSection::insertUser( KmoSectionUser* p )
	{
		_users.insert( p );
	}

	void KmoSection::removeUser( KmoSectionUser* p )
	{
		_users.erase( p );
	}

	vram::RamBlock* KmoSection::getRamBlock( uint64_t offset )
	{
		if( offset >= _size ) return NULL;
		uint64_t index = offset >> VM_PAGE_SHIFT;
		size_t index32 = index % WINDOW_BLOCKS;
		Block* bp = _wnd + index32;
		if( bp->flags & vram::BF_VALID )
		{
			if( _acc & SECTION_MAP_WRITE )
			{
				// Write Access
				if( _type == SEC_FILE )
				{
					RFAILED_( vram::SwapIn( bp ), NULL );
					RFAILED_( _file->seek( offset, SEEK_SET ), NULL );
					RFAILED_( _file->write( vram::Buffer(bp), VM_PAGE_SIZE, 0 ), NULL );
					RFAILED_( vram::Free( bp ), NULL );
				}
				else
				{
					// Only File can write back
					return NULL;
				}
			}
			else
			{
				// Read Only, just free it
				RFAILED_( vram::Free( bp ), NULL );
			}
		}
		
		bp->offset = offset;

		RFAILED_( vram::Alloc( bp, 0 ), NULL );
		if( FAILED(_file->seek( offset, SEEK_SET )) ||
			FAILED(_file->read( vram::Buffer(bp), VM_PAGE_SIZE, 0 )) )
		{
			vram::Free( bp );
			return NULL;
		}
		return bp;
	}
	void KmoSection::onBlockInvalid( vram::RamBlock* pb )
	{
		Block* bp = (Block*)pb;
		if( !bp ) return ;
		users_t::iterator it = _users.begin();
		for( ; it != _users.end(); ++ it )
		{
			(*it)->onBlockInvalid( bp->offset );
		}
	}
};
