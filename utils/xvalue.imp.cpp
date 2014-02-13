#include "stdafx.h"
#include <string>
#include <map>
#include <vector>
#include "txvalue.hpp"

using namespace xv;

//////////////////////////////////////////////////////////////////////////

struct valObject : vObject, Refalbe
{
	REFABLE_IMP_MT;

	typedef std::map< std::string, xvalue_t > ValueMap;

	ValueMap	_elements;

	virtual xv::Value& ref( const char * name )
	{
		if( !name ) throw ExcepInvalidParameter;
		size_t l = strlen(name); 
		if( !l || l > MAX_KEY_LEN ) throw ExcepInvalidParameter;
		return _elements[name];
	}

	virtual bool 	set( const char * name, const xv::Value & item )
	{
		if( !name ) return false;
		size_t l = strlen(name); 
		if( !l || l > MAX_KEY_LEN ) return false;
		_elements[name] = xvalue_t(item);
		return true;
	}
	virtual bool 	get( const char * name, xv::Value & item )
	{
		if( !name ) return false;
		//size_t l = strlen(name); if( l > 254 ) return false;
		ValueMap::iterator it = _elements.find( name );
		if( it == _elements.end() ) return false;
		return ValueSet( &(it->second), &item ) != FALSE;
	}
	virtual bool 	erase( const char * name )
	{
		if( !name ) return false;
		ValueMap::iterator it = _elements.find( name );
		if( it == _elements.end() ) return false;
		_elements.erase( it );
		return true;
	}
	virtual bool 	enum_it( _enum_cb_ptr fcb, void * context )
	{
		if( !fcb ) return false;
		ValueMap::iterator it = _elements.begin();
		size_t i = 0;
		for( ; it != _elements.end(); ++ it, ++ i )
		{
			bool last = ((i + 1) == _elements.size()); 
			if( !fcb( it->first.c_str(), it->second, last, context ) )
				return false;
		}
		return true;
	}

	virtual size_t length()
	{
		return _elements.size();
	}
	virtual bool clear()
	{
		_elements.clear();
		return true;
	}
	virtual bool clone( vComplex ** ppo )
	{
		if( !ppo ) return false;
		refp<valObject> p = new valObject();
		if( !p ) return false;
		if( !cloneTo( p ) ) return false;
		ppo[0] = p.detach();
		return true;
	}
	virtual bool cloneTo( vComplex * po )
	{
		refp<vObject> p = po;
		if( !p ) return false;
		if( !p->clear() ) return false;
		ValueMap::iterator it = _elements.begin();
		for( ; it != _elements.end(); ++ it )
		{
			if( !p->set( it->first.c_str(), it->second ) )
				return false;
		}
		return true;
	}
};

struct valArray : vArray, Refalbe
{
	REFABLE_IMP_MT;

	typedef std::map< size_t, xvalue_t > ValueMap;
	ValueMap		_elements;

	virtual xv::Value& ref( size_t idx )
	{
		return _elements[idx];
	}

	virtual bool 	set( size_t idx, const xv::Value & item )
	{
		_elements[idx] = xvalue_t(item);
		return true;
	}
	virtual bool 	get( size_t idx, xv::Value & item )
	{
		ValueMap::iterator it = _elements.find( idx );
		if( it == _elements.end() ) return false;
		return ValueSet( &(it->second), &item ) != FALSE;
	}
	virtual bool 	erase( size_t idx )
	{
		ValueMap::iterator it = _elements.find( idx );
		if( it == _elements.end() ) return false;
		_elements.erase( it );
		return true;
	}
	virtual bool 	enum_it( _enum_cb_ptr fcb, void * context )
	{
		if( !fcb ) return false;
		ValueMap::iterator it = _elements.begin();
		size_t i = 0;
		for( ; it != _elements.end(); ++ it, ++ i )
		{
			bool last = ((i + 1) == _elements.size()); 
			if( !fcb( it->first, it->second, last, context ) )
				return false;
		}
		return true;
	}
	virtual size_t length()
	{
		return _elements.size();
	}
	virtual bool clear()
	{
		_elements.clear();
		return true;
	}
	virtual bool clone( vComplex ** ppo )
	{
		if( !ppo ) return false;
		refp<vArray> p = new valArray();
		if( !p ) return false;
		if( !cloneTo( p ) ) return false;
		ppo[0] = p.detach();
		return true;
	}
	virtual bool cloneTo( vComplex * po )
	{
		refp<vArray> p = po;
		if( !p ) return false;
		if( !p->clear() ) return false;
		ValueMap::iterator it = _elements.begin();
		for( ; it != _elements.end(); ++ it )
		{
			if( !p->set( it->first, it->second ) )
				return false;
		}
		return true;
	}
};

