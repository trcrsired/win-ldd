#include <stdio.h>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <string>

#include <windows.h>
#include <dbghelp.h>

typedef std::map<std::string, std::string> DepMapType;

//------------------------------------------------------------------------------

inline const DepMapType getDependencies(const HMODULE hMod)
{
    DepMapType depsMap;

    IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)hMod;
    IMAGE_OPTIONAL_HEADER * pOptHeader = (IMAGE_OPTIONAL_HEADER*)((BYTE*)hMod + pDosHeader->e_lfanew +24);
    IMAGE_IMPORT_DESCRIPTOR* pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*) ((BYTE*)hMod + pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    while ( pImportDesc->FirstThunk )
    {
        char* dllName = (char*) ((BYTE*)hMod + pImportDesc->Name);

        std::string dllPath = "";
        std::unique_ptr<typename std::remove_pointer<HMODULE>::type,decltype(FreeLibrary)*> hModDep(::LoadLibraryEx(dllName, NULL, DONT_RESOLVE_DLL_REFERENCES),FreeLibrary);
        if(hModDep)
        {
            std::unique_ptr<typename std::remove_pointer<LPTSTR>::type[]>  strDLLPath(new TCHAR[_MAX_PATH]);
            ::GetModuleFileName(hModDep.get(), strDLLPath.get(), _MAX_PATH);
            dllPath = std::string(strDLLPath.get());
        }
        depsMap[std::string(dllName)] = dllPath;

        pImportDesc++;
    }

    return depsMap;
}

//------------------------------------------------------------------------------

inline int printDependencies(::std::ostream& os,char const* libPath)
{
    std::unique_ptr<typename std::remove_pointer<HMODULE>::type,decltype(FreeLibrary)*> hMod(::LoadLibraryEx(libPath, NULL, DONT_RESOLVE_DLL_REFERENCES),FreeLibrary);
    if(!hMod)
    {
        // Retrieves the last error message.
        DWORD   lastError = GetLastError();
        char    buffer[1024];
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, lastError, 0, buffer, 1024, 0 );
        fprintf(stderr,"\t%s\n",buffer);
        return -1;
    }
    const DepMapType& depMap = getDependencies(hMod.get());
    DepMapType::const_iterator iter;
    for(iter = depMap.begin(); iter != depMap.end(); ++iter)
    {
        os << "\t" << iter->first << " => " << iter->second << '\n';
    }

    return 0;
}

//------------------------------------------------------------------------------

inline int printUsage()
{
    puts("ldd for Windows, Version 1.0\n"
        "usage:\n ldd FILE...\n");
    return -1;
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    if(argc <= 1)
    {
        return printUsage();
    }
    int res = 0;
    ::std::ostringstream oss;
    for(int i = 1; i < argc; ++i)
    {
        oss<<argv[i]<<'\n';
        res = printDependencies(oss, argv[i]);
    }
    ::std::string ossstr = oss.str();
    fwrite(ossstr.data(),1,ossstr.size(),stdout);
    return res;
}

//------------------------------------------------------------------------------

