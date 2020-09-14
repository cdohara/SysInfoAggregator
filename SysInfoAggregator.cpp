#include <Windows.h>
#include <intrin.h>
#include <iostream>
#include <string>
#include <fstream>
#include <lmcons.h>
#include <atlstr.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iostream>

using namespace std;

wstring getRAM() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    return to_wstring(statex.ullTotalPhys / (1024.0 * 1024 * 1024));
}

wstring getDriveSpace() {
    unsigned __int64 total_space, free_space;
    GetDiskFreeSpaceExA(NULL, NULL, (PULARGE_INTEGER)&total_space, (PULARGE_INTEGER)&free_space);
    return to_wstring(free_space / (1024.0 * 1024 * 1024)) + L"/" + to_wstring(total_space / (1024.0 * 1024 * 1024));
}

wstring getCPU() {
    int CPUInfo[4] = { -1 };
    char CPUBrandString[0x40];
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];
    memset(CPUBrandString, 0, sizeof(CPUBrandString));
    for (int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(CPUInfo, i);
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    wstring ws(&CPUBrandString[0], &CPUBrandString[0x40]);
    return ws;
}

#define _WIN32_DCOM
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
wstring getGPU() { // I honestly couldn't find a better way. Maybe use OpenGL, but that's just overkill.
    wstring tempgpu;
    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) break;
        VARIANT vtProp;
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        //wcout << " GPU : " << vtProp.bstrVal << endl;
        tempgpu = vtProp.bstrVal;
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return tempgpu;
}

int main() {
    wstring out;
    out.append(L"CPU:\t\t" + getCPU() + L"\n");
    out.append(L"RAM:\t\t" + getRAM() + L" GB\n");
    out.append(L"Drive Space:\t" + getDriveSpace() + L" GB\n");
    out.append(L"GPU:\t\t" + getGPU() + L"\n");
    wcout << out;
    string str(out.begin(), out.end());
    ofstream outputFile("output.txt");
    outputFile << str;
    cout << "Created file named \"" << "output.txt" << "\" in the base directory.\n";
    cout << "Press the enter key to exit!\t\t\\(^_^)/" << endl;
    cin.get();
    return 0;
}