//
//struct valArray : vArray, Refalbe
//{
//	REFABLE_IMP_MT;
//
//	typedef std::vector< xvalue_t > Impl_t;
//
//	Impl_t	_elements;
//	size_t	_size;
//
//	virtual bool 	set( size_t i, xv::Value & item )
//	{
//		if( _elements.size() == i ) 
//			return append( item );
//		_elements.resize( i + 1 );
//		_elements[i] = xvalue_t(item);
//		return true;
//	}
//	virtual bool 	get( size_t i, xv::Value & item )
//	{
//		if( i >= _elements.size() ) 
//			return false;
//		return ValueSet( &_elements[i], &item ) != FALSE;
//	}
//	virtual bool 	insert( size_t i, xv::Value& item )
//	{
//		size_t c = _elements.size();
//		if( i >= c )
//		{
//			_elements.push_back( item );
//		}
//		else
//		{
//			Impl_t::iterator it = _elements.begin() + i;
//			_elements.insert( it, item );
//		}
//		return true;
//	}
//	virtual bool 	erase( size_t i ) 
//	{
//		if( i >= _elements.size() ) 
//			return false;
//		Impl_t::iterator it = _elements.begin() + i;
//		_elements.erase(it);
//		return true;
//	}
//	virtual bool 	append( xv::Value & item )
//	{
//		size_t c = _elements.size();
//		_elements.push_back( item );
//		return true;
//	}
//	virtual size_t	length()
//	{
//		return _elements.size();
//	}
//	virtual bool	clear()
//	{
//		_elements.clear();
//		return true;
//	}
//	virtual bool clone( vComplex ** ppo )
//	{
//		refp<vArray> oarr = new valArray();
//		if( !oarr ) return false;
//		if( !cloneTo( oarr ) ) return false;
//		ppo[0] = oarr.detach();
//		return true;
//	}
//	virtual bool	cloneTo( vComplex * po )
//	{
//		refp<vArray> p = po;
//		if( !p ) return false;
//		if( !p->clear() ) return false;
//		for( size_t i = 0; i < _elements.size(); ++ i )
//		{
//			if( !p->append( _elements[i] ) ) 
//				return false;
//		}
//		return true;
//	}
//};

template < class CharType, int MINI_LEN = 12 >
struct _str_t 
{
protected:
	CharType			_mini_str[MINI_LEN];
	CharType *			_std_str;
	size_t				_length;
	const CharType *	_str_ptr;
public:
	_str_t() : _std_str(0) 
	{
		clear();
	}
	~_str_t()
	{
		clear();
	}
	bool assign( const CharType * pstr )
	{
		if( !clear() ) return false;
		if( !pstr ) return true;
		size_t len = 0;
		for( ;pstr[len];len++);
		if( !len ) return true;
		if( len < MINI_LEN ) 
		{
			memcpy( _mini_str, pstr, (len+1)*sizeof(CharType) );
		}
		else
		{
			_std_str = new CharType[len+1];
			if( !_std_str ) return false;
			memcpy( _std_str, pstr, (len+1)*sizeof(CharType) );
			_str_ptr = _std_str;
		}
		_length = len;
		return true;
	}
	const CharType * c_str() const
	{
		return _str_ptr;
	}
	size_t length()
	{
		return _length;
	}
	bool clear()
	{
		memset( _mini_str, 0, sizeof(_mini_str) );
		if( _std_str ) { ::free( _std_str ); _std_str = 0; }
		_str_ptr = _mini_str;
		_length = 0;
		return true;
	}
};

struct valStringA : vStringA, Refalbe
{
	REFABLE_IMP_MT;

	_str_t<char>	_str;

	virtual bool assign( const char * str ) 
	{ 
		return _str.assign( str );
	}
	virtual const char * c_str() 
	{ 
		return _str.c_str();
	}
	virtual size_t	length()
	{
		return _str.length()+ 1;
	}
	virtual bool clear()
	{
		return _str.clear();
	}
	virtual bool clone( vComplex ** ppo )
	{
		if( !ppo ) return false;
		refp<valStringA> ostr = new valStringA();
		if( !ostr ) return false;
		if( !cloneTo( ostr ) ) return false;
		ppo[0] = ostr.detach();
		return true;
	}
	virtual bool cloneTo( vComplex * po )
	{
		if( !po ) return false;
		refp<vStringA> ostr( po );
		return ostr->assign( c_str() );
	}
};

