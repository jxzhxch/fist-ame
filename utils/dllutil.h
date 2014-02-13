#ifndef __RE_DLLUTIL_H__
#define __RE_DLLUTIL_H__

#if defined(PLATFORM_TYPE_POSIX)
//#include <dlfcn.h>
//#include <stdio.h>
#endif

struct CDllUtil
{
#if !defined(PLATFORM_TYPE_POSIX)
	static HMODULE WINAPI LoadLibraryA( LPCSTR lpLibFileName )
	{
		return ::LoadLibraryA(lpLibFileName);
	}
	static HMODULE WINAPI LoadLibraryW( LPCWSTR lpLibFileName )
	{
		return ::LoadLibraryW(lpLibFileName);
	}
	static FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
	{
		return ::GetProcAddress(hModule, lpProcName);
	}

	static BOOL WINAPI FreeLibrary( HMODULE hLibModule )
	{
		return ::FreeLibrary(hLibModule);
	}

#else //PLATFORM_TYPE_POSIX
	static HMODULE WINAPI LoadLibraryA( LPCSTR lpLibFileName )
	{
		HMODULE h = dlopen(lpLibFileName, RTLD_LAZY);
		if (!h)
		{
			//fputs(dlerror(), stderr);
		}
		return h;
	}

	static FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
	{
		return (FARPROC)dlsym(hModule, lpProcName);
	}

	static BOOL WINAPI FreeLibrary( HMODULE hLibModule )
	{
		return dlclose(hLibModule);
	}
#endif //PLATFORM_TYPE_POSIX
};


#endif //__RE_DLLUTIL_H__
