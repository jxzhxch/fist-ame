#ifndef __GET_OPTION__
#define __GET_OPTION__

#include <map>
#include <vector>
#include <string>

#ifndef WIN32
extern unsigned char _mbctype[];
#define _M1     0x04    /* MBCS 1st (lead) byte */
#define _ismbblead(_c)  ((_mbctype+1)[(unsigned char)(_c)] & _M1)


#ifdef  _UNICODE
		#define _T(x)      L ## x
#else   /* _UNICODE */        
		#ifdef _T
			#undef _T
		#endif
		#define _T(x)      x
#endif /* _UNICODE */

#endif


#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')
#define TABCHAR		_T('\t')
#define NULCHAR		_T('\x00')
#define SLASHCHAR   _T('\\')
#define XSLASHCHAR  _T('/') 

typedef wchar_t	swchar;
typedef wchar_t	uwchar;

static void __cdecl parse_cmdline( const swchar * cmdstart, swchar **argv, swchar *args, int *numargs,int *numchars)
{
	const swchar *p;
	int inquote;		    /* 1 = inside quotes */
	int copychar;		   /* 1 = copy char to *args */
	unsigned numslash;	   /* num of backslashes seen */

	*numchars = 0;
	*numargs = 0;		   /* the program name at least */

	inquote = FALSE;
	p = cmdstart;

	/* loop on each argument */
	for(;;) {

	    if ( *p ) {
			while (*p == SPACECHAR || *p == TABCHAR)
				++p;
		}

		if (*p == NULCHAR)
			break;	      /* end of args */

		/* scan an argument */
		if (argv)
			*argv++ = args;     /* store ptr to arg */
	    
		++*numargs;

//#ifdef _UNICODE
//	/* To handle later wild card expansion, we prefix each entry by
//	it's first character before quote handling.  This is done
//	so _[w]cwild() knows whether to expand an entry or not. */
//	if (args)
//	    *args++ = *p;
//	++*numchars;
//
//#endif  /* _UNICODE */

	/* loop through scanning one argument */
	for (;;) {
	    copychar = 1;
	    /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
	       2N+1 backslashes + " ==> N backslashes + literal "
	       N backslashes ==> N backslashes */
	    numslash = 0;
	    while (*p == SLASHCHAR) {
			/* count number of backslashes for use below */
			++p;
			++numslash;
	    }
	    if (*p == DQUOTECHAR) {
		/* if 2N backslashes before, start/end quote, otherwise
		    copy literally */
			if (numslash % 2 == 0) {
				if (inquote && p[1] == DQUOTECHAR) {
					p++;    /* Double quote inside quoted string */
				} else {    /* skip first quote char and copy second */
					copychar = 0;       /* don't copy quote */
					inquote = !inquote;
				}
			}
			numslash /= 2;	  /* divide numslash by two */
	    }

	    /* copy slashes */
	    while (numslash--) {
			if (args)
				*args++ = SLASHCHAR;
			++*numchars;
	    }

	    /* if at end of arg, break loop */
	    if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
			break;

	    /* copy character into argument */
#ifndef _UNICODE
	    if (copychar) {
		if (args) {
		    if (_ismbblead(*p)) {
			*args++ = *p++;
			++*numchars;
		    }
		    *args++ = *p;
		} else {
		    if (_ismbblead(*p)) {
			++p;
			++*numchars;
		    }
		}
		++*numchars;
	    }
	    ++p;
#else  /* _MBCS */
	    if (copychar) {
			if (args)
				*args++ = *p;
			++*numchars;
	    }
	    ++p;
#endif  /* _MBCS */
	    }

	    /* null-terminate the argument */

	    if (args)
			*args++ = NULCHAR;	  /* terminate string */
	    ++*numchars;
	}

	/* We put one last argument in -- a null ptr */
	if (argv)
	    *argv++ = NULL;
	++*numargs;
}

struct Arguments
{
	Arguments()
	{

	}

	typedef std::vector< std::wstring >		wstrvec;

	struct Option
	{
		wstrvec						_descs;
		std::wstring				_desc;
		bool						_special;
		size_t						_max_argc;
		std::vector<std::wstring>	_values;

		Option() : _max_argc(0),_special(false) {};

