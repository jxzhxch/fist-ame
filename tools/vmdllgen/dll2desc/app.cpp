#include "pdbloader.h"
#include "sparsefile.h"
#include "pe_bliss_1.0.0\pe_lib\pe_bliss.h"
#include "yaml-cpp-0.5.1\include\yaml-cpp\yaml.h"
#undef max
#include "getopt\getopt.h"
#include <algorithm>
#include <exception>
#include <fstream>
#include <stdio.h>

void help()
{
    wprintf(L"dll2desc [-h] [-s symbol_search_path] [-o output_dir] dll_path\n");
}


void GenerateDll(std::ifstream& stm, pe_bliss::pe_base& image, const wchar_t* outputPath)
{

    stm.seekg(0, std::ios::beg);
    SparseFileWritter generatedDll;

    std::vector<char> buffer;

    if(buffer.size() < image.get_pe_header_start())
        buffer.resize(image.get_pe_header_start());
    stm.read(buffer.data(), image.get_pe_header_start());
    if(stm.gcount() != image.get_pe_header_start()) throw std::exception();
    generatedDll.Write(buffer.data(), image.get_pe_header_start());

    if(image.get_pe_type() == pe_bliss::pe_type::pe_type_32) {
        if(buffer.size() < sizeof(IMAGE_NT_HEADERS32))
            buffer.resize(sizeof(IMAGE_NT_HEADERS32));
        stm.read(buffer.data(), sizeof(IMAGE_NT_HEADERS32));
        if(stm.gcount() != sizeof(IMAGE_NT_HEADERS32)) throw std::exception();
        IMAGE_NT_HEADERS32* header = (IMAGE_NT_HEADERS32*)buffer.data();
        //memset(header->OptionalHeader.DataDirectory + 1, 0, sizeof(IMAGE_DATA_DIRECTORY) * (IMAGE_NUMBEROF_DIRECTORY_ENTRIES - 1));
        generatedDll.Write(buffer.data(), sizeof(IMAGE_NT_HEADERS32));
    } else {
        if(buffer.size() < sizeof(IMAGE_NT_HEADERS64))
            buffer.resize(sizeof(IMAGE_NT_HEADERS64));
        stm.read(buffer.data(), sizeof(IMAGE_NT_HEADERS64));
        if(stm.gcount() != sizeof(IMAGE_NT_HEADERS64)) throw std::exception();
        IMAGE_NT_HEADERS64* header = (IMAGE_NT_HEADERS64*)buffer.data();
        //memset(header->OptionalHeader.DataDirectory + 1, 0, sizeof(IMAGE_DATA_DIRECTORY) * (IMAGE_NUMBEROF_DIRECTORY_ENTRIES - 1));
        generatedDll.Write(buffer.data(), sizeof(IMAGE_NT_HEADERS64));
    }

    if(buffer.size() < sizeof(IMAGE_SECTION_HEADER) * image.get_number_of_sections())
        buffer.resize(sizeof(IMAGE_SECTION_HEADER) * image.get_number_of_sections());
    stm.read(buffer.data(), sizeof(IMAGE_SECTION_HEADER) * image.get_number_of_sections());
    if(stm.gcount() != sizeof(IMAGE_SECTION_HEADER) * image.get_number_of_sections()) throw std::exception();
    generatedDll.Write(buffer.data(), sizeof(IMAGE_SECTION_HEADER) * image.get_number_of_sections());

    if(image.has_exports()) {

        uint32_t exportRva = image.get_directory_rva(pe_bliss::pe_win::image_directory_entry_export);
        uint32_t exportSize = image.get_directory_size(pe_bliss::pe_win::image_directory_entry_export);
        uint32_t exportFileOffset = image.rva_to_file_offset(exportRva);
        buffer.resize(exportSize);
        stm.seekg(exportFileOffset, std::ios::beg);
        stm.read(buffer.data(), buffer.size());
        generatedDll.Seek(exportFileOffset);
        generatedDll.Write(buffer.data(), buffer.size());

    }

    stm.seekg(0, std::ios::end);
    generatedDll.Seek(stm.tellg());
    generatedDll.SetEndOfFile();

    std::ofstream outputStream(outputPath, std::ios::binary);
    //generatedDll.SerialToStreamUnsparse(outputStream);
    generatedDll.SerialToStream(outputStream);

}

