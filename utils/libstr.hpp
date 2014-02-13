#ifndef __RX_TCS__
#define __RX_TCS__

#include <string>
#include <vector>

namespace tcs
{
	typedef CHAR		TCH8;
	typedef UINT16		TCH16;
	typedef UINT32		TCH32;
	// strlen
	template < class XCHAR >
	size_t len( const XCHAR * p )
	{
		const XCHAR *eos = p;
		while( *eos++ ) ;
		return( eos - p - 1 );
	}

	// strlwr
	template < class XCHAR >
	XCHAR* lwr( XCHAR * p )
	{
		for( XCHAR * p1 = p; *p1; ++ p1 ) *p1 = ::tolower(*p1);
		return p;
	}

	// strupr
	template < class XCHAR >
	XCHAR* upr( XCHAR * p )
	{
		for( XCHAR * p1 = p;*p1; ++ p1 ) *p1 = ::toupper(*p1);
		return p;
	}

	template < class XCHAR >
	XCHAR* dup( const XCHAR * p )
	{
		size_t cch = tcs::len(p);
		void * p2 = malloc( (cch+1)*sizeof(XCHAR) );
		if( !p2 ) return NULL;
		memcpy( p2, p, (cch+1)*sizeof(XCHAR) );
		return (XCHAR*)p2;
	}

	template < class XCHAR >
	XCHAR* chr( XCHAR* p, int ch )
	{
		while (*p && *p != (XCHAR)ch)
			p++;

		if (*p == (XCHAR)ch)
			return((XCHAR *)p);
		return(NULL);
	}

	template < class XCHAR >
	XCHAR * rchr ( const XCHAR * string,int ch)
	{
		XCHAR *start = (XCHAR *)string;

		while (*string++)                       /* find end of string */
			;
		/* search towards front */
		while (--string != start && *string != (XCHAR)ch)
			;

		if (*string == (XCHAR)ch)                /* char found ? */
			return( (XCHAR *)string );

		return(NULL);
	}


	template < class XCHAR >
	XCHAR* str( const XCHAR* str1, const XCHAR* str2 )
	{
		XCHAR *cp = (XCHAR *) str1;
		XCHAR *s1, *s2;

		if ( !*str2 )
			return((XCHAR *)str1);

		while (*cp)
		{
			s1 = cp;
			s2 = (XCHAR *) str2;

			while ( *s1 && *s2 && !(*s1-*s2) )
				s1++, s2++;

			if (!*s2)
				return(cp);

			cp++;
		}

		return(NULL);
	}

	template < class XCHAR >
	int icmp( const XCHAR * src, const XCHAR * dst )
	{
		int delta = 0;
		for( ; *src && *dst; ++ src, ++ dst )
		{
			delta = ::tolower(*src) - ::tolower(*dst);
			if( delta ) return delta;
		}
		return *src - *dst;
	}
	template < class XCHAR >
	int cmp( const XCHAR * src, const XCHAR * dst )
	{
		int delta = 0;
		for( ; *src && *dst; ++ src, ++ dst )
		{
			delta = (*src) - (*dst);
			if( delta ) return delta;
		}
		return *src - *dst;
	}

	template < class XCHAR >
	int nicmp( const XCHAR * src, const XCHAR * dst1, size_t count )
	{
		const XCHAR* dst = (const XCHAR*)dst1;
		if(count)
		{
			int f=0;
			int l=0;
			do
			{
				f = ::tolower(*src++);
				l = ::tolower(*dst++);
			}
			while ( --count && f && (f == l) );

			return ( f - l );
		}
		else
		{
			return 0;
		}
	}
	template < class XCHAR >
	int ncmp( const XCHAR * src, const XCHAR * dst, size_t count )
	{
		if(count)
		{
			int f=0;
			int l=0;
			do
			{
				f = (*src++);
				l = (*dst++);
			}
			while ( --count && f && (f == l) );

			return ( f - l );
		}
		else
		{
			return 0;
		}
	}

