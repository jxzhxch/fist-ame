#ifndef __BITMAP0__
#define __BITMAP0__

#include <vector>

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

#endif