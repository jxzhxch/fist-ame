#ifndef __VIN_KOBASE__
#define __VIN_KOBASE__

#include "kerror.h"

namespace vin
{
	// register all kernel-object types here
	enum _KoClass
	{
		TKoSymbolLink,		// KoSymbolLink
		TKoProcess,
		TKoThread,
		TKoMutex,
		TKoEvent,
		TKoSection,
		TKoWkmImage,
		TKoRegistry,
		TKoDevice,
		TKoPipe,
		TKoMailSlot,
		TKoFile,
		TKoVolume,
		TKoVirtFS,
	};

	typedef size_t	KoClass;

	struct KoBase
	{
		size_t access;

		KoBase() : access(0) { };

		virtual ~KoBase() { };

		virtual KoClass main_class() = 0;

		virtual void* as_class( KoClass clsid ) = 0 ;

		virtual uint32_t get_access() { return access; };

		virtual void set_access( size_t n ) { access = n; };

		template < class T >
		T* classptr() { return (T*)as_class(T::ObId); };
	};

	template < KoClass cls >
	struct TKoBase : KoBase
	{
		enum { ObId = cls };
		virtual KoClass main_class() { return ObId; };
		virtual void* as_class( KoClass clsid ) { return ( clsid == ObId ) ? this : 0; }
	};


};


#endif