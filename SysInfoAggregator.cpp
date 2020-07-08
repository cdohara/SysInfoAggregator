#include <Windows.h>
#include <stdio.h>
#include <string>
#include <intrin.h>
#include <lmcons.h>
#include <atlstr.h>
#include <fstream>
#include <iostream>

using namespace std;

wstring textOutput = L"";

#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")


#define _WIN32_DCOM
#include <iostream>
using namespace std;
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

wstring tempgpu;

int wmicom()
{
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x"
            << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );


    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                    // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x"
            << hex << hres << endl;
        pLoc->Release();
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x"
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        cout << "Query for GPU failed."
            << " Error code = 0x"
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        wcout << " GPU : " << vtProp.bstrVal << endl;
        tempgpu = vtProp.bstrVal;
        
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return 0;   // Program successfully completed.

}

wstring GetProcessorName()
{
    int CPUInfo[4] = { -1 };
    char CPUBrandString[0x40];
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    // Get the information associated with each extended ID.
    for (int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string.
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

bool EnumInstalledSoftware(void)
{
    HKEY hUninstKey = NULL;
    HKEY hAppKey = NULL;
    WCHAR sAppKeyName[1024];
    WCHAR sSubKey[1024];
    WCHAR sDisplayName[1024];
    //const WCHAR* sRoot = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    const WCHAR* sRoot = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    long lResult = ERROR_SUCCESS;
    DWORD dwType = KEY_ALL_ACCESS;
    DWORD dwBufferSize = 0;

    //Open the "Uninstall" key.
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sRoot, 0, KEY_READ, &hUninstKey) != ERROR_SUCCESS)
    {
        return false;
    }

    for (DWORD dwIndex = 0; lResult == ERROR_SUCCESS; dwIndex++)
    {
        //Enumerate all sub keys...
        dwBufferSize = sizeof(sAppKeyName);
        if ((lResult = RegEnumKeyEx(hUninstKey, dwIndex, sAppKeyName,
            &dwBufferSize, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
        {
            //Open the sub key.
            wsprintf(sSubKey, L"%s\\%s", sRoot, sAppKeyName);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, sSubKey, 0, KEY_READ, &hAppKey) != ERROR_SUCCESS)
            {
                RegCloseKey(hAppKey);
                RegCloseKey(hUninstKey);
                return false;
            }

            //Get the display name value from the application's sub key.
            dwBufferSize = sizeof(sDisplayName);
            if (RegQueryValueEx(hAppKey, L"DisplayName", NULL,
                &dwType, (unsigned char*)sDisplayName, &dwBufferSize) == ERROR_SUCCESS)
            {
                //wprintf(L"%s\n", sDisplayName);
                wstring ws(sDisplayName);
                textOutput.append(ws+L"\n");
            }
            else {
                //Display name value doe not exist, this application was probably uninstalled.
            }

            RegCloseKey(hAppKey);
        }
    }

    RegCloseKey(hUninstKey);

    return true;
}

CString GetStringFromReg(HKEY keyParent, CString keyName, CString keyValName)
{
    CRegKey key;
    CString out;
    if (key.Open(keyParent, keyName, KEY_READ) == ERROR_SUCCESS)
    {
        ULONG len = 256;
        key.QueryStringValue(keyValName, out.GetBuffer(256), &len);
        out.ReleaseBuffer();
        key.Close();
    }
    return out;
}




int main() {
    // computer name
    cout << "Getting computer name... ";
    TCHAR compname[UNCLEN + 1];
    DWORD compname_len = UNCLEN + 1;
    GetComputerName((TCHAR*)compname, &compname_len);
    wstring wcomp(compname);
    textOutput.append(L"Computer Name: " + wcomp + L"\n");
    cout << "Done." << endl;


    // username
    cout << "Getting username... ";
    TCHAR username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    GetUserName((TCHAR*)username, &username_len);
    wstring wuser(username);
    textOutput.append(L"Username: " + wuser + L"\n");
    cout << "Done." << endl;


    // get cpu brand
    cout << "Getting CPU... ";
    textOutput.append(L"CPU: " + GetProcessorName() + L"\n");
    cout << "Done." << endl;


    // get ram
    cout << "Getting RAM size... ";
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    int ram = round(statex.ullTotalPhys / (1.0*1024*1024*1024));
    textOutput.append(L"RAM: " + to_wstring(ram) + L" GB\n");
    cout << "Done." << endl;


    // get drive space
    cout << "Getting remaining and total drive space... ";
    LPCWSTR pszDrive = NULL;
    BOOL check, fResult;
    __int64 lpFreeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes;
    check = GetDiskFreeSpaceEx(pszDrive,(PULARGE_INTEGER)&lpFreeBytesAvailable,(PULARGE_INTEGER)&lpTotalNumberOfBytes,(PULARGE_INTEGER)&lpTotalNumberOfFreeBytes);
    textOutput.append(L"Drive Space: " + to_wstring(lpTotalNumberOfFreeBytes / (1024 * 1024 * 1024)) + L" GB /" + to_wstring(lpTotalNumberOfBytes / (1024 * 1024 * 1024)) + L" GB \n");
    cout << "Done." << endl;
    

    // get os version
    cout << "Getting Windows version... ";
    CString osname = GetStringFromReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
    wstring os(osname);
    textOutput.append(L"OS: " + os + L"\n");
    cout << "Done." << endl;
    
    
    // get gpu
    cout << "Getting GPU... ";
    wmicom();
    textOutput.append(L"GPU: " + tempgpu +  L"\n");
    cout << "Done." << endl;


    

    // Get installed programs
    cout << "Getting list of installed programs... ";
    textOutput.append(L"Installed Programs:\n");
    EnumInstalledSoftware();
    cout << "Done." << endl;

    // for debugging only
    
    string str(textOutput.begin(), textOutput.end());
    //cout << str << endl;
    

    cout << "\n===================================\n" << endl;
    // output

    string usernamestr(wuser.begin(), wuser.end());
    usernamestr += ".txt";
    cout << "Created file named \"" << usernamestr << "\" in the base directory.\n" << endl;

    ofstream outputFile(usernamestr);
    outputFile << str;

    cout << "Press the enter key to exit!     \\(^_^)/" << endl;
    cin.get();

    return 0;
}
