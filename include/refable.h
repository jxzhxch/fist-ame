#ifndef __REF_PTR_IMPL__
#define __REF_PTR_IMPL__

struct IRefable
{
	virtual long _reference() = 0;
	virtual long _release() = 0;
};

#define REFABLE_IMP_ST	\
	virtual long _reference() { return ++_ref_count; };	\
	virtual long _release(){ long r = --_ref_count; if( !r ) delete this; return r; };

#if TARGET_OS == OS_WINDOWS

    #define REFABLE_IMP_MT	\
        virtual long _reference() { return InterlockedIncrement( &_ref_count ); };	\
        virtual long _release(){ long r = InterlockedDecrement( &_ref_count ); if( !r ) delete this; return r; };
#else
    #define REFABLE_IMP_MT  REFABLE_IMP_ST
#endif
struct RefableImp : IRefable
{
protected:
	long	_ref_count;
public:
	RefableImp() : _ref_count(0) {};
	virtual ~RefableImp() {};
	REFABLE_IMP_MT;
};

struct Refalbe
{
protected:
	long	_ref_count;
public:
	Refalbe() : _ref_count(0) {};
};

template < class T >
struct refp
{
	T * m_p;
	refp() : m_p(0)
	{}

	refp( int p ) : m_p(0)
	{
	}
	template< class P>
	refp( P* p ) : m_p(0)
	{
		bind( (T*)p );
	}
	refp( const refp & r ) : m_p(0)
	{
		bind( r.m_p );
	}
	refp & operator = ( const refp<T> & t )
	{
		return bind( t.m_p );
	}
	refp & operator = ( T* p )
	{
		return bind( p );
	}
	template< class P>
	refp & operator = ( P * p )
	{
		return bind( (T*)p );
	}
	~refp()
	{
		dispose();
	}
	bool operator < ( const refp & r )
	{
		return m_p < r.m_p;
	}
	refp & bind( T * p, bool rf = true )
	{
		dispose();
		m_p = (T*)p;
		if( rf && m_p ) m_p->_reference();
		return *this;
	}
	T * operator -> ( )
	{
		return m_p;
	}
	bool operator < ( const refp<T> & r ) const
	{
		return m_p < r.m_p;
	}
	operator T * ()
	{
		return m_p;
	}
	void dispose()
	{
		if( m_p ) m_p->_release();
		m_p = 0;
	}
	void attach( T * p )
	{
		dispose();
		m_p = p;
	}
	T * detach()
	{
		T * r = m_p;
		m_p = 0;
		return r;
	}
	template < class U >
	U * cast()
	{
		return (U*)m_p;
	}
	T** pp()
	{
		dispose();
		return &m_p;
	}
};

//////////////////////////////////////////////////////////////////////////
template < class T >
struct tcop
{
	T * m_p;
	tcop() : m_p(0)
	{}

	tcop( int p ) : m_p(0)
	{
	}
	template< class P>
	tcop( P* p ) : m_p(0)
	{
		bind( (T*)p );
	}
	tcop( const tcop & r ) : m_p(0)
	{
		bind( r.m_p );
	}
	tcop & operator = ( const tcop<T> & t )
	{
		return bind( t.m_p );
	}
	tcop & operator = ( T* p )
	{
		return bind( p );
	}
	template< class P>
	tcop & operator = ( P * p )
	{
		return bind( (T*)p );
	}
	~tcop()
	{
		destroy();
	}
	bool operator < ( const tcop & r )
	{
		return m_p < r.m_p;
	}
	tcop & bind( T * p, bool rf = true )
	{
		destroy();
		m_p = (T*)p;
		if( rf && m_p ) m_p->_reference();
		return *this;
	}
	T * operator -> ( )
	{
		return m_p;
	}
	bool operator < ( const tcop<T> & r ) const
	{
		return m_p < r.m_p;
	}
	operator T * ()
	{
		return m_p;
	}
	void destroy()
	{
		if( m_p ) m_p->_release();
		m_p = 0;
	}
	void attach( T * p )
	{
		destroy();
		m_p = p;
	}
	T * detach()
	{
		T * r = m_p;
		m_p = 0;
		return r;
	}
};


#endif