	template < class XCHAR >
	XCHAR* cpy( XCHAR* dst, const XCHAR* src )
	{
		XCHAR * cp = dst;

		while( *cp++ = *src++ )
			;               /* Copy src over dst */

		return( dst );
	}

	template < class XCHAR >
	XCHAR * ncpy ( XCHAR * dest, const XCHAR * source, size_t count )
	{
		XCHAR *start = dest;

		while (count && (*dest++ = *source++))    /* copy string */
			count--;

		if (count)                              /* pad out with zeroes */
			while (--count)
				*dest++ = '\0';

		return(start);
	}

	template < class XCHAR >
	XCHAR * cat (XCHAR * dst,const XCHAR * src)
	{
		XCHAR * cp = dst;
		while( *cp ) cp++;				/* find end of dst */
		while( *cp++ = *src++ );		/* Copy src to end of dst */
		return( dst );					/* return dst */
	}

	template < class XCHAR >
	XCHAR * ncat (XCHAR * front, const XCHAR * back, size_t count )
	{
		XCHAR *start = front;
		while (*front++) ;
		front--;
		while (count--)
			if (!(*front++ = *back++))
				return(start);
		*front = '\0';
		return(start);
	}

	template < class XCHAR >
	XCHAR * str (XCHAR * str1, const XCHAR * str2 )
	{
		XCHAR *cp = (XCHAR *) str1;
		XCHAR *s1, *s2;

		if ( !*str2 )
			return((XCHAR *)str1);

		while (*cp)
		{
			s1 = cp;
			s2 = (XCHAR *) str2;

			while ( *s1 && *s2 && !(*s1-*s2) )
				s1++, s2++;

			if (!*s2)
				return(cp);

			cp++;
		}

		return(NULL);
	}
	//////////////////////////////////////////////////////////////////////////


	// =============================================================================
	// str to int
	template<bool used_char, bool valid_arg>
	struct __str2int
	{
		template<int base, class _Ty, class _CharType>
		inline static _Ty convert(_CharType *ptr, size_t size)
		{
			return 0;
		}
	};

	template<>
	struct __str2int<false, true>
	{
		template<int base, class _Ty, class _CharType>
		inline static _Ty convert(_CharType *ptr, size_t size)
		{

			_Ty ret = 0;
			while(size-->0 && 0x30 <= *ptr && *ptr <0x30+base)
			{
				ret = ret*base + (*ptr-0x30);
				++ptr;
			}
			return ret;
		}
	};

	template<>
	struct __str2int<true, true>
	{
		template<int base, class _Ty, class _CharType>
		inline static _Ty convert(_CharType *ptr, size_t size)
		{
			_Ty ret=0;
			for(; ;)
			{
				if (0 == size--) break;
				if (0x30 <= *ptr && *ptr <= 0x39)
				{
					ret = ret*base + (*ptr-0x30);
				}
				else if (0x41 <= *ptr && *ptr < 0x41+base-10)
				{
					ret = ret*base + (*ptr-0x41+10);
				}
				else if (0x61 <= *ptr && *ptr < 0x61+base-10)
				{
					ret = ret*base + (*ptr-0x61+10);
				}
				else
					break;
				++ptr;
			}
			return ret;
		}
	};

	template<int base, class _Ty, class _CharType>
	_Ty str2int(_CharType *ptr, size_t size)
	{
		size_t i=0;
		for(; i<size && 0x20==ptr[i]; ++i);
		return __str2int<(base>10), (2<=base && base<=36)>::template convert<base, _Ty, _CharType>(ptr+i, size-i);
	}


	// int to str
	template<bool used_char, bool valid_arg>
	struct __int2str
	{
		template<int base, class _Ty, class _CharType>
		inline static bool convert(_Ty, _CharType, size_t) { return false; }
	};