struct PdbSymbolComparer {
    bool operator() (const PdbSession::PdbPublicSymbolPtr& lhs, const PdbSession::PdbPublicSymbolPtr& rhs) {
        return lhs->Rva() < rhs->Rva();
    }
	bool operator() (const PdbSession::PdbPublicSymbolPtr& lhs, uint32_t rva) {
        return lhs->Rva() < rva;
    }
};

void GenerateDesc(const wchar_t* filePath, pe_bliss::pe_base& image, const wchar_t* symbolSearchPath, const wchar_t* outputPath)
{

    std::ofstream output(outputPath);

    if(!image.has_exports()) return;

    PdbLoader loader;
    PdbSession session = loader.Load(filePath, symbolSearchPath);
    auto symbols = session.GetPublicSymbols();

    std::sort(begin(symbols), end(symbols), PdbSymbolComparer());

    pe_bliss::export_info info;
    const pe_bliss::exported_functions_list exports = pe_bliss::get_exported_functions(image, info);
    for(auto exportedFunc : exports) {

        if(exportedFunc.is_forwarded())
            continue;

		char exportName[512];
        if(exportedFunc.has_name()) {
            _snprintf(exportName, sizeof(exportName), "%s", exportedFunc.get_name().c_str());
            exportName[sizeof(exportName) - 1] = 0;
        } else {
            _snprintf(exportName, sizeof(exportName), "#%d", exportedFunc.get_ordinal());
            exportName[sizeof(exportName) - 1] = 0;
        }

        int paramNum = 0;
        bool isData = false;

        auto it = std::lower_bound(begin(symbols), end(symbols), exportedFunc.get_rva(), PdbSymbolComparer());

		if(it == end(symbols)) {
			wprintf(L"警告：没有在PDB中找到有关导出函数%S的信息\n", exportName);
		}

        if(it != end(symbols) && (*it)->Rva() == exportedFunc.get_rva()) {
            if((*it)->Type() == PdbPublicSymbol::SymbolType::Data) {
                isData = true;
            } else {
                auto name = (*it)->Name();
                int n = name.find_last_of('@');
                if(n != std::string::npos) {
					int stack = 0;
					sscanf(&name[n+1], "%d", &stack);
					paramNum = stack / 4;
				}
            }
        }

        char buffer[1024];
        _snprintf(buffer, sizeof(buffer), "0x%08x,%s,%d,%s\n",
                  exportedFunc.get_rva(),
                  exportName,
                  paramNum,
                  isData ? "d" : "f");
        buffer[sizeof(buffer) - 1] = 0;

        output << buffer;
    }
}


int _wmain(int argc, wchar_t** argv)
{
    const wchar_t* symbolSearchPath = L"SRV*.\\symbols*http://msdl.microsoft.com/download/symbols";
    const wchar_t* inputPath;
    const wchar_t* inputFileName;
    std::wstring outputDir;

    int c;
    while((c = getopt(argc, argv, L"hs:o:")) != -1) {
        switch(c) {
        case L'h':
            help();
            return 0;
        case L's':
            symbolSearchPath = optarg;
            break;
        case L'o':
            outputDir = optarg;
            break;
        }
    }

    if(optind + 1 != argc) {
        help();
        return 0;
    }
    inputPath = argv[optind ++];

    inputFileName = std::max(wcsrchr(inputPath, L'\\'), wcsrchr(inputPath, L'/'));
    if(!inputFileName) inputFileName = inputPath;

    if(outputDir.empty()) {
        outputDir = L".\\";
    }

    if((outputDir[outputDir.size() - 1] != L'\\') && (outputDir[outputDir.size() - 1] != L'/'))
        outputDir.push_back(L'\\');

    std::ifstream dllFile(inputPath, std::ios::in | std::ios::binary);
    if(!dllFile) {
        throw std::exception("open target file failed");
    }

    pe_bliss::pe_base image(pe_bliss::pe_factory::create_pe(dllFile));

    {
        std::ifstream dllStream(inputPath, std::ios::in | std::ios::binary);
        std::wstring outputPath = outputDir;
        outputPath.append(inputFileName);
        outputPath.append(L".gen");
        GenerateDll(dllStream, image, outputPath.c_str());
    }

    {
        std::wstring outputPath = outputDir;
        outputPath.append(inputFileName);
        outputPath.append(L".txt");
        GenerateDesc(inputPath, image, symbolSearchPath, outputPath.c_str());
    }

    return 0;
}


int wmain(int argc, wchar_t** argv)
{
    try {
		setlocale(LC_ALL, ".ACP");
        return _wmain(argc, argv);
    } catch (std::exception& e) {
        wprintf(L"发生错误: %S\n", e.what());
        return -1;
    }
}