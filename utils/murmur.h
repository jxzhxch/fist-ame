#ifndef __MURMUR_HASH__
#define __MURMUR_HASH__

template< class T >
static FORCEINLINE uint32_t MurmurHash1( const T * pdat, size_t cnt, uint32_t seed )
{
	const void * key = (const void *)pdat;
	uint32_t len = (uint32_t)(sizeof(T)*cnt);

	const uint32_t m = 0xc6a4a793;

	const uint32_t r = 16;

	uint32_t h = seed ^ (len * m);

	//----------

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		uint32_t k = (UINT32)GET_AS_LE32(data);

		h += k;
		h *= m;
		h ^= h >> 16;

		data += 4;
		len -= 4;
	}

	//----------

	switch(len)
	{
	case 3:
		h += data[2] << 16;
	case 2:
		h += data[1] << 8;
	case 1:
		h += data[0];
		h *= m;
		h ^= h >> r;
	};

	//----------

	h *= m;
	h ^= h >> 10;
	h *= m;
	h ^= h >> 17;

	return (uint32_t)h;
} 

template < class T >
static FORCEINLINE T lv( T v )
{
	unsigned char * pch = (unsigned char*)&v;
	unsigned char * pend = pch + sizeof(T);
	for( ; pch < pend; ++ pch )
	{
		unsigned char ch = *pch;
		if( (ch & 0x20) ) continue;
		if( ch >= 'A' && ch <= 'Z' ) *pch = ch + 'a' - 'A';
	}
	return v;
}

static FORCEINLINE unsigned char lv( unsigned char v )
{
	return ((v&0x20)||(v<'A'||v>'Z')) ? v : (v+'a'-'A');
}

template< class T >
static FORCEINLINE uint32_t MurmurHash1LowCase( const T * pdat, size_t cnt, uint32_t seed )
{
	const void * key = (const void *)pdat;
	uint32_t len = (uint32_t)(sizeof(T)*cnt);

	const uint32_t m = 0xc6a4a793;

	const uint32_t r = 16;

	uint32_t h = seed ^ (len * m);

	//----------

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		// always treat as LOW-ENDIAN
		uint32_t k = lv((UINT32)GET_AS_LE32(data));

		h += k;
		h *= m;
		h ^= h >> 16;

		data += 4;
		len -= 4;
	}

	//----------

	switch(len)
	{
	case 3:
		h += lv(data[2]) << 16;
	case 2:
		h += lv(data[1]) << 8;
	case 1:
		h += lv(data[0]);
		h *= m;
		h ^= h >> r;
	};

	//----------

	h *= m;
	h ^= h >> 10;
	h *= m;
	h ^= h >> 17;

	return (uint32_t)h;
} 

#endif