		Option( size_t c, const wchar_t * desc ) : _max_argc(c), _special(false)
		{
			_desc = desc ? desc : L"";
		};

		void appendDesc( const wchar_t * desc )
		{
			if( desc ) _descs.push_back(desc);
		}

		size_t numOfValue()
		{
			return _values.size();
		}

		bool getAsStr( std::wstring & str, size_t idx = 0 )
		{
			if( _values.size() <= idx )
			return false;
			str = _values[idx];
			return true;
		}
		bool getAsDec( unsigned int& number, size_t idx = 0 )
		{
			if( _values.size() <= idx )
			return false;
			int r = swscanf( _values[idx].c_str(), L"%d", &number );
			if( r != 1 ) return false;
			return true;
		}
		bool getAsHex( unsigned int& number, size_t idx = 0 )
		{
			if( _values.size() <= idx )
			return false;
			int r = swscanf( _values[idx].c_str(), L"%x", &number );
			if( r != 1 ) return false;
			return true;
		}
	};

	typedef std::map< std::wstring, Option >	OptionMap;
	typedef std::map< std::wstring, std::wstring >	AlaisMap;

	OptionMap		_wfields;
	AlaisMap		_walais;
	std::wstring	_foots;

	bool addAlais( const wchar_t * alais, const wchar_t * name )
	{
		if( !alais || !alais[0] ) return false;
		if( !name || !name[0] ) return false;
		_walais[alais] = name;
		return true;
	}
	bool addOption( const wchar_t * name, size_t max_opt, const wchar_t * desc )
	{
		if( !name || !name[0] ) return false;
		_wfields[name] = Option(max_opt, desc);
		return true;
	}
	bool addDesc( const wchar_t * name, const wchar_t * desc )
	{
		if( !name || !name[0] ) return false;
		if( _wfields.find(name) == _wfields.end() ) 
			return false;
		_wfields[name].appendDesc( desc );
		return true;
	}

	void setFoot( const wchar_t * desc )
	{
		_foots = desc ? desc : L"";
	}
	
	size_t parse( const wchar_t * optstr )
	{
		int argc = 0;
		int chnum = 0;
		parse_cmdline( optstr, 0, 0, &argc, &chnum );
		if( !argc || !chnum ) return false;
		std::vector< wchar_t > tmpbuf(chnum+1);
		std::vector< wchar_t * > tmpptr(argc);
		parse_cmdline( optstr, &tmpptr[0], &tmpbuf[0], &argc, &chnum );
		const wchar_t ** wargs = (const wchar_t**)&tmpptr[0];
		size_t wargc = tmpptr.size()-1;
		return parse( wargs, wargc, 0 );
	}
	size_t parse( const wchar_t ** wargs, size_t argc, size_t argi = 1 )
	{
		size_t ac = 0;
		for( size_t i = argi; i < argc; )
		{
			std::wstring warg(wargs[i]);

			OptionMap::iterator it = _wfields.find(warg);
			if( it == _wfields.end() )
				return 0;

			it->second._special = true;

			++i ;
			++ac ;

			size_t cmax = it->second._max_argc;

			size_t c = 0;
			for( ; i < argc && c < cmax; ++ i, ++ c )
			{
				std::wstring para(wargs[i]);
				OptionMap::iterator it2 = _wfields.find(para);
				if( it2 != _wfields.end() )
				{
					break;
				}
				it->second._values.push_back( para );
			}

			if( cmax != -1 && c != cmax )
				return 0;
		}
		return ac;
	}
	Option * getOption( const wchar_t * field )
	{
		OptionMap::iterator it = _wfields.find(field);
		if( it == _wfields.end() ) return NULL;
		if( !it->second._special ) return NULL;
		return &(it->second);
	}
	void help()
	{
		OptionMap::iterator it = _wfields.begin();
		wprintf( L"Options:\n" );
		for( ; it != _wfields.end(); ++ it )
		{
			Option& opt = it->second;
			wprintf( L"\t%s : %s\n", it->first.c_str(), opt._desc.c_str() );
			for( size_t i = 0; i < opt._descs.size(); ++ i )
			{
				wprintf( L"\t\t%s\n", opt._descs[i].c_str() );
			}
		}
	}
};

#endif