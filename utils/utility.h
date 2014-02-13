#ifndef _RE_UTILITY_H_
#define _RE_UTILITY_H_

#define RSUCCEEDED(x)	{ HRESULT _h_r_ = (x); if(SUCCEEDED(_h_r_)) return _h_r_; }
#define RFAILED(x)		{ HRESULT _h_r_ = (x); if(FAILED(_h_r_)) { return _h_r_; } }
#define RFAILEDV(x)	{ if(FAILED(x)) return; }
#define RFAILEDP(x, _h_r_)	{ if(FAILED(x)) return _h_r_; }
#define RFAILEDOK(x)  RFAILEDP(x, S_OK;)
#define RFAILEDESFALSE(x) RFAILEDP(x, S_FALSE;)
#define RFAILED2(x, hr) { HRESULT _h_r_ = (x); if (FAILED(_h_r_)) return _h_r_; if (hr == _h_r_) return hr; }
#define RFAILED_(x,r) { if(FAILED(x)) return r; }
#define RSUCCEED(x)			{ HRESULT hr = (x); if( SUCCEEDED(hr) ) return hr; };

#define RASSERT(x, _h_r_) { if(!(x)) return _h_r_; }
#define RASSERTV(x) { if(!(x)) return; }
#define RASSERTP(x, _h_r_) { if(NULL==(x)) return _h_r_; }
#define RASSERTPV(x) RASSERTP(x, ; )
#define TASSERT(x, e) { if (!(x)) throw (e); }
#define RTEST(x, _h_r_) { if((x)) return _h_r_; }

#define NOTIMPL { return E_NOTIMPL; }
#define NOTIMPL_ { ASSERT0; return E_NOTIMPL; }


namespace UTIL { ;

struct default_sentry
{
	static void* default_value() { return 0; }
	template<class _Ptr> static bool equal_to(_Ptr l, _Ptr r) { return l == r; }
	template<class _Ptr> static void destroy(_Ptr p) { delete p; }
};

struct default_array_sentry
{
	static void* default_value() { return 0; }
	template<class _Ptr> static bool equal_to(_Ptr l, _Ptr r) { return l == r; }
	template<class _Ptr> static void destroy(_Ptr p) { delete [] p; }
};

typedef default_array_sentry _DAS;

struct co_interface_sentry
{
	static void* default_value() { return 0; }
	template<class _Ptr> static bool equal_to(_Ptr, _Ptr) { return false; }
	template<class _Ptr> static void destroy(_Ptr p) { if(p) p->Release(); }
};


template<class _Ptr,
		 class _Traits = default_sentry>
struct sentry
{
public:
	sentry(const _Traits& tr = _Traits()) : m_tr(tr) { m_p = (_Ptr)m_tr.default_value(); }
	sentry(_Ptr p, const _Traits& tr = _Traits()) : m_p(p), m_tr(tr) {}
	~sentry() { m_tr.destroy(m_p); }
	sentry& operator = (_Ptr p) { if(!m_tr.equal_to(m_p, p)) { m_tr.destroy(m_p); m_p = p; } return *this; }
	_Ptr detach() { _Ptr tmp = m_p; m_p = (_Ptr)m_tr.default_value(); return tmp; }
	void dispose(){ m_tr.destroy(m_p); m_p = (_Ptr)m_tr.default_value(); };
	operator _Ptr () const { return m_p; }
	_Ptr operator -> () const { return m_p; }
	//不能再使用这个操作符重载了!
	//_Ptr* operator & () { return &m_p; }
	_Ptr* pp( bool _dispose = true ) { if(_dispose) dispose(); return &m_p; }
	_Ptr& rp( bool _dispose = true ) { if(_dispose) dispose(); return m_p; }
public:
	_Ptr m_p;
	_Traits m_tr;
private:
	sentry(const sentry&);
	sentry& operator = (const sentry&);
};

template<class I>
struct com_ptr : sentry<I*, co_interface_sentry>
{
	typedef sentry<I*, co_interface_sentry> base;
	using base::m_p; // gcc 3.4
	// default construct:
	com_ptr() : base() {}
	// construct with:
	template<class U>
		com_ptr(const com_ptr<U>& rhs) : base()
		{
		    if(rhs.m_p && FAILED(rhs.m_p->QueryInterface(re_uuidof(I), (void**)&m_p)))
                m_p = 0;
        } // gcc
	com_ptr(const com_ptr& rhs) : base() { if(rhs.m_p && FAILED(rhs.m_p->QueryInterface(re_uuidof(I), (void**)&m_p))) m_p = 0; } // gcc
	template<class U>
		com_ptr(const sentry<U*, co_interface_sentry>& rhs) : base() { if(rhs.m_p && FAILED(rhs.m_p->QueryInterface(re_uuidof(I), (void**)&m_p))) m_p = 0; } // gcc

	template<class U>
		com_ptr(U *p) : base() { if(p && FAILED(p->QueryInterface(re_uuidof(I), (void**)&m_p))) m_p = 0; }
	// operator =:
	template<class U>
		com_ptr& operator = (const com_ptr<U>& rhs) { if((void*)m_p != (void*)rhs.m_p) *this = rhs.m_p; return *this; }

	com_ptr& operator = (const com_ptr& rhs) { if(m_p != rhs.m_p) *this = rhs.m_p; return *this; }
	template<class U>
		com_ptr& operator = (const sentry<U*, co_interface_sentry>& rhs) { if((void*)m_p != (void*)rhs.m_p) *this = rhs.m_p; return *this; }

	template<class U>
		com_ptr& operator = (U *p) { if((void*)m_p == (void*)p) return *this; base::operator=(0); if(p && FAILED(p->QueryInterface(re_uuidof(I), (void**)&m_p))) m_p = 0; return *this; }
};

} // namespace UTIL


// macro - like


template<typename T, typename K>
T __stub(T n, K a)       { return n %a; }


template<typename T, typename K>
T __padding(T n, K a)    { return a -__stub(n-1, a) -1; }


template<typename T, typename K>
T __align_up(T n, K a)   { return n +__padding(n, a); }


template<typename T, typename K>
T __align_down(T n, K a) { return n -__stub(n, a); }

#define comptr UTIL::com_ptr

#endif // duplicate inclusion protection
