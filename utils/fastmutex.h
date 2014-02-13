#ifndef __RSUTIL_FAST_MUTEX__
#define __RSUTIL_FAST_MUTEX__

namespace syn
{

	#if (TARGET_OS==OS_WINDOWS) || !defined(TARGET_OS)

	struct FastMutex
	{
		CRITICAL_SECTION cri;
		FastMutex() { ::InitializeCriticalSection(&cri); }
		~FastMutex() { ::DeleteCriticalSection(&cri); }
		void Acquire() { ::EnterCriticalSection(&cri); }
		void Release() { ::LeaveCriticalSection(&cri); }
	};

	#elif (TARGET_OS==OS_POSIX)

	struct FastMutex
	{
		FastMutex() {}
		~FastMutex() {}
		void Acquire() {}
		void Release() {}
	};

	#elif (TARGET_OS==OS_NATIVE)

	struct FastMutex
	{
		FastMutex() {}
		~FastMutex() {}
		void Acquire() {}
		void Release() {}
	};

	#endif

	struct FastMutexScope
	{
		FastMutex& _Mutex;
		FastMutexScope( FastMutex& rMutex ) : _Mutex(rMutex)
		{
			_Mutex.Acquire();
		}
		~FastMutexScope()
		{
			_Mutex.Release();
		}
	};

};

#endif