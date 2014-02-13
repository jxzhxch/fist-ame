#ifndef __SDX_WRITTER__
#define __SDX_WRITTER__

namespace xv
{
	namespace writter
	{
		bool write( xvalue_t & v, Output * o );

		template < typename dataType >
		bool write_t( dataType v, Output * o )
		{
			return o->write( &v, sizeof(v) );
		};

		bool write_type( type_byte type, Output * o )
		{
			return o->write( &type, sizeof(type) );
		}

		bool write_null( xvalue_t & v, Output * o )
		{
			return true;
		}

		size_t write_leb128( size_t v, Output * o )
		{
			size_t i = 0;
			do {
				UINT8 b = v & 0x7F; v >>= 7;
				if( !v ) b |= 0x80;
				if( !o->write( &b, 1) ) return 0;
				++ i;
			} while( v );
			return i;
		}

		bool write_int( xvalue_t & v, Output * o )
		{
			if( !write_t( v._type, o ) ) return false;
			switch( v._type.bsiz )
			{
			case _8:	return write_t<int8_t> ( (int8_t)v.integer, o );
			case _16:	return write_t<int16_t>( (int16_t)v.integer, o );
			case _32:	return write_t<int32_t>( (int32_t)v.integer, o );
			case _64:	return write_t<int64_t>( (int64_t)v.integer, o );
			}
			return false;
		};
		bool write_real( xvalue_t & v, Output * o )
		{
			if( !write_t( v._type, o ) ) return false;
			return write_t<double> ( v.real, o );
		};

		bool write_lstr( xvalue_t& v, Output * o )
		{
			refp<vStringA> str = v.asStringACP(false);
			if( !str ) return false;
			size_t cch = str->length();
			if( !cch ) return false;
			bool fok = true;
			if( cch < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)cch, o ) ) return false;
			}
			else if( cch < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)cch, o ) ) return false;
			}
			else
				return false;		// too long

			// write string , include \0
			return o->write( str->c_str(), cch );
		};

		bool write_utf8( xvalue_t& v, Output * o )
		{
			refp<vStringA> str = v.asStringUTF8(false);
			if( !str ) return false;
			size_t cch = str->length();
			if( !cch ) return false;
			bool fok = true;
			if( cch < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)cch, o ) ) return false;
			}
			else if( cch < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)cch, o ) ) return false;
			}
			else
				return false;		// too long

			// write string , include \0
			return o->write( str->c_str(), cch );
		};
		bool write_unicode( xvalue_t& v, Output * o )
		{
			refp<vStringW> str = v.asStringW(false);
			if( !str ) return false;
			size_t cch = str->length();
			if( !cch ) return false;
			bool fok = true;
			if( cch < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)cch, o ) ) return false;
			}
			else if( cch < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)cch, o ) ) return false;
			}
			else
				return false;		// too long

			// write string , include \0
			return o->write( str->c_str(), cch*sizeof(wchar_t) );
		};

		bool write_buffer( xvalue_t& v, Output * o )
		{
			refp<vBuffer> buf = v.asBuffer(false);
			if( !buf ) return false;
			size_t cb = buf->length();
			if( cb < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)cb, o ) ) return false;
			}
			else if( cb < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)cb, o ) ) return false;
			}
			else
			{
				v._type.bsiz = _32;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint32_t)cb, o ) ) return false;
			}
			return o->write( buf->base(), cb );
		};

		bool write_array( xvalue_t& v, Output * o )
		{
			refp<vArray> arr = v.asArray(false);
			if( !arr ) return false;
			size_t ce = arr->length();
			if( ce < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)ce, o ) ) return false;
			}
			else if( ce < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)ce, o ) ) return false;
			}
			else return false;		// too many elements

			struct dumper
			{
				static bool dump_pair( size_t idx, xv::Value & item, bool last, void * p )
				{
					if( !p ) return false;
					Output * o = (Output*)p;
					if( !o ) return false;
					// write key
					if( !write_leb128(idx,o) ) return false;
					// write value
					xvalue_t tmp(item);
					return write( tmp, o );
				}
			};
			return arr->enum_it( dumper::dump_pair, o );
		};

		bool write_dict( xvalue_t& v, Output * o )
		{
			refp<vObject> obj = v.asObject(false);
			if( !obj ) return false;
			size_t cf = obj->length();
			if( cf < 0x100 )
			{
				v._type.bsiz = _8;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint8_t)cf, o ) ) return false;
			}
			else if( cf < 0x10000 )
			{
				v._type.bsiz = _16;
				if( !write_t( v._type, o ) ) return false;
				if( !write_t( (uint16_t)cf, o ) ) return false;
			}
			else return false;		// too many fields

			struct dumper
			{
				static bool dump_pair( const char * name, xv::Value & item, bool last, void * p )
				{
					if( !name || !p ) return false;
					Output * o = (Output*)p;
					if( !o ) return false;
					// write key
					size_t l = strlen(name) + 1;
					if( l > 255 ) return false;
					uint8_t kl = (uint8_t)l;
					if( !write_t(kl,o) ) return false;
					if( !o->write( name, kl ) ) return false;
					// write value
					xvalue_t tmp(item);
					return write( tmp, o );
				}
			};
			return obj->enum_it( dumper::dump_pair, o );
		};
		bool write( xvalue_t & v, Output * o )
		{
			switch( v._type.type )
			{
			case dNull:		return write_null( v, o );
			case dInt:		return write_int( v, o );
			case dReal:		return write_real( v, o );
			case dLocalStr:	return write_lstr( v, o );
			case dUtf8Str:	return write_utf8( v, o );
			case dUniStr:	return write_unicode( v, o );
			case dArray:	return write_array( v, o );
			case dObject:	return write_dict( v, o );
			case dBuffer:	return write_buffer( v, o );
			}
			return false;
		};
	};
};

#endif
