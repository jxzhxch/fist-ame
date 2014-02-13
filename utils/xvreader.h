#ifndef __XVALUE_READER__
#define __XVALUE_READER__

namespace xv
{
	struct bytestream
	{
		const uint8_t * _dbase;
		const uint8_t * _dtail;
		const uint8_t * _dcurrent;

		size_t cursor()
		{
			return _dcurrent - _dbase;
		}

		bytestream( const void * data, size_t bytes )
		{
			_dbase = (const uint8_t *)data;
			_dcurrent = _dbase;
			_dtail = _dbase + bytes;
		}

		template< class T >
		bool fetch( T & t )
		{
			const uint8_t * rtail = _dcurrent + sizeof(t);
			if( rtail > _dtail )
				return false;
			t = *((T*)_dcurrent);
			_dcurrent = rtail;
			return true;
		}
		template< class T >
		bool peek( T & t )
		{
			const uint8_t * rtail = _dcurrent + sizeof(t);
			if( rtail > _dtail )
				return false;
			t = *((T*)_dcurrent);
			return true;
		}
		template < class T >
		T *  fetch_ptr( size_t count )
		{
			size_t bytes = sizeof(T) * count;
			if( bytes > rest() ) return NULL;
			T * ret = (T*)_dcurrent;
			_dcurrent += bytes;
			return ret;
		}
		template < class T >
		bool fetch_to( T * buffer, size_t count )
		{
			size_t bytes = sizeof(T) * count;
			const uint8_t * rtail = _dcurrent + bytes;
			if( rtail > _dtail )
				return false;
			memcpy( buffer, _dcurrent, bytes );
			_dcurrent = rtail;
			return true;
		}
		bool peek_to( uint8_t * buffer, size_t bytes )
		{
			const uint8_t * rtail = _dcurrent + bytes;
			if( rtail > _dtail )
				return false;
			memcpy( buffer, _dcurrent, bytes );
			return true;
		}

		size_t rest()
		{
			return _dtail - _dcurrent;
		}

		bool test( size_t bytes )
		{
			if( bytes > rest() )
				return false;
			return true;
		}

		bool skip( size_t bytes )
		{
			const uint8_t * rtail = _dcurrent + bytes;
			if( rtail > _dtail )
				return false;
			_dcurrent = rtail;
			return true;
		}

		template < class T >
		const T * current_as()
		{
			return (const T*)_dcurrent;
		}

	};

	struct reader
	{
		bytestream	_stm;

		reader( const void * bp, size_t cb )
			: _stm( (const uint8_t *)bp, cb )
		{

		}

		//template < uint8_t Type >
		//bool fetch( xvalue_t& ret )
		//{
		//	type_byte _typ = { dNull, _8 };
		//	if( !_stm.fetch( _typ ) ) return false;
		//	if( _typ.type != Type ) return false;

		//	switch( _typ.type )
		//	{
		//	case dNull: ValueClear(ret);
		//		return true;
		//	case dInt:
		//		return fetch_number( _typ, ret );
		//	case dReal:
		//		return fetch_number( _typ, ret );
		//	case dLocalStr: case dUtf8Str:
		//		return fetch_str<char>( _typ, ret );
		//	case dUniStr:
		//		return fetch_str<wchar_t>( _typ, ret );
		//	case dBuffer:
		//		return fetch_buf( _typ, ret );
		//	case dArray:
		//		return fetch_array( _typ, ret );
		//	case dDict:
		//		return fetch_dict( _typ, ret );
		//	}

		//	return false;
		//}


		bool fetch( xvalue_t & ret )
		{
			type_byte vt = { dNull, _8 };
			if( !_stm.fetch( vt ) ) return false;
			switch( vt.type )
			{
			case dNull:		return ret.set(nothing);
			case dInt:		return fetch_number( vt, ret );
			case dReal:		return fetch_number<double>( ret );
			case dLocalStr: case dUtf8Str:
							return fetch_str<char>( vt, ret );
			case dUniStr:	return fetch_str<wchar_t>( vt, ret );
			case dBuffer:	return fetch_buf( vt, ret );
			case dArray:	return fetch_array( vt, ret );
			case dObject:	return fetch_dict( vt, ret );
			}
			return false;
		}

