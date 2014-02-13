#ifndef __KBO__
#define __KBO__

namespace vin
{
	struct KboFileIO
	{
		virtual long access( uint32_t * acc ) = 0;
		virtual long seek( int64_t off, size_t method ) = 0;
		virtual long tell( uint64_t* ptr ) = 0;
		virtual long read( void* pb, size_t cb, size_t * pcb ) = 0;
		virtual long write( const void* pb, size_t cb, size_t * pcb ) = 0;
		virtual long length( uint64_t* len ) = 0;
		virtual long resize( uint64_t len ) = 0;
	};
};


#endif