	template<>
	struct __int2str<false, true>
	{
		template<int base, class _Ty, class _CharType>
		inline static bool convert(_Ty val, _CharType *ptr, size_t size)
		{
			_CharType *ptrend = ptr+size-1;
			*ptrend-- = 0;
			while (ptrend >= ptr)
			{
				if (0 == val) break;
				*ptrend-- =  static_cast<_CharType>((val % base) + 0x30);
				val /= base;
			}
			if (0 == val)
			{
				while(ptrend >= ptr) *ptrend-- = 0x30;
				return true;
			}
			return false;
		}
	};

	template<>
	struct __int2str<true, true>
	{
		template<int base, class _Ty, class _CharType>
		inline static bool convert(_Ty val, _CharType *ptr, size_t size)
		{
			_CharType *ptrend = ptr+size-1;
			*ptrend-- = 0;
			while (ptrend >= ptr)
			{
				if (0 == val) break;
				_CharType cur = static_cast<_CharType>((val % base));
				*ptrend-- =  cur < 10 ? cur+0x30 : cur-10+0x41;
				val /= base;
			}
			if (0 == val)
			{
				while(ptrend >= ptr) *ptrend-- = 0x30;
				return true;
			}
			return false;
		}
	};

	template<int base, class _Ty, class _CharType>
	bool int2str(_Ty val, _CharType *ptr, size_t size)
	{
		return __int2str<(base>10), (2<=base && base<=36)>::template convert<base, _Ty, _CharType>(val, ptr, size);
	}


	// string ================================================================
#if TARGET_OS==OS_WINDOWS
	//
	inline size_t wtoa(char *mbstr, const wchar_t *wcstr, size_t count)
	{
		return ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1, mbstr, (int)count, NULL, NULL);
	}

	inline size_t atow(wchar_t *wcstr, const char *mbstr, size_t count)
	{
		return ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1, wcstr, (int)count);
	}

#else
	//
	inline size_t wtoa(char *mbstr, const wchar_t *wcstr, size_t count)
	{
		return wcstombs(mbstr, wcstr, count)+1;
	}

	inline size_t atow(wchar_t *wcstr, const char *mbstr, size_t count)
	{
		return mbstowcs(wcstr, mbstr, count)+1;
	}

#endif

#if (TARGET_OS==OS_WINDOWS)

	struct xstr_t
	{
		UINT				CodePage;
		std::string			StrMBS;
		std::wstring		StrWCH;

		xstr_t()
		{
			CodePage = GetACP();
		}
		xstr_t( UINT cp )
		{
			CodePage = cp;
		}
		bool set( XSTRP& ostr, UINT cp )
		{
			if( ostr.IsUnicode )
				return set( ostr.NameW );
			else
				return set( cp, ostr.NameA );
		}
		bool set( REFCXSTRP ostr )
		{
			if( ostr.IsUnicode )
				return set( ostr.NameW );
			else
				return set( GetACP(), ostr.NameA );
		}
		bool set( PROPVARIANT& ovar )
		{
			if( ovar.vt == VT_LPWSTR )
				return set( ovar.pwszVal );
			else if( ovar.vt == VT_LPSTR )
				return set( GetACP(), ovar.pszVal );
			else
				return false;
		}
		bool set( const wchar_t * p )
		{
			StrWCH.assign( p );
			return chcp(CodePage);
		}
		bool set( const wchar_t * p, size_t cch )
		{
			StrWCH.assign( p, cch );
			return chcp(CodePage);
		}
		bool set( UINT cp, const char * p )
		{
			int need = MultiByteToWideChar( cp, 0, (LPCSTR)p, -1, NULL, 0 );
			if( 0 == need ) return false;
			//int err = GetLastError();
			CodePage = cp;
			StrWCH.resize(need);
			MultiByteToWideChar( cp, 0, (LPCSTR)p, -1, &StrWCH[0], need );
			StrWCH.assign(StrWCH.c_str());
			StrMBS.assign(p);
			return true;
		}
		bool set( UINT cp, const char * p, size_t cch )
		{
			int need = MultiByteToWideChar( cp, 0, (LPCSTR)p, cch, NULL, 0 );
			if( 0 == need ) return false;
			//DWORD err = GetLastError();
			CodePage = cp;
			StrWCH.resize(need);
			MultiByteToWideChar( cp, 0, (LPCSTR)p, cch, &StrWCH[0], need );
			StrWCH.assign(StrWCH.c_str());
			StrMBS.assign(p,cch);
			return true;
		}
		bool chcp( UINT cp )
		{
			DWORD need = WideCharToMultiByte( cp, 0, StrWCH.c_str(), (int)StrWCH.size(), NULL, 0, NULL, NULL );
			if( 0 == need ) return false;
			CodePage = cp;
			StrMBS.resize(need);
			WideCharToMultiByte( cp, 0, StrWCH.c_str(), (int)StrWCH.size(), &StrMBS[0], need, NULL, NULL );
			StrMBS.assign(StrMBS.c_str());
			return true;
		}
		LPCSTR a()
		{
			if( StrMBS.empty() ) return "";
			return StrMBS.c_str();
		}
		LPCWSTR w()
		{
			if( StrWCH.empty() ) return L"";
			return StrWCH.c_str();
		}
		DWORD cp()
		{
			return CodePage;
		}
		operator LPCSTR ()
		{
			return a();
		}
		operator LPCWSTR()
		{
			return w();
		}
		operator std::string& ()
		{
			return StrMBS;
		}
		operator std::wstring& ()
		{
			return StrWCH;
		}
	};

