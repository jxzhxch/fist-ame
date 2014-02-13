#ifndef __XVALUE_HELPER__
#define __XVALUE_HELPER__

#include "../include/xvalue.h"
#include <vector>
//////////////////////////////////////////////////////////////////////////
//
//
//				Helper Class
//
//
//////////////////////////////////////////////////////////////////////////

namespace xv
{
	typedef const char *	acp_str_t;
	typedef const char *	utf8_str_t;


	struct _null
	{};

	static _null nothing;

	enum exception_t
	{
		ExcepValueInit,
		ExcepTypeConvert,
		ExcepInvalidParameter,
	};

#define _xvcall_		

	struct xvalue_t : Value
	{
		_xvcall_ ~xvalue_t()
		{
			ValueClear(this);
		}
		_xvcall_ xvalue_t( xv::value_type typ )
		{
			if( !ValueSetType( this, typ) )
				throw ExcepValueInit;
		}
		_xvcall_ xvalue_t()
		{
			ValueInit( this );
		}
		_xvcall_ xvalue_t( const Value & r )
		{
			ValueInit( this );
			if( !set( r ) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( const xvalue_t & r )
		{
			ValueInit( this );
			if( !set( r ) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( int64_t i ) 
		{
			ValueInit( this );
			if( !set(i) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( int32_t i ) 
		{
			ValueInit( this );
			if( !set(i) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( short i ) 
		{
			ValueInit( this );
			if( !set(i) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( char i ) 
		{
			ValueInit( this );
			if( !set(i) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( double i )
		{
			ValueInit( this );
			if( !set(i) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( const wchar_t * ustr )
		{
			ValueInit( this );
			if( !set(ustr) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( acp_str_t lstr )
		{
			ValueInit( this );
			if( !set(lstr) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( const void * data, size_t len )
		{
			ValueInit( this );
			RX_BLOB blob(data,len);
			if( !set(blob)) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( const chunk_t & block )
		{
			ValueInit( this );
			if( !set(block) ) throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( const _null )
		{
			ValueInit( this );
		}
		_xvcall_ xvalue_t( vObject* pobj )
		{
			if( !set(pobj) )
				throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( vArray* parr )
		{
			if( !set(parr) )
				throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( vStringA* psa )
		{
			if( !set(psa) )
				throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( vStringW* psw )
		{
			if( !set(psw) )
				throw ExcepValueInit;
		}
		_xvcall_ xvalue_t( vBuffer* pbuf )
		{
			if( !set(pbuf) )
				throw ExcepValueInit;
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool changeType( value_type typ, value_size vs = _8 )
		{
			return (ValueSetType2( this, typ, vs ) != FALSE);
		}
		_xvcall_ bool checkType( value_type typ )
		{
			return (ValueCheckType( this, typ )  != FALSE);
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ xv::xvalue_t operator () ( size_t index, xv::xvalue_t def )
		{
			xv::xvalue_t tmp;
			if( !get( index, tmp ) )
				return def;
			return tmp;
		}
		_xvcall_ xv::xvalue_t operator () ( const char* name, xv::xvalue_t def )
		{
			xv::xvalue_t tmp;
			if( !get( name, tmp ) )
				return def;
			return tmp;
		}
		_xvcall_ xv::xvalue_t& operator [] ( size_t index )
		{
			return ref( index );
		}
		_xvcall_ xv::xvalue_t& operator [] ( const char* name )
		{
			return ref( name );
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool get( int8_t & i ) const
		{
			if( !ValueCheckType( this, dInt ) ) 
				return false;
			i = (int8_t)integer;
			return true;
		}
		_xvcall_ bool get( int16_t & i ) const
		{
			if( !ValueCheckType( this, dInt ) ) 
				return false;
			i = (int16_t)integer;
			return true;
		}
		_xvcall_ bool get( int32_t & i ) const
		{
			if( !ValueCheckType( this, dInt ) ) 
				return false;
			i = (int32_t)integer;
			return true;
		}
		_xvcall_ bool get( int64_t & i ) const
		{
			if( !ValueCheckType( this, dInt ) ) 
				return false;
			i = (int64_t)integer;
			return true;
		}
		_xvcall_ bool get( double & i ) const
		{
			if( ValueCheckType( this, dReal ) )
			{
				i = real;
			}
			else if( ValueCheckType( this, dInt ) ) 
			{
				i = (double)integer;
			}
			else
			{
				return false;
			}
			return true;
		}
		_xvcall_ bool get( chunk_t & block ) const 
		{
			if( !ValueCheckType( this, dBuffer ) ) 
				return false;
			block.Data = ((vBuffer*)complex)->base();
			block.Length = ((vBuffer*)complex)->length();
			block.FreePtr = 0;
			return true;
		}
		_xvcall_ bool get( const wchar_t * & p ) const
		{
			if( !ValueCheckType( this, dUniStr ) ) 
				return false;
			p = ((vStringW*)complex)->c_str();
			return true;
		}
		_xvcall_ bool get( acp_str_t & mbstr ) const
		{
			if( !ValueCheckType( this, dLocalStr ) ) 
				return false;
			refp<vStringA> ostr( complex );
			mbstr = ostr->c_str();
			return true;
		}
		_xvcall_ bool get( wchar_t * & p ) const
		{
			if( !ValueCheckType( this, dUniStr ) ) 
				return false;
			p = (wchar_t*)((vStringW*)complex)->c_str();
			return true;
		}
		_xvcall_ bool get( IUnknown * & p ) const
		{
			if( !ValueCheckType( this, dUnknown ) ) 
				return false;
			p = punk;
			return true;
		}
		_xvcall_ bool get( char * & mbstr ) const
		{
			if( !ValueCheckType( this, dLocalStr ) &&
				!ValueCheckType( this, dUtf8Str ) )
				return false;
			refp<vStringA> ostr( complex );
			mbstr = (char*)ostr->c_str();
			return true;
		}
		_xvcall_ bool get( xvalue_t & rval ) const
		{
			return ValueSet( this, &rval ) != FALSE;
		}
		_xvcall_ bool get( Value & rval ) const
		{
			return ValueSet( this, &rval ) != FALSE;
		}
		template < class T >
		_xvcall_ bool get( const char * name, T & elmt ) const
		{
			if( !ValueCheckType( this, dObject ) ) 
				return false;
			refp<vObject> obj = complex;
			xvalue_t xo;
			if( !obj->get( name, xo ) )
				return false;
			return xo.get( elmt );
		}
		template < class T >
		_xvcall_ bool get( size_t index, T & elmt ) const
		{
			if( !ValueCheckType( this, dArray ) ) 
				return false;
			refp<vArray> arry = complex;
			xvalue_t xo;
			if( !arry->get( index, xo ) )
				return false;
			return xo.get( elmt );
		}

		_xvcall_ xv::xvalue_t& ref( const char * name ) const
		{
			if( !ValueCheckType( this, dObject ) ) 
				throw ExcepTypeConvert;
			refp<vObject> obj = complex;
			return (xv::xvalue_t&)obj->ref( name );
		}
		_xvcall_ xv::xvalue_t& ref( size_t index ) const
		{
			if( !ValueCheckType( this, dArray ) ) 
				throw ExcepTypeConvert;
			refp<vArray> arry = complex;
			return (xv::xvalue_t&)arry->ref( index );
		}

		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool set( const _null )
		{
			return ValueSetType( this, dNull ) != FALSE;
		}
		_xvcall_ bool set( int8_t i )
		{
			if( !ValueSetType2( this, dInt, _8 ) ) 
				return false;
			integer = i;
			return true;
		}
		_xvcall_ bool set( int16_t i )
		{
			if( !ValueSetType2( this, dInt, _16 ) )  
				return false;
			i = (int16_t)integer;
			return true;
		}
		_xvcall_ bool set( int32_t i )
		{
			if( !ValueSetType2( this, dInt, _32 ) ) 
				return false;
			integer = i;
			return true;
		}
		_xvcall_ bool set( int64_t i )
		{
			if( !ValueSetType2( this, dInt, _64 ) ) 
				return false;
			integer = i;
			return true;
		}

		_xvcall_ bool set( uint8_t i )
		{
			if( !ValueSetType2( this, dInt, _8 ) ) 
				return false;
			integer = i;
			return true;
		}
		_xvcall_ bool set( uint16_t i )
		{
			if( !ValueSetType2( this, dInt, _16 ) )  
				return false;
			i = (int16_t)integer;
			return true;
		}
		_xvcall_ bool set( uint32_t i )
		{
			if( !ValueSetType2( this, dInt, _32 ) ) 
				return false;
			integer = i;
			return true;
		}
		_xvcall_ bool set( uint64_t i )
		{
			if( !ValueSetType2( this, dInt, _64 ) ) 
				return false;
			integer = i;
			return true;
		}

		_xvcall_ bool set( double i )
		{
			if( !ValueSetType( this, dReal ) ) 
				return false;
			real = i;
			return true;
		}
		_xvcall_ bool set( const wchar_t * p )
		{
			if( !ValueSetType( this, dUniStr ) ) 
				return false;
			refp<vStringW> uni(complex);
			return uni->assign( p );
		}
		_xvcall_ bool set( acp_str_t lstr )
		{
			if( !ValueSetType( this, dLocalStr ) ) 
				return false;
			refp<vStringA> mbs(complex);
			return mbs->assign( lstr );
		}
		_xvcall_ bool set( const chunk_t & block )
		{
			if( !ValueSetType( this, dBuffer ) ) 
				return false;
			refp<vBuffer> buf(complex);
			return buf->assign( block.Data, block.Length );
		}
		template< class T >
		_xvcall_ bool set( std::vector<T>& vec )
		{
			if( !ValueSetType( this, dBuffer ) ) 
				return false;
			refp<vBuffer> buf(complex);
			return buf->assign( &vec[0], vec.size()*sizeof(T) );
		}
		_xvcall_ bool set( const Value & val ) 
		{
			return ValueSet( &val, this ) != FALSE;
		}
		_xvcall_ bool set( const xvalue_t & val )
		{
			return ValueSet( &val, this ) != FALSE;
		}
		_xvcall_ bool set( IUnknown* p )
		{
			if( !ValueSetType( this, dUnknown ) ) 
				return false;
			punk = p;
			if( p ) p->AddRef();
			return S_OK;
		}
		template < class T >
		_xvcall_ bool set( const char * name, T elmt )
		{
			if( !ValueSetType( this, dObject ) ) 
				return false;
			refp<vObject> obj = complex;
			xvalue_t xo;
			if( !xo.set( elmt ) ) 
				return false;
			return obj->set( name, xo );
		}
		template < class T >
		_xvcall_ bool set( size_t index, T elmt )
		{
			if( !ValueSetType( this, dArray ) ) 
				return false;
			refp<vArray> arry = complex;
			xvalue_t xo;
			if( !xo.set( elmt ) ) 
				return false;
			return arry->set( index, xo );
		}
		_xvcall_ bool set( vObject* pobj )
		{
			if( !pobj ) return false;
			if( !ValueSetType( this, dObject ) ) 
				return false;
			complex = pobj;
			complex->_reference();
			return true;
		}
		_xvcall_ bool set( vArray* parr )
		{
			if( !parr ) return false;
			if( !ValueSetType( this, dArray ) ) 
				return false;
			complex = parr;
			complex->_reference();
			return true;
		}
		_xvcall_ bool set( vStringA* psa )
		{
			if( !psa ) return false;
			if( !ValueSetType( this, dLocalStr ) ) 
				return false;
			complex = psa;
			complex->_reference();
			return true;
		}
		_xvcall_ bool set( vStringW* psw )
		{
			if( !psw ) return false;
			if( !ValueSetType( this, dUniStr ) ) 
				return false;
			complex = psw;
			complex->_reference();
			return true;
		}
		_xvcall_ bool set( vBuffer* pbuf )
		{
			if( !pbuf ) return false;
			if( !ValueSetType( this, dBuffer ) ) 
				return false;
			complex = pbuf;
			complex->_reference();
			return true;
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ refp<vObject> asObject( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dObject ) )
					return (vObject*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dObject ) )
					return (vObject*)NULL;
			}
			return complex;
		}
		_xvcall_ refp<vArray> asArray( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dArray ) )
					return (vArray*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dArray ) )
					return (vArray*)NULL;
			}
			return complex;
		}
		_xvcall_ refp<vStringA> asStringACP( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dLocalStr ) )
					return (vStringA*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dLocalStr ) )
					return (vStringA*)NULL;
			}
			return complex;
		}
		_xvcall_ refp<vStringA> asStringUTF8( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dUtf8Str ) )
					return (vStringA*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dUtf8Str ) )
					return (vStringA*)NULL;
			}
			return complex;
		}
		_xvcall_ refp<vStringW> asStringW( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dUniStr ) )
					return (vStringW*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dUniStr ) )
					return (vStringW*)NULL;
			}
			return complex;
		}
		_xvcall_ refp<vBuffer> asBuffer( bool force = true )
		{
			if( force )
			{
				if( !ValueSetType( this, dBuffer ) )
					return (vBuffer*)NULL;
			}
			else
			{
				if( !ValueCheckType( this, dBuffer ) )
					return (vBuffer*)NULL;
			}
			return complex;
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ xvalue_t & operator = ( const Value & r ) 
		{
			if( set( r ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( const xvalue_t & r ) 
		{
			if( set( r ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( const _null )
		{
			if( set(nothing) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( int8_t i8 ) 
		{
			if( set( i8 ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( int16_t i16 ) 
		{
			if( set( i16 ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( int32_t i32 ) 
		{
			if( set( i32 ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( int64_t i64 ) 
		{
			if( set( i64 ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( double d64 ) 
		{
			if( set( d64 ) ) return *this;
			throw 0;
		}
		_xvcall_ xvalue_t & operator = ( const char * lstr ) 
		{
			if( set( lstr ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( const wchar_t * ustr ) 
		{
			if( set( ustr ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( const chunk_t & block ) 
		{
			if( set( block ) ) return *this;
			throw ExcepValueInit;
		}
		_xvcall_ xvalue_t & operator = ( IUnknown * p )
		{
			if( !set(p) )
				throw ExcepValueInit;
			return *this;
		}
		//////////////////////////////////////////////////////////////////////////

		_xvcall_ operator int8_t () const 
		{
			if( !ValueCheckType( this, dInt ) )
				throw ExcepTypeConvert;
			return (int8_t)integer;
		}
		_xvcall_ operator int16_t () const 
		{
			if( !ValueCheckType( this, dInt ) )
				throw ExcepTypeConvert;
			return (int16_t)integer;
		}
		_xvcall_ operator int32_t () const 
		{
			if( !ValueCheckType( this, dInt ) )
				throw ExcepTypeConvert;
			return (int32_t)integer;
		}
		_xvcall_ operator int64_t () const 
		{
			if( !ValueCheckType( this, dInt ) )
				throw ExcepTypeConvert;
			return (int64_t)integer;
		}
		_xvcall_ operator double () const 
		{
			if( !ValueCheckType( this, dReal ) )
				throw ExcepTypeConvert;
			return real;
		}
		_xvcall_ operator const char * () const 
		{
			const char * ret;
			if( !get( ret ) )
				throw ExcepTypeConvert;
			return ret;
		}
		_xvcall_ operator const wchar_t * () const 
		{
			const wchar_t * ret;
			if( !get( ret ) )
				throw ExcepTypeConvert;
			return ret;
		}
		_xvcall_ operator IUnknown * () const 
		{
			IUnknown * ret;
			if( !get( ret ) )
				throw ExcepTypeConvert;
			return ret;
		}
		_xvcall_ operator bool () const 
		{
			if( _type.type == dNull ) 
				return false;
			if( _type.type == dInt )
				return integer != 0;
			if( _type.type == dReal )
				return real != 0;
			if( _type.type == dLocalStr || _type.type == dUtf8Str || _type.type == dUniStr ||
				_type.type == dArray || _type.type == dObject )
			{
				if( !complex ) return false;
				return complex->length() != 0;
			}
			if( _type.type == dUnknown )
				return punk != 0;
			return false;
		}
		_xvcall_ operator Value& ()
		{
			return *this;
		}
		_xvcall_ operator const Value& () const
		{
			return *this;
		}

		_xvcall_ xvalue_t operator () ( size_t index ) const 
		{
			xvalue_t r;
			get( index, r );
			return r;
		}
		//_xvcall_ xvalue_t & operator () ( size_t index, const xvalue_t v ) 
		//{
		//	if( !set( index, v ) ) throw 0;
		//	return *this;
		//}
		_xvcall_ xvalue_t operator () ( const char * name ) const
		{
			xvalue_t r;
			get( name, r );
			return r;
		}
		//_xvcall_ xvalue_t & operator () ( const char * name, const xvalue_t v ) 
		//{
		//	if( !set( name, v ) ) throw 0;
		//	return *this;
		//}

		_xvcall_ bool operator == ( const _null ) const 
		{
			return (_type.type == dNull);
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool load( const void * bp, size_t cb )
		{
			if( !ValueClear( this ) ) return false;
			return ValueLoad( this, bp, cb )!=FALSE;
		}
		_xvcall_ bool load( const RX_BLOB & blob )
		{
			if( !ValueClear( this ) ) return false;
			return ValueLoad( this, blob.Data, blob.Length )!=FALSE;
		}
		struct binstm : Output
		{
			std::vector< uint8_t > _buffer;
			_xvcall_ bool write( const void * data, size_t bytes )
			{
				const uint8_t * tmp = (const uint8_t *)data;
				_buffer.insert( _buffer.end(), tmp, tmp + bytes );
				return true;
			}
			_xvcall_ const uint8_t * base()
			{
				if( _buffer.empty() ) return 0;
				return &_buffer[0];
			}
			_xvcall_ size_t bytes()
			{
				return _buffer.size();
			}
			_xvcall_ void clear()
			{
				_buffer.clear();
			}
		};

		_xvcall_ bool save( Output & bs )
		{
			return ValueSave( this, &bs )!=FALSE;
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool erase( const char * name )
		{
			refp<vObject> obj = asObject(false);
			if( !obj ) return false;
			return obj->erase( name );
		}
		_xvcall_ bool erase( size_t idx )
		{
			refp<vArray> ary = asArray(false);
			if( !ary ) return false;
			return ary->erase( idx );
		}
		//////////////////////////////////////////////////////////////////////////
		_xvcall_ bool clear()
		{
			return ValueClear( this )!=FALSE;
		}
		_xvcall_ bool isInt()
		{
			return (_type.type == dInt);
		}
		_xvcall_ bool isMBS()
		{
			return (_type.type == dLocalStr) || (_type.type == dUtf8Str);
		}
		_xvcall_ bool isWCS()
		{
			return (_type.type == dUniStr) ;
		}
		_xvcall_ bool isObject()
		{
			return (_type.type == dObject) ;
		}
		_xvcall_ bool isArray()
		{
			return (_type.type == dArray) ;
		}
		_xvcall_ bool isUnknownPtr()
		{
			return (_type.type == dUnknown) ;
		}
		_xvcall_ bool isReal()
		{
			return (_type.type == dReal) ;
		}
	};

};

#endif