		bool fetch_dict( type_byte typ, xvalue_t & ret )
		{
			uint64_t pairs = 0;
			if( !fetch_x( typ, pairs ) ) return false;
			//check//if( pairs & 0xFFFF0000 ) return false;
			refp<vObject> oobj = ret.asObject(true);
			if( !oobj ) return false;
			if( !oobj->clear() ) return false;
			for( size_t i = 0; i < pairs; ++ i )
			{
				const char * key = fetch_key();
				if( !key ) return false;

				xvalue_t _tmp;
				if( !fetch( _tmp ) ) return false;

				if( !oobj->set( key, _tmp ) ) return false;
			}
			return true;
		}

		bool fetch_array( type_byte typ, xvalue_t & ret )
		{
			uint64_t elmts = 0;
			if( !fetch_x( typ, elmts ) ) return false;
			//check//if( elmts > 0xFFFF ) return false;
			refp<vArray> oarry = ret.asArray(true);
			if( !oarry ) return false;
			if( !oarry->clear() ) return false;
			for( size_t i = 0; i < elmts; ++ i )
			{
				size_t idx = 0;
				if( !fetch_leb128(idx) ) return false;
				xvalue_t elmt;
				if( !fetch( elmt ) ) return false;
				if( !oarry->set(idx,elmt) ) return false;
			}
			return true;
		}

		template < typename numType >
		bool fetch_number( xvalue_t & ret )
		{
			numType vnum = 0;
			if( !_stm.fetch( vnum ) ) return false;
			return ret.set(vnum);
		}
		bool fetch_number( type_byte typ, xvalue_t & ret )
		{
			switch( typ.bsiz )
			{
			case _8:	return fetch_number<int8_t>(ret);
			case _16:	return fetch_number<int16_t>(ret);
			case _32:	return fetch_number<int32_t>(ret);
			case _64:	return fetch_number<int64_t>(ret);
			};
			return false;
		}

		bool fetch_x( type_byte typ, uint64_t & cc64 )
		{
			switch( typ.bsiz )
			{
			case _8:	{ uint8_t cc = 0;	if( !_stm.fetch( cc ) ) return false; cc64 = cc; };	break;
			case _16:	{ uint16_t cc = 0;	if( !_stm.fetch( cc ) ) return false; cc64 = cc; };	break;
			case _32:	{ uint32_t cc = 0;	if( !_stm.fetch( cc ) ) return false; cc64 = cc; };	break;
			case _64:	{ uint64_t cc = 0;	if( !_stm.fetch( cc ) ) return false; cc64 = cc; };	break;
			default:	return false;
			};
			return true;
		}

		/*
		LONG i = 0;
		UINT8 b = 0;
		for( ; (i<5) && (0==(b&0x80)); ++ i )
		{
			RFAILED( pstm->Read( &b, 1, NULL ) );
			UINT32 v = b & 0x7F;
			rVal |= (v << (i*7));
		}
		return i;
		*/

		// LEB128
		size_t fetch_leb128( size_t & k, size_t rmax = 5 )
		{
			size_t i = 0;
			UINT8 b = 0;
			for( ; (i<rmax) && ( 0 == ( b & 0x80) ); ++ i )
			{
				if( !_stm.fetch(b) ) return 0;
				size_t v = b & 0x7F;
				k |= (v << (i*7));
			}
			return i;
		}

		const char * fetch_key( )
		{
			uint8_t cch = 0;
			if( !_stm.fetch(cch) ) return 0;
			if( !cch ) return 0;
			const char * data = _stm.fetch_ptr<char>( cch );
			if( !data ) return 0;
			if( data[cch-1] ) return 0; // must be \0
			return data;
		}

		template < class charType >
		bool fetch_str( type_byte typ, xvalue_t & ret )
		{
			uint64_t _cch = 0;
			if( !fetch_x( typ, _cch ) ) return false;
			//check//if( cch & 0xFFFF0000 ) return false
			size_t cch = (size_t)_cch;
			const charType * data = _stm.fetch_ptr<charType>( cch );
			if( !data ) return false;
			if( data[cch-1] ) return false; // must be \0
			return ret.set( data );
		}
		bool fetch_buf( type_byte typ, xvalue_t & ret )
		{
			uint64_t _ccb = 0;
			if( !fetch_x( typ, _ccb ) ) return false;
			size_t ccb = (size_t)_ccb;
			if( !_stm.test( ccb) ) return false;
			const uint8_t * data = _stm.fetch_ptr<uint8_t>( ccb );
			RX_BLOB blob(data,ccb);
			return ret.set( blob );
		}
	};
};


#endif
