#include "pdbloader.h"

class PdbPublicFunctionSymbol : public PdbPublicSymbol
{
public:

    PdbPublicFunctionSymbol(const char* _name, uint32_t _rva) {
        name = _name;
        rva = _rva;
    }

    SymbolType Type() const {
        return SymbolType::Function;
    }

    const std::string& Name() const {
        return name;
    }

    uint32_t Rva() const {
        return rva;
    }

private:
    std::string name;
    uint32_t rva;
};


class PdbPublicDataSymbol : public PdbPublicSymbol
{
public:

    PdbPublicDataSymbol(const char* _name, uint32_t _rva) {
        name = _name;
        rva = _rva;
    }

    SymbolType Type() const {
        return SymbolType::Function;
    }

    const std::string& Name() const {
        return name;
    }

    uint32_t Rva() const {
        return rva;
    }

private:
    std::string name;
    uint32_t rva;
};


PdbSession::PdbSession(IDiaDataSource *pSource, IDiaSession *pSession, IDiaSymbol *pGlobal)
{
    dataSource = pSource;
    session = pSession;
    globalSymbol = pGlobal;
}


PdbSession::~PdbSession()
{
}


std::vector<PdbSession::PdbPublicSymbolPtr> PdbSession::GetPublicSymbols()
{
	std::vector<PdbSession::PdbPublicSymbolPtr> results;

	CComPtr<IDiaEnumSymbols> enumSymbols;

	if (FAILED(globalSymbol->findChildren(SymTagPublicSymbol, NULL, nsNone, &enumSymbols.p))) {
		throw std::exception("enum public symbol failed");
	}

	CComPtr<IDiaSymbol> symbol;
	ULONG celt = 0;

	while (SUCCEEDED(enumSymbols->Next(1, &symbol.p, &celt)) && (celt == 1)) {
		
		DWORD symTag;
		DWORD rva;

		if(symbol->get_symTag(&symTag) != S_OK)
			throw std::exception();

		if (symbol->get_relativeVirtualAddress(&rva) != S_OK) {
			rva = 0xFFFFFFFF;
		}

		if(symTag != SymTagPublicSymbol)
			continue;

		CComBSTR name;
		if(symbol->get_name(&name.m_str) != S_OK) throw std::exception();
		
		BOOL isFunction = false;
		if(FAILED(symbol->get_function(&isFunction)))
			throw std::exception();

		if(isFunction) {

			results.push_back(PdbPublicSymbolPtr(new PdbPublicFunctionSymbol(ConvertBstrToLocalString(name).c_str(), rva)));

		} else {

			results.push_back(PdbPublicSymbolPtr(new PdbPublicFunctionSymbol(ConvertBstrToLocalString(name).c_str(), rva)));

		}

		symbol.Release();
	}

	return results;
}


std::string PdbSession::ConvertBstrToLocalString(const BSTR wstr)
{
	int nbytes = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, 0, NULL);
	if(!nbytes) throw std::exception("encoding error");
	std::vector<char> buffer;
	buffer.resize(nbytes);
	nbytes = WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer.data(), buffer.size(), 0, NULL);
	if(!nbytes) throw std::exception("encoding error");
	return std::string(buffer.data());
}


class PdbLoader::CCallback : public IDiaLoadCallback2
{
    int m_nRefCount;
public:
    CCallback() {
        m_nRefCount = 0;
    }

    //IUnknown
    ULONG STDMETHODCALLTYPE AddRef() {
        m_nRefCount++;
        return m_nRefCount;
    }
    ULONG STDMETHODCALLTYPE Release() {
        if ( (--m_nRefCount) == 0 )
            delete this;
        return m_nRefCount;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID rid, void **ppUnk ) {
        if ( ppUnk == NULL ) {
            return E_INVALIDARG;
        }
        if (rid == __uuidof( IDiaLoadCallback2 ) )
            *ppUnk = (IDiaLoadCallback2 *)this;
        else if (rid == __uuidof( IDiaLoadCallback ) )
            *ppUnk = (IDiaLoadCallback *)this;
        else if (rid == __uuidof( IUnknown ) )
            *ppUnk = (IUnknown *)this;
        else
            *ppUnk = NULL;
        if ( *ppUnk != NULL ) {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE NotifyDebugDir(
        BOOL fExecutable,
        DWORD cbData,
        BYTE data[]) { // really a const struct _IMAGE_DEBUG_DIRECTORY *
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE NotifyOpenDBG(
        LPCOLESTR dbgPath,
        HRESULT resultCode) {
        // wprintf(L"opening %s...\n", dbgPath);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE NotifyOpenPDB(
        LPCOLESTR pdbPath,
        HRESULT resultCode) {
        wprintf(L"opening %s...\n", pdbPath);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictRegistryAccess() {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess() {
		wprintf(L"将要从网络下载PDB文件\n");
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictOriginalPathAccess() {
        // return hr != S_OK to prevent querying the registry for symbol search paths
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictReferencePathAccess() {
        // return hr != S_OK to prevent accessing a symbol server
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictDBGAccess() {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE RestrictSystemRootAccess() {
		return S_OK;
    }
};


PdbLoader::PdbLoader()
{

    HRESULT hr = CoInitialize(NULL);
    if(FAILED(hr)) throw std::exception("CoCreateInstance failed");

}


PdbLoader::~PdbLoader()
{

    CoUninitialize();

}


PdbSession PdbLoader::Load(const wchar_t* filePath, const wchar_t* pdbSearchPath)
{

    CComPtr<IDiaDataSource> dataSource;
    CComPtr<IDiaSession> session;
    CComPtr<IDiaSymbol> globalSymbol;
    HRESULT hr;

    wchar_t wszExt[MAX_PATH];

    hr = CoCreateInstance(__uuidof(DiaSource),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IDiaDataSource),
                          (void **) &dataSource.p);

    if (FAILED(hr)) {
        throw std::exception("Create IDiaDataSource Failed");
    }

    _wsplitpath_s(filePath, NULL, 0, NULL, 0, NULL, 0, wszExt, MAX_PATH);

    if (!_wcsicmp(wszExt, L".pdb")) {

        hr = dataSource->loadDataFromPdb(filePath);

        if (FAILED(hr)) {
            throw std::exception("loadDataFromPdb failed");
        }
    } else {

		if(!pdbSearchPath)
			throw std::exception("we need to search for pdb but pdbSearchPath is Null");

        CCallback callback;

        callback.AddRef();

        hr = dataSource->loadDataForExe(filePath, pdbSearchPath, &callback);

        if (FAILED(hr)) {
            throw std::exception("loadDataForExe failed");
        }
    }

    // Open a session for querying symbols

    hr = dataSource->openSession(&session.p);

    if (FAILED(hr)) {
        throw std::exception("openSession failed");
    }

    // Retrieve a reference to the global scope

    hr = session->get_globalScope(&globalSymbol.p);

    if (hr != S_OK) {
        throw std::exception("get_globalScope failed");
    }

    return PdbSession(dataSource, session, globalSymbol);
}