struct valStringW : vStringW, Refalbe
{
	REFABLE_IMP_MT;

	_str_t<wchar_t>	_str;

	virtual bool assign( const wchar_t * str ) 
	{ 
		return _str.assign( str );
	}
	virtual const wchar_t * c_str() 
	{ 
		return _str.c_str(); 
	}
	virtual size_t	length()
	{
		return _str.length() + 1;
	}
	virtual bool clear()
	{
		return _str.clear();
	}
	virtual bool clone( vComplex ** ppo )
	{
		if( !ppo ) return false;
		refp<valStringW> ostr = new valStringW();
		if( !ostr ) return false;
		if( !cloneTo( ostr ) ) return false;
		ppo[0] = ostr.detach();
		return true;
	}
	virtual bool cloneTo( vComplex * po )
	{
		if( !po ) return false;
		refp<vStringW> ostr( po );
		return ostr->assign( c_str() );
	}
};

struct valBuffer : vBuffer, Refalbe 
{
	REFABLE_IMP_MT;

	typedef std::vector<uint8_t> Impl_t;

	Impl_t	_buffer;

	virtual bool assign( const void * p, size_t cb ) 
	{
		if( !p ) return false;
		const uint8_t * ph = (const uint8_t*)p;
		_buffer.assign( ph, ph + cb );
		return true;
	}
	virtual uint8_t * base()
	{
		if( _buffer.empty() ) return 0;
		return &_buffer[0];
	}
	virtual bool resize( uint32_t size ) 
	{
		_buffer.resize(size);
		return true;
	}
	virtual size_t length()
	{
		return _buffer.size();
	}
	virtual bool clear()
	{
		_buffer.clear();
		return true;
	}
	virtual bool clone( vComplex ** ppo )
	{
		if( !ppo ) return false;
		refp<vBuffer> obuf = new valBuffer();
		if( !obuf ) return false;
		if( !cloneTo( obuf ) ) return false;
		ppo[0] = obuf.detach();
		return true;
	}
	virtual bool cloneTo( vComplex * po )
	{
		if( !po ) return false;
		refp<vBuffer> obuf( po );
		return obuf->assign( base(), length() );
	}
};

//////////////////////////////////////////////////////////////////////////

BOOL WINAPI ValueInit( xv::Value* pv )
{
	if( !pv ) return FALSE;
	pv->_type.type = dNull;
	pv->_type.bsiz = _8;
	pv->anyval = 0;
	return TRUE;
}
BOOL WINAPI ValueClear( xv::Value * pv )
{
	if( !pv ) return FALSE;
	switch( pv->_type.type )
	{
	case dNull:	case dInt: case dReal: break;
	case dLocalStr: case dUtf8Str:case dUniStr: case dArray: case dObject: case dBuffer:
		if( pv->complex ) pv->complex->_release();
		break;
	case dUnknown:
		if( pv->punk ) pv->punk->Release();
		break;
	}
	return ValueInit( pv );
}