#else

	struct xstr_t
	{
		UINT				CodePage;
		std::string			StrMBS;
		std::wstring		StrWCH;

		xstr_t()
		{
			CodePage = 0;
		}
		xstr_t( UINT cp )
		{
			CodePage = cp;
		}
		bool set( XSTRP& ostr, UINT cp )
		{
			if( ostr.IsUnicode )
				return set( ostr.NameW );
			else
				return set( cp, ostr.NameA );
		}
		bool set( REFCXSTRP ostr )
		{
			if( ostr.IsUnicode )
				return set( ostr.NameW );
			else
				return set( 0, ostr.NameA );
		}
		bool set( PROPVARIANT& ovar )
		{
			if( ovar.vt == VT_LPWSTR )
				return set( ovar.pwszVal );
			else if( ovar.vt == VT_LPSTR )
				return set( 0, ovar.pszVal );
			else
				return false;
		}
		bool set( const wchar_t * p )
		{
			StrWCH.assign( p );
			return chcp(CodePage);
		}
		bool set( const wchar_t * p, size_t cch )
		{
			StrWCH.assign( p, cch );
			return chcp(CodePage);
		}
		bool set( UINT cp, const char * p )
		{
			StrMBS.assign(p);
			size_t need = mbstowcs( 0, p, -1 );
			if( (size_t)-1 == need ) return false;
			CodePage = cp;
			StrWCH.resize(need);
			mbstowcs( &StrWCH[0], p, need );
			StrWCH.assign(StrWCH.c_str());
			return true;
		}
		bool set( UINT cp, const char * p, size_t cch )
		{
			StrMBS.assign(p,cch);
			size_t need = mbstowcs( 0, p, cch );
			if( (size_t)-1 == need ) return false;
			CodePage = cp;
			StrWCH.resize(need);
			mbstowcs( &StrWCH[0], p, need );
			StrWCH.assign(StrWCH.c_str());
			return true;
		}
		bool chcp( UINT cp )
		{
			size_t need = wcstombs( 0, StrWCH.c_str(), (int)StrWCH.size() );
			if( (size_t)-1 == need ) return false;
			CodePage = cp;
			StrMBS.resize(need);
			wcstombs( &StrMBS[0], StrWCH.c_str(), StrWCH.size() );
			StrMBS.assign(StrMBS.c_str());
			return true;
		}
		LPCSTR a()
		{
			if( StrMBS.empty() ) return "";
			return StrMBS.c_str();
		}
		LPCWSTR w()
		{
			if( StrWCH.empty() ) return L"";
			return StrWCH.c_str();
		}
		DWORD cp()
		{
			return CodePage;
		}
		operator LPCSTR ()
		{
			return a();
		}
		operator LPCWSTR()
		{
			return w();
		}
		operator std::string& ()
		{
			return StrMBS;
		}
		operator std::wstring& ()
		{
			return StrWCH;
		}
	};

