#pragma once
#include "dia2.h"
#include <string>
#include <stdint.h>
#include <atlbase.h>
#include <memory>
#include <vector>


class PdbPublicSymbol
{
public:
	enum class SymbolType {
		Data, 
		Function
	};
public:
	
	virtual ~PdbPublicSymbol() {}

	virtual SymbolType Type() const = 0;

	virtual const std::string& Name() const = 0;

	virtual uint32_t Rva() const = 0;
};

class PdbSession
{
public:

    PdbSession(IDiaDataSource *pSource, IDiaSession *pSession, IDiaSymbol *pGlobal);

    ~PdbSession();

	typedef std::unique_ptr<PdbPublicSymbol> PdbPublicSymbolPtr;

	std::vector<PdbPublicSymbolPtr> GetPublicSymbols();

private:

	std::string ConvertBstrToLocalString(const BSTR);

private:

    CComPtr<IDiaDataSource> dataSource;
    CComPtr<IDiaSession> session;
    CComPtr<IDiaSymbol> globalSymbol;
};

class PdbLoader
{
    class CCallback;
public:

    PdbLoader();

    ~PdbLoader();

    PdbSession Load(const wchar_t* filePath, const wchar_t* pdbSearchPath = 0);
};