BOOL WINAPI ValueSetType( xv::Value* pv, xv::value_type vt )
{
	if( !pv ) return FALSE;
	if( pv->_type.type == vt )
		return TRUE;

	if( !ValueClear(pv) ) 
		return FALSE;

	pv->_type.type = vt;
	
	BOOL fok = TRUE;
	switch( vt )
	{
	case dNull: pv->anyval = 0; break;
	case dInt:	pv->integer = 0; break;
	case dReal:	pv->real = 0;	break;
	case dLocalStr: case dUtf8Str:
		{
			refp<vComplex> o = new valStringA();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dUniStr:
		{
			refp<vComplex> o = new valStringW();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dArray:
		{
			refp<vComplex> o = new valArray();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dObject:
		{
			refp<vComplex> o = new valObject();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dBuffer:
		{
			refp<vComplex> o = new valBuffer();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dUnknown:
		{
			break;
		}
	}
	if( !fok ) ValueClear( pv );
	return fok;
}

BOOL WINAPI ValueSetType2( xv::Value* pv, xv::value_type vt, xv::value_size vs)
{
	if( !pv ) return FALSE;
	if( pv->_type.type == vt )
	{
		pv->_type.bsiz = vs;
		return TRUE;
	}

	if( !ValueClear(pv) ) 
		return FALSE;

	pv->_type.type = vt;
	pv->_type.bsiz = vs;

	BOOL fok = TRUE;
	switch( vt )
	{
	case dNull: pv->anyval = 0; break;
	case dInt:	pv->integer = 0; break;
	case dReal:	pv->real = 0;	break;
	case dLocalStr: case dUtf8Str:
		{
			refp<vComplex> o = new valStringA();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dUniStr:
		{
			refp<vComplex> o = new valStringW();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dArray:
		{
			refp<vComplex> o = new valArray();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dObject:
		{
			refp<vComplex> o = new valObject();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dBuffer:
		{
			refp<vComplex> o = new valBuffer();
			pv->complex = o.detach();
			if( !pv->complex ) fok = FALSE;
			break;
		}
	case dUnknown:
		{
			break;
		}
	}

	if( !fok ) ValueClear( pv );
	return fok;
}

BOOL WINAPI ValueClone( const xv::Value* vpIn, xv::Value* vpOut )
{
	if( !vpIn || !vpOut ) 
		return FALSE;

	if( !ValueClear( vpOut ) ) 
		return FALSE;

	if( !ValueSetType2( vpOut, (xv::value_type)(vpIn->_type.type), (xv::value_size)(vpIn->_type.bsiz) ) ) 
		return FALSE;

	BOOL fok = TRUE;
	switch( vpIn->_type.type )
	{
	case dNull:	vpOut->anyval = vpIn->anyval; break;
	case dInt:	vpOut->integer = vpIn->integer; break;
	case dReal:	vpOut->real = vpIn->real;		break;
	case dLocalStr:	case dUtf8Str:	case dUniStr:
	case dArray:	case dObject:	case dBuffer:
		if( vpIn->complex ) 
			fok = vpIn->complex->cloneTo( vpOut->complex );
		else
			fok = FALSE; 
		break;
	case dUnknown:
		if( vpIn->punk )
		{
			vpOut->punk = vpIn->punk;
			vpOut->punk->AddRef();
		}
	}
	if( !fok ) ValueClear( vpOut );
	return fok;
}
BOOL WINAPI ValueSet( const xv::Value* vpIn, xv::Value* vpOut )
{
	if( !vpIn || !vpOut ) 
		return FALSE;

	if( !ValueClear( vpOut ) ) 
		return FALSE;

	vpOut->_type = vpIn->_type;
	switch( vpIn->_type.type )
	{
	case dNull:	vpOut->anyval = vpIn->anyval; return TRUE;
	case dInt:	vpOut->integer = vpIn->integer; return TRUE;
	case dReal:	vpOut->real = vpIn->real;		return TRUE;
	case dLocalStr:case dUtf8Str:case dUniStr:
	case dArray:case dObject:case dBuffer:
		if( !vpIn->complex ) return FALSE;
		vpOut->complex = vpIn->complex;
		vpOut->complex->_reference();
		return TRUE;
	case dUnknown:
		if( vpIn->punk )
		{
			vpOut->punk = vpIn->punk;
			vpOut->punk->AddRef();
		}
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI ValueCheckType( const xv::Value * vp, xv::value_type vt )
{
	if( !vp ) return FALSE;
	return ( vp->_type.type == vt )?TRUE:FALSE; 
}
BOOL WINAPI ValueCheckType2( const xv::Value * vp, xv::value_type vt, xv::value_size vs )
{
	if( !vp ) return FALSE;
	if( vp->_type.type != vt ) return FALSE; 
	if( vp->_type.bsiz != vs ) return FALSE; 
	return TRUE;
}
BOOL WINAPI ValueCheckTypes( const xv::Value * vp, uint32_t mask )
{
	if( !vp ) return FALSE;
	uint32_t tm = 1<<(vp->_type.type);
	return (tm&mask)?TRUE:FALSE; 
}

//////////////////////////////////////////////////////////////////////////
//
//
//				Laod/Save
//
//
//////////////////////////////////////////////////////////////////////////

#include "xvreader.h"
#include "xvwritter.h"

BOOL WINAPI ValueLoad( xv::Value* vp, const void * bp, size_t cb )
{
	if( !vp || !bp || !cb ) return FALSE;
	xvalue_t tmp;
	if( !xv::reader(bp,cb).fetch( tmp ) ) return FALSE;
	return ValueSet( &tmp, vp );
}

BOOL WINAPI ValueSave( xv::Value* vp, Output * wo )
{
	if( !vp || !wo ) return FALSE;
	xvalue_t tmp( *vp );
	return xv::writter::write( tmp, wo );
}