#endif

	static inline int ascii_tolower( int ch )
	{
		if( 'A' <= ch && ch <= 'Z' )
			return ch - 'A' + 'a' ;
		return ch;
	}

	static inline int bicmp( const void * first, const void * last, size_t count )
	{
		int f = 0;
		int l = 0;

		while ( count-- )
		{
			if ( (*(unsigned char *)first == *(unsigned char *)last) ||
				((f = ascii_tolower( *(unsigned char *)first )) == (l = ascii_tolower( *(unsigned char *)last ))) )
			{
				first = (char *)first + 1;
				last = (char *)last + 1;
			}
			else
				break;
		}
		return ( f - l );
	}

	static inline bool xprintf( std::string& cont, const char * fmt, ... )
	{
		if( !fmt ) return false;
		va_list al;
		va_start( al, fmt);
		int ch = vsnprintf( 0, 0, fmt, al );
		if( ch < 0 ) return false;
		++ ch;
		std::vector< char > _buffer( ch );
		va_start( al, fmt);
		ch = vsnprintf( &_buffer[0], ch, fmt, al );
		if( ch < 0 ) return false;
		cont.append( &_buffer[0], ch );
		return true;
	};

	static inline bool wxprintf( std::wstring& cont, const wchar_t * fmt, ... )
	{
		if( !fmt ) return false;
		va_list al;
		va_start( al, fmt);
		int ch = vswprintf( 0, 0, fmt, al );
		if( ch < 0 ) return false;
		++ ch;
		std::vector< wchar_t > _buffer( ch );
		ch = vswprintf( &_buffer[0], ch, fmt, al );
		if( ch < 0 ) return false;
		cont.append( &_buffer[0], ch );
		return true;
	};

	static inline int bncmp( const void * p1, const void * p2, size_t c )
	{
		const UINT8 * pb1 = (const UINT8 *)p1;
		const UINT8 * pb1e = pb1 + c;
		const UINT8 * pb2 = (const UINT8 *)p2;
		while( pb1 < pb1e )
		{
			int diff = *pb1++ - *pb2++;
			if( diff ) return diff;
		}
		return 0;
	}
	template < class XCHAR, class NUMTYPE >
	static inline bool hex2lui( const XCHAR* p, NUMTYPE& val )
	{
		val = 0;
		for( ; *p; ++ p )
		{
			int ch = ascii_tolower(*p);
			if( ( ch >= '0' ) && ( ch <= '9' ) )
			{
				val <<= 4;
				val += ch-'0';
			}
			else if( ( ch >= 'a' ) && ( ch <= 'f' ) )
			{
				val <<= 4;
				val += (10+ch-'a');
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	template < class XCHAR >
	size_t dropz( const XCHAR* p, char * outp, size_t cb )
	{
		size_t xl = len(p);
		const char * pc = (const char *)p;
		xl *= sizeof(XCHAR);
		size_t cc = 0;
		for( size_t i = 0; i < xl; ++ i )
		{
			if( !pc[i] ) continue;
			if( outp && cc < cb ) outp[cc] = pc[i];
			++ cc;
		}
		if( cc < cb ) outp[cc++] = 0;
		return cc;
	}

};

//#define strlen		tcs::len<TCH8>
#define tcsdup			tcs::dup
#define tcslen			tcs::len
#define tcslwr			tcs::lwr
#define tcsupr			tcs::upr
#define tcschr			tcs::chr
#define tcsstr			tcs::str
#define tcscmp			tcs::cmp
#define tcsicmp			tcs::icmp
#define tcsncmp			tcs::ncmp
#define tcsnicmp		tcs::nicmp
#define tcscpy			tcs::cpy
#define tcsncpy			tcs::ncpy
#define tcscat			tcs::cat
#define tcsncat			tcs::ncat
#define memicmp			tcs::bicmp
#define tcsstr			tcs::str
#define tcsrchr			tcs::rchr

#define int2bstr		tcs::int2str<2, unsigned long, char>
#define int2ostr		tcs::int2str<8, unsigned long, char>
#define int2dstr		tcs::int2str<10, unsigned long, char>
#define int2hstr		tcs::int2str<16, unsigned long, char>

#define bstr2int		tcs::str2int<2, unsigned long, const char>
#define ostr2int		tcs::str2int<8, unsigned long, const char>
#define dstr2int		tcs::str2int<10, unsigned long, const char>
#define hstr2int		tcs::str2int<16, unsigned long, const char>

//////////////////////////////////////////////////////////////////////////

// OLE CHAR <=> WIDE CHAR ================================================================
#include <vector>

// wide char to ole char
template<bool> struct __olechar {};

template<> struct __olechar<true>
{
	LPCWSTR m_pwc;
	__olechar(LPCWSTR pwc) : m_pwc(pwc) {}
	operator const OLECHAR* () const { return (const OLECHAR*)m_pwc; }
};

template<> struct __olechar<false>
{
	std::vector<OLECHAR> m_olechar;
	__olechar(LPCWSTR pwc)
	{
		m_olechar.resize(wcslen(pwc)+1);
		if(m_olechar.empty()) { return; }
		for(size_t i = 0; i < m_olechar.size(); ++i) { m_olechar[i] = (OLECHAR)pwc[i]; }
		m_olechar[m_olechar.size()-1] = 0;
	}
	operator const OLECHAR* () const { return m_olechar.empty() ? (const OLECHAR*)L"" : &m_olechar[0]; }
};

struct wc_2_oc
{
	__olechar<(sizeof(wchar_t)==sizeof(OLECHAR))> m_olechar;
	wc_2_oc(LPCWSTR pwc) : m_olechar(pwc) {}
	operator const OLECHAR* () const { return m_olechar; }
};

// ole char to wide char
template <bool> struct __widechar { };

template <> struct __widechar<true>
{
	LPCOLESTR m_pole;
	__widechar(LPCOLESTR pole) : m_pole(pole) { }
	operator LPCWSTR () const { return (LPCWSTR)m_pole; }
};

template <> struct __widechar<false>
{
	std::vector<WCHAR> m_widechar;
	__widechar(LPCOLESTR pole)
	{
		m_widechar.resize(tcslen(pole) + 1);
		if(m_widechar.empty()) { return; }
		for (size_t i = 0; i < m_widechar.size(); ++i) { m_widechar[i] = (WCHAR)pole[i]; }
		m_widechar[m_widechar.size()-1] = 0;
	}
	operator LPCWSTR () const { return m_widechar.empty() ? L"" : &m_widechar[0]; }
};

struct oc_2_wc
{
	__widechar<(sizeof(wchar_t)==sizeof(OLECHAR))> m_widechar;
	oc_2_wc(LPCOLESTR pole) : m_widechar(pole) { }
	operator LPCWSTR () const { return m_widechar; }
};


#if !defined(RS_BIG_ENDIAN)
#define oc_2_wc_sbo oc_2_wc
#else
struct __widechar_sbo // swap byte order
{
	std::vector<WCHAR> m_widechar;
	__widechar_sbo(LPCOLESTR pole)
	{
		m_widechar.resize(tcslen(pole) + 1);
		if(m_widechar.empty()) { return; }
		for (size_t i = 0; i < m_widechar.size(); ++i) { m_widechar[i] = (WCHAR)GET_AS_LE16(&pole[i]); }
		m_widechar[m_widechar.size()-1] = 0;
	}
	operator LPCWSTR () const { return m_widechar.empty() ? L"" : &m_widechar[0]; }
};

struct oc_2_wc_sbo // swap byte order
{
	__widechar_sbo m_widechar;
	oc_2_wc_sbo(LPCOLESTR pole) : m_widechar(pole) { }
	operator LPCWSTR () const { return m_widechar; }
};
#endif


#endif
