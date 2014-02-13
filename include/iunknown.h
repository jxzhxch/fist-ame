#ifndef RE_EXPORTS_UNKNOWN_H_
#define RE_EXPORTS_UNKNOWN_H_

#if (TARGET_OS == OS_WINDOWS && CPP_COMPILER==CC_MSVC)
	#define USE_WINDOWS_UNKNOWN_AND_IID
#endif

#ifdef USE_WINDOWS_UNKNOWN_AND_IID

	#define re_uuidof(iface)	__uuidof(iface)

	#define	RE_DECLARE_IID

	#define	RE_DEFINE_IID(iface, uuid_string, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)	class __declspec(uuid(uuid_string)) iface;

#else

    #ifndef DECLSPEC_SELECTANY
        #define DECLSPEC_SELECTANY
    #endif

	#define re_uuidof(iface)	(iface::iid)

	#define	RE_DECLARE_IID		static const GUID iid

	#ifdef INITGUID
		#define RE_DEFINE_IID(iface, uuid_string, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)	\
			const GUID DECLSPEC_SELECTANY iface::iid = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } };
	#else
		#define RE_DEFINE_IID(iface, uuid_string, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
	#endif // INITGUID

    #undef DEFINE_GUID
	#ifdef INITGUID
		#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
			EXTERN_C const GUID DECLSPEC_SELECTANY name \
				= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
		#else
			#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
			EXTERN_C const GUID name
	#endif

	#if (TARGET_OS != OS_WINDOWS)

		class IUnknown
		{
		public:
			STDMETHOD(QueryInterface)(REFGUID riid, void **ppv) PURE;
			STDMETHOD_(ULONG, AddRef)() PURE;
			STDMETHOD_(ULONG, Release)() PURE;
		public:
			RE_DECLARE_IID;
		};
		RE_DEFINE_IID(IUnknown, "{00000000-0000-0000-C000-000000000046}",
			  0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

		class IClassFactory : public IUnknown
		{
		public:
			virtual HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown *pUnkOuter, REFIID riid, void **ppvObject) = 0;
			virtual HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock) = 0;
		public:
			RE_DECLARE_IID;
		};

		RE_DEFINE_IID(IClassFactory, "{00000001-0000-0000-C000-000000000046}",
			0x00000001, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

	#else

		// Windows & GCC, Rename IUnknwon to IUnknown__

		class IUnknown__ : public IUnknown
		{
		public:
			RE_DECLARE_IID;
		};
		RE_DEFINE_IID(IUnknown__, "{00000000-0000-0000-C000-000000000046}",
			0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

		class IClassFactory__ : public IUnknown
		{
		public:
			virtual HRESULT STDMETHODCALLTYPE CreateInstance( IUnknown__ *pUnkOuter, REFIID riid, void **ppvObject) = 0;
			virtual HRESULT STDMETHODCALLTYPE LockServer( BOOL fLock) = 0;
		public:
			RE_DECLARE_IID;
		};

		RE_DEFINE_IID(IClassFactory__, "{00000001-0000-0000-C000-000000000046}",
			0x00000001, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

		#define IUnknown			IUnknown__
		#define IClassFactory		IClassFactory__

	#endif

#endif

#endif
