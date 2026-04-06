#include <windows.h>
#include <string>

// MSVC লিংকারের জন্য রেজিস্ট্রি লাইব্রেরি
#pragma comment(lib, "advapi32.lib")

using namespace std;

void ToggleAdBlock(bool enable) {
    HKEY hKey;
    
    // Chrome Registry Path
    string chromePath = "SOFTWARE\\Policies\\Google\\Chrome\\ExtensionInstallForcelist";
    string adGuardChrome = "bgnkhhnnamicmpeenaelnjfhikgbkllg;https://clients2.google.com/service/update2/crx";
    
    // Edge Registry Path
    string edgePath = "SOFTWARE\\Policies\\Microsoft\\Edge\\ExtensionInstallForcelist";
    string adGuardEdge = "pdffkfellgipmhklpdmokmckkkfcopbh;https://edge.microsoft.com/extensionwebstorebase/v1/crx";

    if (enable) {
        if (RegCreateKeyExA(HKEY_CURRENT_USER, chromePath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            // static_cast<DWORD> দিয়ে সাইজ কনভার্সন ওয়ার্নিং ফিক্স করা হয়েছে
            RegSetValueExA(hKey, "1", 0, REG_SZ, (const BYTE*)adGuardChrome.c_str(), static_cast<DWORD>(adGuardChrome.length() + 1));
            RegCloseKey(hKey);
        }
        
        if (RegCreateKeyExA(HKEY_CURRENT_USER, edgePath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "1", 0, REG_SZ, (const BYTE*)adGuardEdge.c_str(), static_cast<DWORD>(adGuardEdge.length() + 1));
            RegCloseKey(hKey);
        }
    } else {
        if (RegOpenKeyExA(HKEY_CURRENT_USER, chromePath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueA(hKey, "1");
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExA(HKEY_CURRENT_USER, edgePath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueA(hKey, "1");
            RegCloseKey(hKey);
        }
    }
}
