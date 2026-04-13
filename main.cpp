#include <windows.h>
#include <tlhelp32.h>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <commctrl.h>
#include <shellapi.h>
#include <time.h>
#include <dwmapi.h>
#include <wininet.h>
#include <shlobj.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")

using namespace std;

#define WM_TRAYICON (WM_USER + 1)
#define WM_WAKEUP (WM_USER + 2) 

// UI IDs
#define ID_RADIO_BLOCK 101
#define ID_RADIO_ALLOW 102
#define ID_BTN_ADD_ALLOW_APP 103
#define ID_BTN_REM_ALLOW_APP 104
#define ID_BTN_ADD_ALLOW_WEB 105
#define ID_BTN_REM_ALLOW_WEB 106
#define ID_BTN_STOP_FOCUS 107
#define ID_BTN_START_FOCUS 1
#define ID_BTN_ADD_APP 2
#define ID_BTN_REM_APP 4
#define ID_BTN_ADD_WEB 6
#define ID_BTN_REM_WEB 8
#define ID_BTN_ADD_RUNNING 10
#define ID_BTN_USER_GUIDE 108
#define ID_BTN_POMODORO 109
#define ID_BTN_EYE_CURE 110
#define ID_BTN_UPGRADE 111
#define ID_BTN_SUBMIT_UPGRADE 301
#define ID_CHK_ADBLOCK 112
#define ID_BTN_LIVE_CHAT 113
#define ID_BTN_SEND_CHAT 302
#define ID_BTN_SAVE_PROFILE 114
#define ID_CHK_REELS 115
#define ID_CHK_SHORTS 116
#define ID_BTN_CLOSE_BROADCAST 117
#define ID_BTN_START_POMO 401
#define ID_BTN_STOP_POMO 402
#define ID_BTN_STOPWATCH 118

// Globals
HWND hMainWnd, hOverlay, hDimFilter, hWarmFilter, hEyePanel, hUpgradePanel, hLiveChatPanel, hBroadcastPanel, hPomodoroPanel, hExpiredPanel, hStopwatchWnd;
HWND hLblTimeLeft, hPassEdit, hTimeHrEdit, hTimeMinEdit, hBtnStart, hBtnStopFocus, hBtnUserGuide, hBtnEyeCure, hBtnPomodoro, hBtnStopwatch, hChkAdBlock, hChkReels, hChkShorts, hBtnUpgrade, hLblTrialStatus, hLblAdminMsg;
HWND hEditProfileName, hChatLogEdit, hChatInputEdit, hEditEmail, hEditPhone, hEditTrx, hComboPkg, hLblFocusStatus;
HWND hRadioBlock, hRadioAllow, hAppInput, hComboApp, hBtnAddApp, hAppList, hBtnRemApp, hWebInput, hComboWeb, hBtnAddWeb, hWebList, hBtnRemWeb, hAllowAppInput, hComboAllowApp, hBtnAddAllowApp, hAllowAppList, hBtnRemAllowApp, hAllowWebInput, hComboAllowWeb, hBtnAddAllowWeb, hAllowWebList, hBtnRemAllowWeb, hRunningList, hBtnAddFromRunning;
HWND hPomoMinEdit, hPomoSessionEdit, hLblPomoStatus;

// Stopwatch vars
HWND hSwTxt, hBtnSwStart, hBtnSwReset;
bool swRunning = false; DWORD swStart = 0, swElapsed = 0;

bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false, isOverlayVisible = false; 
bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false; 
bool isPomodoroMode = false, isPomodoroBreak = false;
bool userClosedExpired = false; 

int currentOverlayType = 1, eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;

string currentSessionPass = "", userProfileName = "", adminMessage = "", pendingAdminPass = "", lastAdminChat = "", currentBroadcastMsg = "";
string pendingAdminChatStr = ""; 
string pendingBroadcastMsg = ""; 
string lastSeenBroadcastMsg = ""; 
string safeAdminMsg = ""; 
bool isLicenseValid = false, isTrialExpired = false;
int trialDaysLeft = 7, pendingAdminCmd = 0; 
NOTIFYICONDATA nid;
HHOOK hKeyboardHook;
string globalKeyBuffer = "";

COLORREF colBg = RGB(248, 250, 252), colTextDark = RGB(15, 23, 42);
COLORREF colBtnBlue = RGB(37, 99, 235), colBtnGreen = RGB(16, 185, 129), colBtnRed = RGB(239, 68, 68), colBtnGray = RGB(100, 116, 139), colBtnTeal = RGB(13, 148, 136), colBtnPurple = RGB(139, 92, 246), colBtnGold = RGB(245, 158, 11);
COLORREF colOverlayAdultBg = RGB(9, 61, 31), colOverlayTimeBg = RGB(26, 37, 47), colOverlayText = RGB(255, 255, 255);
HBRUSH hbrBg; 

vector<string> blockedApps, blockedWebs, allowedApps, allowedWebs;
vector<string> systemApps = {"explorer.exe", "svchost.exe", "taskmgr.exe", "cmd.exe", "conhost.exe", "csrss.exe", "dwm.exe", "lsass.exe", "services.exe", "smss.exe", "wininit.exe", "winlogon.exe", "spoolsv.exe", "fontdrvhost.exe", "searchui.exe", "searchindexer.exe", "sihost.exe", "taskhostw.exe", "ctfmon.exe", "applicationframehost.exe", "system", "registry", "audiodg.exe", "searchapp.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "textinputhost.exe"};
vector<string> commonThirdPartyApps = {"chrome.exe", "msedge.exe", "firefox.exe", "brave.exe", "opera.exe", "vivaldi.exe", "yandex.exe", "safari.exe", "waterfox.exe", "code.exe", "pycharm64.exe", "python.exe", "idea64.exe", "studio64.exe", "vlc.exe", "telegram.exe", "whatsapp.exe", "discord.exe", "zoom.exe", "skype.exe", "obs64.exe", "steam.exe", "epicgameslauncher.exe", "winword.exe", "excel.exe", "powerpnt.exe", "notepad.exe", "spotify.exe"};
vector<string> explicitKeywords = {"porn", "xxx", "sex", "nude", "nsfw", "xvideos", "pornhub", "xnxx", "xhamster", "brazzers", "onlyfans", "playboy", "mia khalifa", "bhabi", "chudai", "bangla choti", "magi", "sexy"};
vector<string> safeBrowserTitles = {"new tab", "start", "blank page", "allowed websites focus mode", "loading", "untitled", "connecting", "pomodoro break", "premium upgrade"};
vector<wstring> islamicQuotes = { L"\"মুমিনদের বলুন, তারা যেন তাদের দৃষ্টি নত রাখে এবং তাদের যৌনাঙ্গর হেফাযত করে।\" - (সূরা আন-নূর: ৩০)", L"\"লজ্জাশীলতা কল্যাণ ছাড়া আর কিছুই বয়ে আনে না।\" - (সহীহ বুখারী)" };
vector<wstring> timeQuotes = { L"\"যারা সময়কে মূল্যায়ন করে না, সময়ও তাদেরকে মূল্যায়ন করে না।\" - এ.পি.জে. আবদুল কালাম" };
wstring currentDisplayQuote = L"";

extern void ToggleAdBlock(bool enable); 

void SetupAutoStart(); void CreateDesktopShortcut(); void SyncLiveTrackerToFirebase(); void SyncTogglesToFirebase(); void SyncProfileNameToFirebase(string name); void SyncPasswordToFirebase(string pass, bool isLocking); void RegisterDeviceToFirebase(string deviceId); void ValidateLicenseAndTrial(); void ApplyEyeFilters(); void SaveSessionData(); void LoadSessionData(); void ClearSessionData(); void UpdateTimerDisplay(); 
// EnforceFocusMode দুই ভাগে ভাগ করা হয়েছে ফাস্ট পারফরম্যান্সের জন্য
void EnforceFocusMode_Processes(); void EnforceFocusMode_Window(); 
void ShowPomodoroBreakPage(); void ShowAllowedWebsitesPage(); void SelectRandomQuote(int type); void CheckAlwaysOnAdultFilter(); void CloseActiveTabAndMinimize(HWND hBrowser); bool CheckMatch(string url, string sTitle); string GenerateDisplayURL(string url); string EnsureExe(string name); bool CheckSingleInstance(); string GetDeviceID(); void ShowUserGuide(); void LoadData(); void SaveData(); vector<string> GetAppListForUI(); void ToggleUIElements(bool enable); void UpdateMainUIState();

string GetSecretDir() {
    string dir = "C:\\ProgramData\\SysCache_Ras";
    CreateDirectoryA(dir.c_str(), NULL);
    SetFileAttributesA(dir.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "\\";
}
string GetSessionFilePath() { return GetSecretDir() + "session.dat"; }

int GetTotalDaysForPackage(string pkg) {
    if (pkg.find("1 Year") != string::npos) return 365;
    if (pkg.find("6 Months") != string::npos) return 180;
    return 7; 
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (!blockAdult) return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam); 
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam; char c = MapVirtualKey(kbdStruct->vkCode, MAPVK_VK_TO_CHAR);
        if (c >= 32 && c <= 126) {
            globalKeyBuffer += tolower(c); if (globalKeyBuffer.length() > 50) globalKeyBuffer.erase(0, 1);
            for (const auto& kw : explicitKeywords) {
                if (globalKeyBuffer.find(kw) != string::npos) {
                    globalKeyBuffer = ""; HWND hActive = GetForegroundWindow(); if(hActive) SendMessage(hActive, WM_CLOSE, 0, 0);
                    currentOverlayType = 1; SelectRandomQuote(1); isOverlayVisible = true;
                    ShowWindow(hOverlay, SW_SHOWNOACTIVATE); SetWindowPos(hOverlay, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    SetTimer(hOverlay, 2, 6000, NULL); InvalidateRect(hOverlay, NULL, TRUE); break;
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

bool CheckSingleInstance() {
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "RasFocusPro_Mutex_V55");
    if (GetLastError() == ERROR_ALREADY_EXISTS) { HWND hExisting = FindWindowA("FocusApp", NULL); if (hExisting) { PostMessage(hExisting, WM_WAKEUP, 0, 0); } return false; }
    return true;
}

void CreateDesktopShortcut() {
    char exePath[MAX_PATH]; GetModuleFileName(NULL, exePath, MAX_PATH);
    char desktopPath[MAX_PATH]; SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);
    string shortcutPath = string(desktopPath) + "\\RasFocus Pro.lnk";
    
    ifstream f(shortcutPath.c_str());
    if (!f.good()) {
        CoInitialize(NULL); IShellLink* psl;
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl))) {
            psl->SetPath(exePath); psl->SetDescription("Launch RasFocus Pro");
            psl->SetIconLocation(exePath, 0); 
            IPersistFile* ppf;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf))) {
                WCHAR wsz[MAX_PATH]; MultiByteToWideChar(CP_ACP, 0, shortcutPath.c_str(), -1, wsz, MAX_PATH); ppf->Save(wsz, TRUE); ppf->Release();
            } psl->Release();
        } CoUninitialize();
    }
}

string GetDeviceID() {
    char compName[MAX_COMPUTERNAME_LENGTH + 1]; DWORD size = sizeof(compName); GetComputerNameA(compName, &size);
    DWORD volSerial = 0; GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char id[256]; sprintf(id, "%s-%X", compName, volSerial); return string(id);
}

void SelectRandomQuote(int type) { srand((unsigned int)time(0)); if (type == 1) currentDisplayQuote = islamicQuotes[rand() % islamicQuotes.size()]; else currentDisplayQuote = timeQuotes[rand() % timeQuotes.size()]; }

void CloseActiveTabAndMinimize(HWND hBrowser) { 
    if (GetForegroundWindow() == hBrowser) {
        keybd_event(VK_CONTROL,0,0,0); keybd_event('W',0,0,0); 
        keybd_event('W',0,KEYEVENTF_KEYUP,0); keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0); 
        Sleep(50); 
    }
    ShowWindow(hBrowser, SW_MINIMIZE); 
}

void UpdateMainUIState() {
    if (isSessionActive) {
        EnableWindow(hBtnStart, FALSE); 
        EnableWindow(hBtnStopFocus, TRUE);
        SetWindowText(hLblFocusStatus, "Focus is Active. For deactivate write password and click ok or contact developer in live chat.");
        InvalidateRect(hMainWnd, NULL, TRUE); 
    } else {
        EnableWindow(hBtnStart, TRUE); 
        EnableWindow(hBtnStopFocus, FALSE);
        SetWindowText(hLblFocusStatus, "");
        InvalidateRect(hMainWnd, NULL, TRUE);
    }
}

bool HandleSettingsWarning() {
    if (isSessionActive) {
        MessageBox(hMainWnd, "You cannot change settings while Focus is active.", "Warning", MB_ICONWARNING);
        return true;
    }
    return false;
}

void ToggleUIElements(bool enable) {
    if (isSessionActive && enable) return; 
    if (!isSessionActive && !enable) return;

    EnableWindow(hRadioBlock, enable); EnableWindow(hRadioAllow, enable); 
    EnableWindow(hBtnPomodoro, enable);
    
    bool eBlock = enable && !useAllowMode;
    bool eAllow = enable && useAllowMode;
    
    EnableWindow(hAppInput, eBlock); EnableWindow(hComboApp, eBlock); EnableWindow(hBtnAddApp, eBlock); EnableWindow(hBtnRemApp, eBlock);
    EnableWindow(hWebInput, eBlock); EnableWindow(hComboWeb, eBlock); EnableWindow(hBtnAddWeb, eBlock); EnableWindow(hBtnRemWeb, eBlock);
    
    EnableWindow(hAllowAppInput, eAllow); EnableWindow(hComboAllowApp, eAllow); EnableWindow(hBtnAddAllowApp, eAllow); EnableWindow(hBtnRemAllowApp, eAllow);
    EnableWindow(hAllowWebInput, eAllow); EnableWindow(hComboAllowWeb, eAllow); EnableWindow(hBtnAddAllowWeb, eAllow); EnableWindow(hBtnRemAllowWeb, eAllow);

    UpdateMainUIState();
}

void SaveSessionData() { 
    ofstream f(GetSessionFilePath()); 
    if(f.is_open()) { f << (isSessionActive?1:0) << endl << (isTimeMode?1:0) << endl << (isPassMode?1:0) << endl << currentSessionPass << endl << focusTimeTotalSeconds << endl << timerTicks << endl << (useAllowMode?1:0) << endl << (isPomodoroMode?1:0) << endl << (isPomodoroBreak?1:0) << endl << pomoTicks << endl << eyeBrightness << endl << eyeWarmth << endl << (blockReels?1:0) << endl << (blockShorts?1:0) << endl << (isAdblockActive?1:0) << endl << pomoCurrentSession << endl << userProfileName << endl; f.close(); } 
}

void LoadSessionData() { 
    ifstream f(GetSessionFilePath()); 
    if(f.is_open()) { 
        int a=0, tm=0, pm=0, ua=0, po=0, pb=0, br=0, bs=0, ad=0, pc=1; 
        f >> a >> tm >> pm >> currentSessionPass >> focusTimeTotalSeconds >> timerTicks >> ua >> po >> pb >> pomoTicks; 
        if(f >> eyeBrightness >> eyeWarmth >> br >> bs >> ad >> pc) { blockReels=(br==1); blockShorts=(bs==1); isAdblockActive=(ad==1); pomoCurrentSession = pc; } 
        else { eyeBrightness=100; eyeWarmth=0; blockReels=false; blockShorts=false; isAdblockActive=false; pomoCurrentSession=1; }
        f >> ws; getline(f, userProfileName); 
        if(a==1){isSessionActive=true; isTimeMode=(tm==1); isPassMode=(pm==1); useAllowMode=(ua==1); isPomodoroMode=(po==1); isPomodoroBreak=(pb==1); ToggleUIElements(false); } f.close(); 
    } 
}

void ClearSessionData() { 
    isSessionActive=false; isTimeMode=false; isPassMode=false; isPomodoroMode=false; isPomodoroBreak=false; 
    currentSessionPass=""; focusTimeTotalSeconds=0; timerTicks=0; pomoTicks=0; pomoCurrentSession=1; 
    SaveSessionData(); ToggleUIElements(true); SetWindowText(hLblTimeLeft, ""); 
    if(hMainWnd) KillTimer(hMainWnd, 1);
}

void SetupAutoStart() { 
    char p[MAX_PATH]; 
    GetModuleFileNameA(NULL, p, MAX_PATH); 
    string pathWithArg = "\"" + string(p) + "\" -autostart";
    
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "RasFocusPro", 0, REG_SZ, (const BYTE*)pathWithArg.c_str(), pathWithArg.length() + 1);
        RegCloseKey(hKey);
    }
}

bool CheckMatch(string url, string sTitle) { string t=sTitle; t.erase(remove_if(t.begin(), t.end(), ::isspace), t.end()); string s=url; transform(s.begin(), s.end(), s.begin(), ::tolower); string exact=s; exact.erase(remove_if(exact.begin(), exact.end(), ::isspace), exact.end()); if (!exact.empty() && t.find(exact) != string::npos) return true; replace(s.begin(), s.end(), '.', ' '); replace(s.begin(), s.end(), '/', ' '); replace(s.begin(), s.end(), ':', ' '); replace(s.begin(), s.end(), '-', ' '); stringstream ss(s); string word; while(ss >> word) { if (word=="https"||word=="http"||word=="www"||word=="com"||word=="org"||word=="net"||word=="html"||word=="github") continue; if (word.length()>=3 && t.find(word) != string::npos) return true; } return false; }
string GenerateDisplayURL(string url) { string s=url; string e[]={"https://","http://","www.","/*"}; for(const string& p:e){ size_t pos=s.find(p); if(pos!=string::npos) s.erase(pos,p.length()); } return s; }
string EnsureExe(string n) { if(n.length()<4 || n.substr(n.length()-4)!=".exe") return n+".exe"; return n; }

struct WindowProcData { vector<string> apps; };
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) { if(!IsWindowVisible(hwnd)) return TRUE; if(GetWindowTextLength(hwnd)==0) return TRUE; DWORD pid=0; GetWindowThreadProcessId(hwnd, &pid); if(pid==0 || pid==GetCurrentProcessId()) return TRUE; HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid); if(h){ char n[MAX_PATH]; DWORD s=MAX_PATH; if(QueryFullProcessImageNameA(h,0,n,&s)){ string p=n; string e=p.substr(p.find_last_of("\\/")+1); transform(e.begin(), e.end(), e.begin(), ::tolower); if(e!="explorer.exe"&&e!="applicationframehost.exe"&&e!="textinputhost.exe"){ WindowProcData* d=(WindowProcData*)lParam; if(find(d->apps.begin(), d->apps.end(), e)==d->apps.end()) d->apps.push_back(e); } } CloseHandle(h); } return TRUE; }
vector<string> GetAppListForUI() { WindowProcData d; EnumWindows(EnumWindowsProc, (LPARAM)&d); for(const auto& a:commonThirdPartyApps){ if(find(d.apps.begin(), d.apps.end(), a)==d.apps.end()) d.apps.push_back(a); } sort(d.apps.begin(), d.apps.end()); return d.apps; }
void LoadData() { auto l=[](string f, vector<string>& v){ ifstream i(GetSecretDir() + f); string s; while(getline(i,s)){ if(!s.empty()) v.push_back(s); } i.close(); }; l("bl_app.dat", blockedApps); l("bl_web.dat", blockedWebs); l("al_app.dat", allowedApps); l("al_web.dat", allowedWebs); }
void SaveData() { auto s=[](string f, const vector<string>& v){ ofstream o(GetSecretDir() + f); for(const auto& i:v) o<<i<<endl; o.close(); }; s("bl_app.dat", blockedApps); s("bl_web.dat", blockedWebs); s("al_app.dat", allowedApps); s("al_web.dat", allowedWebs); }

void ApplyEyeFilters() {
    int dimAlpha = (100 - eyeBrightness) * 2.55; if (dimAlpha < 0) dimAlpha = 0; if (dimAlpha > 240) dimAlpha = 240; 
    if (dimAlpha > 0) { ShowWindow(hDimFilter, SW_SHOWNOACTIVATE); SetLayeredWindowAttributes(hDimFilter, 0, dimAlpha, LWA_ALPHA); } else { ShowWindow(hDimFilter, SW_HIDE); }
    int warmAlpha = eyeWarmth * 1.5; if (warmAlpha < 0) warmAlpha = 0; if (warmAlpha > 180) warmAlpha = 180;
    if (warmAlpha > 0) { ShowWindow(hWarmFilter, SW_SHOWNOACTIVATE); SetLayeredWindowAttributes(hWarmFilter, 0, warmAlpha, LWA_ALPHA); } else { ShowWindow(hWarmFilter, SW_HIDE); }
}

void SyncProfileNameToFirebase(string name) {
    string deviceId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
}

void SyncPasswordToFirebase(string pass, bool isLocking) {
    string deviceId = GetDeviceID(); string val = isLocking ? pass : ""; string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=livePassword&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ livePassword = @{ stringValue = '" + val + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
}

void SyncLiveTrackerToFirebase() {
    string deviceId = GetDeviceID(); string mode = "None"; string timeL = "00:00"; string activeStr = isSessionActive ? "$true" : "$false";
    if (isSessionActive) {
        if(isPomodoroMode) { mode = "Pomodoro"; int l = (pomoLengthMin*60) - pomoTicks; if(isPomodoroBreak) l = (2*60) - pomoTicks; if(l<0) l=0; char buf[20]; sprintf(buf, "%02d:%02d", l/60, l%60); timeL = buf; }
        else if(isTimeMode) { mode = "Timer"; int l = focusTimeTotalSeconds - timerTicks; if(l<0) l=0; char buf[20]; sprintf(buf, "%02d:%02d", l/60, l%60); timeL = buf; }
        else if(isPassMode) { mode = "Password"; timeL = "Manual Lock"; }
    }
    string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=isSelfControlActive&updateMask.fieldPaths=activeModeType&updateMask.fieldPaths=timeRemaining&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ isSelfControlActive = @{ booleanValue = " + activeStr + " }; activeModeType = @{ stringValue = '" + mode + "' }; timeRemaining = @{ stringValue = '" + timeL + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
}

void SyncTogglesToFirebase() {
    string deviceId = GetDeviceID(); string bR = blockReels ? "$true" : "$false"; string bS = blockShorts ? "$true" : "$false"; string bA = isAdblockActive ? "$true" : "$false";
    string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
}

void RegisterDeviceToFirebase(string deviceId) {
    string params = "-WindowStyle Hidden -Command \"$url='https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY'; $body = @{ fields = @{ deviceID = @{ stringValue = '" + deviceId + "' }; status = @{ stringValue = 'TRIAL' }; package = @{ stringValue = '7 Days Trial' }; adminMessage = @{ stringValue = '' }; adminCmd = @{ stringValue = 'NONE' }; adminPass = @{ stringValue = '' }; liveChatAdmin = @{ stringValue = '' }; liveChatUser = @{ stringValue = '' }; livePassword = @{ stringValue = '' }; profileName = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri $url -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
}

void ValidateLicenseAndTrial() {
    string deviceId = GetDeviceID(); 
    
    HINTERNET hInternet = InternetOpenA("RasFocus", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        DWORD timeout = 4000;
        InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        
        string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect) {
            char buffer[1024]; DWORD bytesRead; string response = "";
            while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; }
            InternetCloseHandle(hConnect);

            string fbPackage = "7 Days Trial";
            size_t pkgPos = response.find("\"package\"");
            if (pkgPos != string::npos) {
                size_t valPos = response.find("\"stringValue\": \"", pkgPos);
                if (valPos != string::npos) {
                    valPos += 16; size_t endPos = response.find("\"", valPos);
                    if (endPos != string::npos) fbPackage = response.substr(valPos, endPos - valPos);
                }
            }

            string trialFile = GetSecretDir() + "sys_lic.dat"; 
            ifstream in(trialFile); 
            time_t activationTime = 0;
            string savedPackage = "7 Days Trial";
            
            if (in >> activationTime) { 
                getline(in, savedPackage);
                if (!savedPackage.empty() && savedPackage[0] == ' ') savedPackage = savedPackage.substr(1);
            } else { 
                activationTime = time(0); 
                savedPackage = fbPackage;
                ofstream out(trialFile); out << activationTime << " " << savedPackage; out.close(); 
                RegisterDeviceToFirebase(deviceId); 
            }

            if (!fbPackage.empty() && fbPackage != savedPackage) {
                activationTime = time(0);
                savedPackage = fbPackage;
                ofstream out(trialFile); out << activationTime << " " << savedPackage; out.close();
            }

            int totalDays = GetTotalDaysForPackage(savedPackage);
            double daysPassed = difftime(time(0), activationTime) / 86400.0;
            trialDaysLeft = totalDays - (int)daysPassed;

            bool explicitlyRevoked = (response.find("\"stringValue\": \"REVOKED\"") != string::npos);
            bool explicitlyApproved = (response.find("\"stringValue\": \"APPROVED\"") != string::npos);

            if (explicitlyRevoked) { 
                isLicenseValid = false; isTrialExpired = true; trialDaysLeft = 0; 
            } else { 
                if (trialDaysLeft <= 0) {
                    isTrialExpired = true; trialDaysLeft = 0; isLicenseValid = false;
                } else {
                    isTrialExpired = false;
                    isLicenseValid = explicitlyApproved; 
                }
            }

            auto parseBool = [&](string fName, bool defaultVal) {
                size_t pos = response.find("\"" + fName + "\"");
                if(pos != string::npos) { size_t vPos = response.find("\"booleanValue\":", pos); if(vPos != string::npos) { if(response.find("true", vPos) < response.find("}", vPos)) return true; if(response.find("false", vPos) < response.find("}", vPos)) return false; } }
                return defaultVal;
            };
            
            bool newAdult = parseBool("adultBlock", blockAdult);
            bool newReels = parseBool("fbReelsBlock", blockReels);
            bool newShorts = parseBool("ytShortsBlock", blockShorts);
            bool newAd = parseBool("adBlock", isAdblockActive);
            
            bool saveRequired = false;
            if(newAdult != blockAdult) blockAdult = newAdult;
            if(newReels != blockReels) { blockReels = newReels; saveRequired = true; }
            if(newShorts != blockShorts) { blockShorts = newShorts; saveRequired = true; }
            if(newAd != isAdblockActive) { isAdblockActive = newAd; saveRequired = true; ToggleAdBlock(isAdblockActive); }
            if(saveRequired) SaveSessionData();

            size_t pNamePos = response.find("\"profileName\""); if (pNamePos != string::npos) { size_t valPos = response.find("\"stringValue\": \"", pNamePos); if (valPos != string::npos) { valPos += 16; size_t endPos = response.find("\"", valPos); if(endPos != string::npos) userProfileName = response.substr(valPos, endPos - valPos); } }
            
            size_t msgPos = response.find("\"adminMessage\""); 
            if (msgPos != string::npos) { 
                size_t valPos = response.find("\"stringValue\": \"", msgPos); 
                if (valPos != string::npos) { 
                    valPos += 16; size_t endPos = response.find("\"", valPos); 
                    if(endPos != string::npos) safeAdminMsg = response.substr(valPos, endPos - valPos); 
                } 
            }

            size_t cmdPos = response.find("\"adminCmd\"");
            if (cmdPos != string::npos) {
                size_t vPos = response.find("\"stringValue\": \"", cmdPos);
                if (vPos != string::npos) {
                    vPos += 16; size_t ePos = response.find("\"", vPos); string cmd = response.substr(vPos, ePos - vPos);
                    if (cmd == "START_FOCUS" && !isSessionActive) { string aPass = "12345"; size_t pPos = response.find("\"adminPass\""); if (pPos != string::npos) { size_t pvPos = response.find("\"stringValue\": \"", pPos); if (pvPos != string::npos) { pvPos += 16; size_t pePos = response.find("\"", pvPos); aPass = response.substr(pvPos, pePos - pvPos); } } pendingAdminPass = aPass; pendingAdminCmd = 1; }
                    else if (cmd == "STOP_FOCUS" && isSessionActive) { pendingAdminCmd = 2; }
                }
            }
            
            size_t bcastPos = response.find("\"broadcastMsg\"");
            if (bcastPos != string::npos) {
                size_t vPos = response.find("\"stringValue\": \"", bcastPos);
                if (vPos != string::npos) { 
                    vPos += 16; size_t ePos = response.find("\"", vPos); 
                    string bMsg = response.substr(vPos, ePos - vPos);
                    if (!bMsg.empty() && bMsg != "ACK" && bMsg != lastSeenBroadcastMsg) { 
                        lastSeenBroadcastMsg = bMsg;
                        pendingBroadcastMsg = bMsg; 
                    }
                }
            }

            size_t chatPos = response.find("\"liveChatAdmin\"");
            if (chatPos != string::npos) {
                size_t cvPos = response.find("\"stringValue\": \"", chatPos);
                if (cvPos != string::npos) { cvPos += 16; size_t cePos = response.find("\"", cvPos); string adminChatStr = response.substr(cvPos, cePos - cvPos);
                    if (!adminChatStr.empty() && adminChatStr != lastAdminChat) { 
                        lastAdminChat = adminChatStr; 
                        pendingAdminChatStr = adminChatStr; 
                    }
                }
            }
        } InternetCloseHandle(hInternet);
    } else { isLicenseValid = !isTrialExpired; }
}

DWORD WINAPI LicenseCheckThread(LPVOID lpParam) {
    while(true) {
        Sleep(4000); 
        ValidateLicenseAndTrial();
    } 
    return 0;
}

void UpdateTimerDisplay() {
    char tip[128] = "Focus Mode Manager";
    if (isSessionActive) {
        if (isTrialExpired) { strcpy_s(tip, "Session PAUSED"); }
        else if (isPomodoroMode) { 
            if (isPomodoroBreak) { int left=(2*60)-pomoTicks; if(left<0) left=0; sprintf(tip, "Break: %02d:%02d", left/60, left%60); } 
            else { int left=(pomoLengthMin*60)-pomoTicks; if(left<0) left=0; sprintf(tip, "Focus (%d/%d): %02d:%02d", pomoCurrentSession, pomoTotalSessions, left/60, left%60); } 
        } 
        else if (isTimeMode) { int l=focusTimeTotalSeconds-timerTicks; if(l<0) l=0; sprintf(tip, "Time Left: %02d:%02d:%02d", l/3600, (l%3600)/60, l%60); } 
        else { strcpy_s(tip, "Focus Active"); }
    }
    strcpy_s(nid.szTip, tip); Shell_NotifyIcon(NIM_MODIFY, &nid); 
    if (hLblTimeLeft) SetWindowText(hLblTimeLeft, tip);
    if (hMainWnd) SetWindowText(hMainWnd, tip);
    if (isPomodoroMode && hLblPomoStatus) SetWindowText(hLblPomoStatus, tip);
}

void ShowPomodoroBreakPage() {
    char p[MAX_PATH]; GetCurrentDirectory(MAX_PATH, p); string hPath=string(p)+"\\pomodoro_break.html"; ofstream html(hPath);
    html<<"<!DOCTYPE html><html><head><style>body{margin:0;height:100vh;display:flex;flex-direction:column;justify-content:center;align-items:center;background:linear-gradient(to bottom, #1e3c72, #2a5298);color:white;font-family:'Segoe UI',sans-serif;}h1{font-size:50px;margin-bottom:10px;}p{font-size:20px;color:#a0c4ff;}</style></head><body><h1>Time to Relax & Drink Water!</h1><p>2 Minutes Break Started.</p></body></html>";
    html.close(); ShellExecute(NULL, "open", hPath.c_str(), NULL, NULL, SW_SHOWMAXIMIZED);
}

void ShowAllowedWebsitesPage() {
    static DWORD lastTime = 0; if (GetTickCount()-lastTime<3000) return; lastTime=GetTickCount();
    string hPath = GetSecretDir() + "allowed_sites.html"; ofstream html(hPath);
    html<<"<!DOCTYPE html><html><head><style>body{font-family:sans-serif;text-align:center;}a{background:#007bff;color:white;padding:10px;margin:5px;display:inline-block;}</style></head><body><h2>Focus Active! Allowed:</h2>";
    for (const auto& w:allowedWebs) html<<"<a href='https://"<<GenerateDisplayURL(w)<<"' target='_blank'>"<<w<<"</a>"; html<<"</body></html>"; html.close(); ShellExecute(NULL, "open", hPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------------------------------
// পরিবর্তন ৩: EnforceFocusMode কে দুই ভাগে ভাগ করা হয়েছে।
// অ্যাপ ব্লক করার কাজ হবে ১ সেকেন্ড পর পর। 
// ---------------------------------------------------------
void EnforceFocusMode_Processes() {
    if (!isSessionActive || isTrialExpired || (isPomodoroMode && isPomodoroBreak)) return; 
    HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32 pe={sizeof(pe)}; DWORD myPid=GetCurrentProcessId();
    if(Process32First(h,&pe)){ do{
        if(pe.th32ProcessID==myPid) continue; string n=pe.szExeFile; transform(n.begin(), n.end(), n.begin(), ::tolower);
        
        if(n=="taskmgr.exe" || n=="msiexec.exe" || n=="setup.exe" || n=="install.exe" || n=="installer.exe"){ 
            HANDLE p_term=OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); 
            if(p_term){TerminateProcess(p_term,1);CloseHandle(p_term);} 
            continue;
        }
        
        if(useAllowMode){
            bool isSys=(find(systemApps.begin(), systemApps.end(), n)!=systemApps.end()); bool isAll=false;
            for(const auto& a:allowedApps){ string la=EnsureExe(a); transform(la.begin(), la.end(), la.begin(), ::tolower); if(n==la){isAll=true;break;} }
            bool isCommonBrowser = (n=="chrome.exe"||n=="msedge.exe"||n=="firefox.exe"||n=="brave.exe"||n=="opera.exe"||n=="vivaldi.exe"||n=="yandex.exe"||n=="safari.exe"||n=="waterfox.exe"); 
            if(!isSys && !isAll && !isCommonBrowser){ HANDLE p_term=OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(p_term){TerminateProcess(p_term,1);CloseHandle(p_term);} }
        } else {
            for(const auto& a:blockedApps){ string la=EnsureExe(a); transform(la.begin(), la.end(), la.begin(), ::tolower); if(n==la){ HANDLE p_term=OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(p_term){TerminateProcess(p_term,1);CloseHandle(p_term);} } }
        }
    } while(Process32Next(h,&pe)); } CloseHandle(h);
}

// ---------------------------------------------------------
// পরিবর্তন ৪: শুধুমাত্র বর্তমান উইন্ডো (Browser Tab) ব্লকের জন্য ফাস্ট লজিক।
// ---------------------------------------------------------
void EnforceFocusMode_Window() {
    if (!isSessionActive || isTrialExpired || (isPomodoroMode && isPomodoroBreak)) return; 
    if(isOverlayVisible) return;
    
    HWND hActive = GetForegroundWindow();
    if(hActive && hActive != hOverlay) {
        char title[512], cls[256]; GetClassName(hActive, cls, sizeof(cls));
        if(GetWindowText(hActive, title, sizeof(title))>0){
            string sTitle=title; transform(sTitle.begin(), sTitle.end(), sTitle.begin(), ::tolower); string sClass=cls; string sClassLower=sClass; transform(sClassLower.begin(), sClassLower.end(), sClassLower.begin(), ::tolower);
            if(sClass=="#32770" && sTitle=="run") { SendMessage(hActive, WM_CLOSE, 0, 0); return; }
            if(sTitle.find("appdata")!=string::npos || sTitle.find("roaming")!=string::npos || sTitle.find("programdata")!=string::npos) { SendMessage(hActive, WM_CLOSE, 0, 0); return; } 
            bool isSafe=false; for(const auto& s:safeBrowserTitles){ if(sTitle.find(s)!=string::npos){isSafe=true;break;} } if(isSafe) return;
            
            DWORD activePid; GetWindowThreadProcessId(hActive, &activePid); HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, activePid); string activeExe = "";
            if(ph){ char ex[MAX_PATH]; DWORD sz=MAX_PATH; if(QueryFullProcessImageNameA(ph,0,ex,&sz)){ string p=ex; activeExe=p.substr(p.find_last_of("\\/")+1); transform(activeExe.begin(), activeExe.end(), activeExe.begin(), ::tolower); } CloseHandle(ph); }
            
            bool isBrowser = (activeExe=="chrome.exe" || activeExe=="msedge.exe" || activeExe=="firefox.exe" || activeExe=="brave.exe" || activeExe=="opera.exe" || activeExe=="vivaldi.exe" || activeExe=="yandex.exe" || activeExe=="safari.exe" || activeExe=="waterfox.exe" || sClassLower.find("chrome")!=string::npos || sClassLower.find("mozilla")!=string::npos || sClassLower.find("msedge")!=string::npos);
            
            if(useAllowMode && isBrowser){
                bool isAll=false; for(const auto& w:allowedWebs){ if(CheckMatch(w, sTitle)){isAll=true;break;} }
                if(!isAll){ CloseActiveTabAndMinimize(hActive); ShowAllowedWebsitesPage(); }
            } else if(!useAllowMode){
                for(const auto& w:blockedWebs){ if(CheckMatch(w, sTitle)){ CloseActiveTabAndMinimize(hActive); currentOverlayType=2; SelectRandomQuote(2); isOverlayVisible=true; ShowWindow(hOverlay, SW_SHOWNOACTIVATE); SetWindowPos(hOverlay, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); SetTimer(hOverlay, 2, 4000, NULL); InvalidateRect(hOverlay, NULL, TRUE); break; } }
            }
        }
    }
}

void CheckAlwaysOnAdultFilter() {
    if (isOverlayVisible) return; 
    if (!blockAdult && !blockReels && !blockShorts) return;

    HWND hActive = GetForegroundWindow();
    if (hActive && hActive != hOverlay) {
        char title[512];
        if (GetWindowText(hActive, title, sizeof(title)) > 0) {
            string sTitle = title; transform(sTitle.begin(), sTitle.end(), sTitle.begin(), ::tolower);
            bool blocked = false;
            
            if (blockAdult) {
                for (const auto& keyword : explicitKeywords) { if (sTitle.find(keyword) != string::npos) { blocked = true; break; } }
            }
            
            if (!blocked && blockReels && sTitle.find("facebook") != string::npos && sTitle.find("reels") != string::npos) blocked = true;
            if (!blocked && blockShorts && sTitle.find("youtube") != string::npos && sTitle.find("shorts") != string::npos) blocked = true;
            
            if (blocked) { CloseActiveTabAndMinimize(hActive); currentOverlayType = 1; SelectRandomQuote(1); isOverlayVisible = true; ShowWindow(hOverlay, SW_SHOWNOACTIVATE); SetWindowPos(hOverlay, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); SetTimer(hOverlay, 2, 8000, NULL); InvalidateRect(hOverlay, NULL, TRUE); return; }
        }
    }
}

LRESULT CALLBACK StopwatchProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            HFONT hFontBig = CreateFont(60,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Consolas");
            hSwTxt = CreateWindow("STATIC", "00:00:00.00", WS_VISIBLE|WS_CHILD|SS_CENTER, 0, 50, 400, 70, hwnd, NULL, NULL, NULL);
            SendMessage(hSwTxt, WM_SETFONT, (WPARAM)hFontBig, TRUE);
            hBtnSwStart = CreateWindow("BUTTON", "START / PAUSE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 50, 150, 140, 45, hwnd, (HMENU)1, NULL, NULL);
            hBtnSwReset = CreateWindow("BUTTON", "RESET", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 210, 150, 140, 45, hwnd, (HMENU)2, NULL, NULL);
            SetTimer(hwnd, 10, 30, NULL); return 0;
        }
        case WM_TIMER: {
            if(wParam == 10 && swRunning) {
                swElapsed = GetTickCount() - swStart;
                DWORD totalS = swElapsed / 1000; DWORD ms = (swElapsed % 1000) / 10;
                DWORD h = totalS / 3600; DWORD m = (totalS % 3600) / 60; DWORD s = totalS % 60;
                char buf[32]; sprintf(buf, "%02d:%02d:%02d.%02d", h, m, s, ms); SetWindowText(hSwTxt, buf);
            } break;
        }
        case WM_DRAWITEM: { 
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; COLORREF c = colBtnGray; 
            if(p->CtlID == 1) c = swRunning ? colBtnRed : colBtnGreen; 
            HBRUSH br=CreateSolidBrush(c); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 8, 8); 
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE; 
        }
        case WM_CTLCOLORSTATIC: { SetBkMode((HDC)wParam, TRANSPARENT); SetTextColor((HDC)wParam, colTextDark); return (LRESULT)hbrBg; }
        case WM_COMMAND: {
            if(LOWORD(wParam)==1) { swRunning = !swRunning; if(swRunning) swStart = GetTickCount() - swElapsed; InvalidateRect(hwnd, NULL, TRUE); }
            if(LOWORD(wParam)==2) { swRunning = false; swElapsed = 0; SetWindowText(hSwTxt, "00:00:00.00"); InvalidateRect(hwnd, NULL, TRUE); }
            break;
        }
        case WM_CLOSE: { ShowWindow(hwnd, SW_HIDE); return 0; }
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ExpiredProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            CreateWindow("BUTTON", "CLOSE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 30, 160, 100, 40, hwnd, (HMENU)1001, NULL, NULL); 
            CreateWindow("BUTTON", "LIVE CHAT", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 150, 160, 150, 40, hwnd, (HMENU)1002, NULL, NULL); 
            CreateWindow("BUTTON", "UPGRADE NOW", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 320, 160, 150, 40, hwnd, (HMENU)1003, NULL, NULL); 
            return 0; 
        }
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd, &ps); RECT r; GetClientRect(hwnd, &r); HBRUSH br=CreateSolidBrush(RGB(30, 41, 59)); FillRect(hdc, &r, br); DeleteObject(br);
            SetTextColor(hdc, RGB(239, 68, 68)); SetBkMode(hdc, TRANSPARENT); HFONT f=CreateFontW(28,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
            SelectObject(hdc, f); r.top+=40; DrawTextA(hdc, "LICENSE / SESSION PAUSED", -1, &r, DT_CENTER); DeleteObject(f); 
            SetTextColor(hdc, RGB(255, 255, 255)); f=CreateFontW(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
            SelectObject(hdc, f); r.top+=40; DrawTextA(hdc, "Your package has expired or Admin revoked your license.\nPlease upgrade or contact admin to continue using RasFocus Pro.", -1, &r, DT_CENTER); DeleteObject(f); 
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_DRAWITEM: { 
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam); 
            COLORREF c = colBtnGray;
            if(p->CtlID == 1002) c = RGB(236, 72, 153);
            if(p->CtlID == 1003) c = colBtnGreen;
            HBRUSH br=CreateSolidBrush(c); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10); char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE; 
        }
        case WM_COMMAND: {
            if(LOWORD(wParam)==1001) { userClosedExpired = true; ShowWindow(hwnd, SW_HIDE); }
            if(LOWORD(wParam)==1002) { userClosedExpired = true; ShowWindow(hwnd, SW_HIDE); if(hLiveChatPanel){ ShowWindow(hLiveChatPanel, SW_SHOW); SetForegroundWindow(hLiveChatPanel); } }
            if(LOWORD(wParam)==1003) { userClosedExpired = true; ShowWindow(hwnd, SW_HIDE); if(hUpgradePanel){ ShowWindow(hUpgradePanel, SW_SHOW); SetForegroundWindow(hUpgradePanel); } }
            break;
        }
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PomodoroProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(16,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");
            CreateWindow("STATIC", "Focus Length (Min):", WS_VISIBLE|WS_CHILD, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);
            hPomoMinEdit = CreateWindow("EDIT", "25", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, 180, 20, 100, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Total Sessions:", WS_VISIBLE|WS_CHILD, 20, 60, 150, 20, hwnd, NULL, NULL, NULL);
            hPomoSessionEdit = CreateWindow("EDIT", "4", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, 180, 60, 100, 25, hwnd, NULL, NULL, NULL);
            hLblPomoStatus = CreateWindow("STATIC", "Status: Ready", WS_VISIBLE|WS_CHILD, 20, 110, 260, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("BUTTON", "START POMODORO", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 20, 150, 160, 40, hwnd, (HMENU)ID_BTN_START_POMO, NULL, NULL);
            CreateWindow("BUTTON", "STOP", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 190, 150, 90, 40, hwnd, (HMENU)ID_BTN_STOP_POMO, NULL, NULL);
            EnumChildWindows(hwnd, [](HWND c, LPARAM l)->BOOL{SendMessage(c,WM_SETFONT,l,TRUE);return TRUE;}, (LPARAM)hFont); return 0;
        }
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wParam; HWND hw = (HWND)lParam; SetBkMode(hdc, TRANSPARENT); if (hw == hLblPomoStatus) SetTextColor(hdc, colBtnRed); return (LRESULT)hbrBg; }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam);
            HBRUSH br=CreateSolidBrush(p->CtlID==ID_BTN_START_POMO?colBtnGreen:colBtnRed); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10);
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE;
        }
        case WM_COMMAND: {
            if(LOWORD(wParam)==ID_BTN_START_POMO && !isSessionActive && !isTrialExpired){
                char m[10], s[10]; GetWindowText(hPomoMinEdit,m,10); GetWindowText(hPomoSessionEdit,s,10);
                pomoLengthMin = atoi(m); pomoTotalSessions = atoi(s);
                if(pomoLengthMin>0 && pomoTotalSessions>0){ 
                    isPomodoroMode=true; isSessionActive=true; pomoTicks=0; pomoCurrentSession=1; 
                    SaveSessionData(); ToggleUIElements(false); SyncPasswordToFirebase("", true); 
                    SetTimer(hMainWnd, 1, 1000, NULL); 
                    UpdateMainUIState();
                    MessageBox(hwnd, "Pomodoro Started!", "Success", MB_OK); 
                }
            }
            if(LOWORD(wParam)==ID_BTN_STOP_POMO && isPomodoroMode){ ClearSessionData(); UpdateMainUIState(); MessageBox(hwnd, "Pomodoro Stopped.", "Success", MB_OK); }
            break;
        }
        case WM_CLOSE: ShowWindow(hwnd, SW_HIDE); return 0;
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK LiveChatProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(17,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");
            CreateWindow("STATIC", "Live Chat", WS_VISIBLE|WS_CHILD|SS_CENTER, 20, 10, 340, 20, hwnd, NULL, NULL, NULL);
            hChatLogEdit = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY, 20, 35, 340, 320, hwnd, NULL, NULL, NULL);
            hChatInputEdit = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_AUTOHSCROLL|WS_TABSTOP, 20, 370, 260, 30, hwnd, NULL, NULL, NULL); 
            CreateWindow("BUTTON", "SEND", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 290, 370, 70, 30, hwnd, (HMENU)ID_BTN_SEND_CHAT, NULL, NULL);
            EnumChildWindows(hwnd, [](HWND c, LPARAM l)->BOOL{SendMessage(c,WM_SETFONT,l,TRUE);return TRUE;}, (LPARAM)hFont); return 0;
        }
        case WM_ACTIVATE: { if (LOWORD(wParam) != WA_INACTIVE) SetFocus(hChatInputEdit); return 0; }
        case WM_CTLCOLORSTATIC: SetBkMode((HDC)wParam, TRANSPARENT); return (LRESULT)hbrBg;
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam);
            HBRUSH br=CreateSolidBrush(colBtnBlue); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10);
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE;
        }
        case WM_COMMAND: {
            if(LOWORD(wParam)==ID_BTN_SEND_CHAT){
                char msgText[512]; GetWindowText(hChatInputEdit, msgText, 512);
                if (strlen(msgText) > 0) {
                    string displayMsg = "You: " + string(msgText) + "\r\n"; int len = GetWindowTextLength(hChatLogEdit); SendMessage(hChatLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len); SendMessage(hChatLogEdit, EM_REPLACESEL, 0, (LPARAM)displayMsg.c_str()); SetWindowText(hChatInputEdit, "");
                    string deviceId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
                    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + string(msgText) + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\""; SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
                } 
            } break;
        }
        case WM_CLOSE: ShowWindow(hwnd, SW_HIDE); return 0;
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK BroadcastProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: 
            CreateWindow("BUTTON", "CLOSE MESSAGE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 145, 165, 160, 35, hwnd, (HMENU)ID_BTN_CLOSE_BROADCAST, NULL, NULL); 
            return 0; 
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd, &ps); RECT r; GetClientRect(hwnd, &r); 
            HBRUSH br=CreateSolidBrush(RGB(15, 23, 42)); FillRect(hdc, &r, br); DeleteObject(br);
            
            SetTextColor(hdc, RGB(99, 102, 241)); 
            HFONT fHeader=CreateFontW(18,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
            SelectObject(hdc, fHeader); RECT rHeader = r; rHeader.top += 15; DrawTextA(hdc, "ADMIN BROADCAST", -1, &rHeader, DT_CENTER); DeleteObject(fHeader);

            SetTextColor(hdc, RGB(248, 250, 252)); SetBkMode(hdc, TRANSPARENT); 
            HFONT fMsg=CreateFontW(22,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, L"Segoe UI"); 
            SelectObject(hdc, fMsg); RECT rMsg = r; rMsg.top += 55; rMsg.bottom = 150; rMsg.left += 20; rMsg.right -= 20;
            DrawTextA(hdc, currentBroadcastMsg.c_str(), -1, &rMsg, DT_CENTER|DT_WORDBREAK); DeleteObject(fMsg); 
            
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_DRAWITEM: { 
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam); 
            HBRUSH br=CreateSolidBrush(RGB(220, 38, 38)); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10); 
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE; 
        }
        case WM_COMMAND: { 
            if(LOWORD(wParam)==ID_BTN_CLOSE_BROADCAST){ 
                ShowWindow(hwnd, SW_HIDE); string deviceId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=broadcastMsg&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; 
                string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ broadcastMsg = @{ stringValue = 'ACK' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\""; 
                SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei); 
            } break; 
        }
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK UpgradePanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(16,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");
            CreateWindow("STATIC", "Send payment via Nagad/bKash to 01566054963", WS_VISIBLE|WS_CHILD|SS_CENTER, 20, 15, 340, 20, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Email / Name:", WS_VISIBLE|WS_CHILD, 20, 45, 340, 20, hwnd, NULL, NULL, NULL); hEditEmail = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, 20, 65, 340, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Sender Number:", WS_VISIBLE|WS_CHILD, 20, 100, 340, 20, hwnd, NULL, NULL, NULL); hEditPhone = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, 20, 120, 340, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Transaction ID (TrxID):", WS_VISIBLE|WS_CHILD, 20, 155, 340, 20, hwnd, NULL, NULL, NULL); hEditTrx = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, 20, 175, 340, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("STATIC", "Select Package:", WS_VISIBLE|WS_CHILD, 20, 210, 340, 20, hwnd, NULL, NULL, NULL);
            hComboPkg = CreateWindow("COMBOBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST, 20, 230, 340, 100, hwnd, NULL, NULL, NULL);
            SendMessage(hComboPkg, CB_ADDSTRING, 0, (LPARAM)"7 Days Trial"); SendMessage(hComboPkg, CB_ADDSTRING, 0, (LPARAM)"6 Months (50 BDT)"); SendMessage(hComboPkg, CB_ADDSTRING, 0, (LPARAM)"1 Year (100 BDT)"); SendMessage(hComboPkg, CB_SETCURSEL, 0, 0);
            CreateWindow("BUTTON", "SUBMIT REQUEST", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 20, 280, 340, 35, hwnd, (HMENU)ID_BTN_SUBMIT_UPGRADE, NULL, NULL);
            EnumChildWindows(hwnd, [](HWND c, LPARAM l)->BOOL{SendMessage(c,WM_SETFONT,l,TRUE);return TRUE;}, (LPARAM)hFont); return 0;
        }
        case WM_CTLCOLORSTATIC: { SetBkMode((HDC)wParam, TRANSPARENT); return (LRESULT)hbrBg; }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam);
            HBRUSH br=CreateSolidBrush(colBtnGreen); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10);
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_SUBMIT_UPGRADE) {
                char email[256], phone[256], trx[256], pkg[256]; GetWindowText(hEditEmail, email, 256); GetWindowText(hEditPhone, phone, 256); GetWindowText(hEditTrx, trx, 256);
                int sel = SendMessage(hComboPkg, CB_GETCURSEL, 0, 0); SendMessage(hComboPkg, CB_GETLBTEXT, sel, (LPARAM)pkg);
                if (strlen(email) == 0 || strlen(phone) == 0 || strlen(trx) == 0) { MessageBox(hwnd, "Please fill up all the fields!", "Error", MB_ICONERROR); return 0; }
                string deviceId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
                string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ deviceID = @{ stringValue = '" + deviceId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + string(pkg) + "' }; userEmail = @{ stringValue = '" + string(email) + "' }; senderPhone = @{ stringValue = '" + string(phone) + "' }; trxId = @{ stringValue = '" + string(trx) + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
                SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
                MessageBox(hwnd, "Request sent successfully! Please wait for Admin approval.", "Success", MB_OK | MB_ICONINFORMATION); ShowWindow(hwnd, SW_HIDE);
            } break;
        }
        case WM_CLOSE: { ShowWindow(hwnd, SW_HIDE); return 0; }
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd, &ps); RECT r; GetClientRect(hwnd, &r); COLORREF bg=(currentOverlayType==1)?colOverlayAdultBg:colOverlayTimeBg; HBRUSH br=CreateSolidBrush(bg); FillRect(hdc, &r, br); DeleteObject(br);
            SetTextColor(hdc, colOverlayText); SetBkMode(hdc, TRANSPARENT); HFONT f=CreateFontW(24,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, L"Segoe UI");
            SelectObject(hdc, f); r.left+=20; r.right-=20; r.top+=40; DrawTextW(hdc, currentDisplayQuote.c_str(), -1, &r, DT_CENTER|DT_WORDBREAK); DeleteObject(f); EndPaint(hwnd, &ps); return 0;
        }
        case WM_TIMER: if(wParam==2){ ShowWindow(hwnd, SW_HIDE); KillTimer(hwnd, 2); isOverlayVisible=false; return 0; } break;
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK EyePanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hB, hW;
    switch(msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(18,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");
            CreateWindow("STATIC", "Brightness:", WS_VISIBLE|WS_CHILD, 20,20,180,25, hwnd, NULL, NULL, NULL);
            hB=CreateWindow(TRACKBAR_CLASS, "", WS_VISIBLE|WS_CHILD|TBS_AUTOTICKS, 20,50,400,35, hwnd, (HMENU)201, NULL, NULL); SendMessage(hB, TBM_SETRANGE, TRUE, MAKELONG(10,100)); SendMessage(hB, TBM_SETPOS, TRUE, eyeBrightness);
            CreateWindow("STATIC", "Warmth:", WS_VISIBLE|WS_CHILD, 20,110,180,25, hwnd, NULL, NULL, NULL);
            hW=CreateWindow(TRACKBAR_CLASS, "", WS_VISIBLE|WS_CHILD|TBS_AUTOTICKS, 20,140,400,35, hwnd, (HMENU)202, NULL, NULL); SendMessage(hW, TBM_SETRANGE, TRUE, MAKELONG(0,100)); SendMessage(hW, TBM_SETPOS, TRUE, eyeWarmth);
            CreateWindow("BUTTON", "Day Mode", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 20,200,120,40, hwnd, (HMENU)203, NULL, NULL); CreateWindow("BUTTON", "Reading", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 160,200,120,40, hwnd, (HMENU)204, NULL, NULL); CreateWindow("BUTTON", "Night Time", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 300,200,120,40, hwnd, (HMENU)205, NULL, NULL);
            EnumChildWindows(hwnd, [](HWND c, LPARAM l)->BOOL{SendMessage(c,WM_SETFONT,l,TRUE);return TRUE;}, (LPARAM)hFont); return 0;
        }
        case WM_CTLCOLORSTATIC: { SetBkMode((HDC)wParam, TRANSPARENT); return (LRESULT)hbrBg; }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam);
            HBRUSH br=CreateSolidBrush(colBtnGray); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10);
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE;
        }
        case WM_HSCROLL: { eyeBrightness=SendMessage(hB,TBM_GETPOS,0,0); eyeWarmth=SendMessage(hW,TBM_GETPOS,0,0); ApplyEyeFilters(); SaveSessionData(); return 0; }
        case WM_COMMAND: { int id=LOWORD(wParam); if(id==203){eyeBrightness=100;eyeWarmth=0;} if(id==204){eyeBrightness=85;eyeWarmth=30;} if(id==205){eyeBrightness=60;eyeWarmth=75;} SendMessage(hB,TBM_SETPOS,TRUE,eyeBrightness); SendMessage(hW,TBM_SETPOS,TRUE,eyeWarmth); ApplyEyeFilters(); SaveSessionData(); break; }
        case WM_CLOSE: { ShowWindow(hwnd, SW_HIDE); return 0; }
    } return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
            int cornerPref = 2; DwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref)); 
            HFONT hFont = CreateFont(16,0,0,0,FW_NORMAL,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");
            HFONT hFontBold = CreateFont(16,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS, "Segoe UI");

            HICON hI = (HICON)LoadImage(NULL, "icon.ico", IMAGE_ICON, 0,0, LR_LOADFROMFILE|LR_DEFAULTSIZE);
            if(!hI) hI=LoadIcon(NULL,IDI_SHIELD); SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hI); SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hI);
            nid.cbSize=sizeof(NOTIFYICONDATA); nid.hWnd=hwnd; nid.uID=1001; nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; nid.uCallbackMessage=WM_TRAYICON; nid.hIcon=hI; Shell_NotifyIcon(NIM_ADD, &nid);

            CreateWindow("STATIC", "Profile Name:", WS_VISIBLE|WS_CHILD, 15, 17, 100, 20, hwnd, NULL, NULL, NULL);
            hEditProfileName = CreateWindow("EDIT", userProfileName.c_str(), WS_VISIBLE|WS_CHILD|WS_BORDER, 110, 15, 150, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("BUTTON", "SAVE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 270, 14, 60, 28, hwnd, (HMENU)ID_BTN_SAVE_PROFILE, NULL, NULL);
            hLblTrialStatus = CreateWindow("STATIC", "TRIAL: 7 DAYS LEFT", WS_VISIBLE|WS_CHILD|SS_CENTER, 350, 15, 300, 25, hwnd, NULL, NULL, NULL);
            CreateWindow("BUTTON", "LIVE CHAT", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 670, 14, 120, 30, hwnd, (HMENU)ID_BTN_LIVE_CHAT, NULL, NULL);
            hBtnUpgrade = CreateWindow("BUTTON", "UPGRADE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 800, 14, 100, 30, hwnd, (HMENU)ID_BTN_UPGRADE, NULL, NULL);

            int topY = 65;
            HWND lblSelfPass = CreateWindow("STATIC", "Friend Control (Password):", WS_VISIBLE|WS_CHILD, 15, topY-20, 190, 20, hwnd, NULL, NULL, NULL);
            hPassEdit = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_PASSWORD, 15, topY, 140, 28, hwnd, NULL, NULL, NULL);
            
            HWND lblSelfTime = CreateWindow("STATIC", "Self Control (Timer Hr:Min):", WS_VISIBLE|WS_CHILD, 170, topY-20, 190, 20, hwnd, NULL, NULL, NULL);
            hTimeHrEdit = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, 170, topY, 50, 28, hwnd, NULL, NULL, NULL);
            hTimeMinEdit = CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER|ES_NUMBER, 230, topY, 50, 28, hwnd, NULL, NULL, NULL);
            
            hBtnStart = CreateWindow("BUTTON", "START", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 295, topY-3, 80, 32, hwnd, (HMENU)ID_BTN_START_FOCUS, NULL, NULL);
            hBtnStopFocus = CreateWindow("BUTTON", "STOP", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 385, topY-3, 80, 32, hwnd, (HMENU)ID_BTN_STOP_FOCUS, NULL, NULL);
            hBtnStopwatch = CreateWindow("BUTTON", "STOP WATCH", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 475, topY-3, 100, 32, hwnd, (HMENU)ID_BTN_STOPWATCH, NULL, NULL);
            hBtnPomodoro = CreateWindow("BUTTON", "POMODORO", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 585, topY-3, 100, 32, hwnd, (HMENU)ID_BTN_POMODORO, NULL, NULL);
            hBtnEyeCure = CreateWindow("BUTTON", "EYE CURE", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, 695, topY-3, 100, 32, hwnd, (HMENU)ID_BTN_EYE_CURE, NULL, NULL);
            hLblTimeLeft = CreateWindow("STATIC", "Ready", WS_VISIBLE|WS_CHILD, 810, topY+2, 120, 25, hwnd, NULL, NULL, NULL);

            hLblFocusStatus = CreateWindow("STATIC", "", WS_VISIBLE|WS_CHILD, 295, topY-25, 600, 20, hwnd, NULL, NULL, NULL);

            int row3Y = 115;
            hRadioBlock = CreateWindow("BUTTON", "Block List", WS_VISIBLE|WS_CHILD|BS_AUTORADIOBUTTON, 15, row3Y, 90, 25, hwnd, (HMENU)ID_RADIO_BLOCK, NULL, NULL);
            hRadioAllow = CreateWindow("BUTTON", "Allow List (Only Allow List runs in Pomodoro)", WS_VISIBLE|WS_CHILD|BS_AUTORADIOBUTTON, 115, row3Y, 310, 25, hwnd, (HMENU)ID_RADIO_ALLOW, NULL, NULL);
            hChkAdBlock = CreateWindow("BUTTON", "AD BLOCKER (Silent)", WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX, 440, row3Y, 150, 25, hwnd, (HMENU)ID_CHK_ADBLOCK, NULL, NULL);
            hChkReels = CreateWindow("BUTTON", "Block FB Reels", WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX, 600, row3Y, 130, 25, hwnd, (HMENU)ID_CHK_REELS, NULL, NULL);
            hChkShorts = CreateWindow("BUTTON", "Block YT Shorts", WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX, 740, row3Y, 140, 25, hwnd, (HMENU)ID_CHK_SHORTS, NULL, NULL);

            int cY = 155, w = 270, lH = 100, c1 = 15, c2 = 300, c3 = 605;
            
            CreateWindow("STATIC", "Block Apps (e.g., vlc.exe):", WS_VISIBLE|WS_CHILD, c1,cY,w,20, hwnd, NULL, NULL, NULL);
            hAppInput=CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, c1,cY+20,w-150,25, hwnd, (HMENU)2001, NULL, NULL);
            hComboApp=CreateWindow("COMBOBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL, c1+w-145,cY+20,80,200, hwnd, (HMENU)2009, NULL, NULL);
            hBtnAddApp=CreateWindow("BUTTON", "ADD", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c1+w-60,cY+20,60,25, hwnd, (HMENU)ID_BTN_ADD_APP, NULL, NULL);
            hAppList=CreateWindow("LISTBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL, c1,cY+50,w,lH, hwnd, (HMENU)3, NULL, NULL);
            hBtnRemApp=CreateWindow("BUTTON", "Remove", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c1,cY+155,w,30, hwnd, (HMENU)ID_BTN_REM_APP, NULL, NULL);
            
            int block2Y = 310;
            CreateWindow("STATIC", "Block Websites:", WS_VISIBLE|WS_CHILD, c1,block2Y,w,20, hwnd, NULL, NULL, NULL);
            hWebInput=CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, c1,block2Y+20,w-150,25, hwnd, (HMENU)2002, NULL, NULL);
            hComboWeb=CreateWindow("COMBOBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL, c1+w-145,block2Y+20,80,200, hwnd, (HMENU)2007, NULL, NULL);
            hBtnAddWeb=CreateWindow("BUTTON", "ADD", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c1+w-60,block2Y+20,60,25, hwnd, (HMENU)ID_BTN_ADD_WEB, NULL, NULL);
            hWebList=CreateWindow("LISTBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL, c1,block2Y+50,w,lH, hwnd, (HMENU)7, NULL, NULL);
            hBtnRemWeb=CreateWindow("BUTTON", "Remove", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c1,block2Y+155,w,30, hwnd, (HMENU)ID_BTN_REM_WEB, NULL, NULL);

            CreateWindow("STATIC", "Running Apps (Auto-Detected):", WS_VISIBLE|WS_CHILD, c2,cY,w,20, hwnd, NULL, NULL, NULL);
            hBtnAddFromRunning=CreateWindow("BUTTON", "Add Selected App to List", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c2,cY+20,w,25, hwnd, (HMENU)ID_BTN_ADD_RUNNING, NULL, NULL);
            hRunningList=CreateWindow("LISTBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL, c2,cY+50,w,285, hwnd, (HMENU)5, NULL, NULL);

            CreateWindow("STATIC", "Allow Apps (e.g., code.exe):", WS_VISIBLE|WS_CHILD, c3,cY,w,20, hwnd, NULL, NULL, NULL);
            hAllowAppInput=CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, c3,cY+20,w-150,25, hwnd, (HMENU)2003, NULL, NULL);
            hComboAllowApp=CreateWindow("COMBOBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL, c3+w-145,cY+20,80,200, hwnd, (HMENU)2010, NULL, NULL);
            hBtnAddAllowApp=CreateWindow("BUTTON", "ADD", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c3+w-60,cY+20,60,25, hwnd, (HMENU)ID_BTN_ADD_ALLOW_APP, NULL, NULL);
            hAllowAppList=CreateWindow("LISTBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL, c3,cY+50,w,lH, hwnd, (HMENU)2004, NULL, NULL);
            hBtnRemAllowApp=CreateWindow("BUTTON", "Remove", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c3,cY+155,w,30, hwnd, (HMENU)ID_BTN_REM_ALLOW_APP, NULL, NULL);
            
            CreateWindow("STATIC", "Allow Webs:", WS_VISIBLE|WS_CHILD, c3,block2Y,w,20, hwnd, NULL, NULL, NULL);
            hAllowWebInput=CreateWindow("EDIT", "", WS_VISIBLE|WS_CHILD|WS_BORDER, c3,block2Y+20,w-150,25, hwnd, (HMENU)2005, NULL, NULL);
            hComboAllowWeb=CreateWindow("COMBOBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL, c3+w-145,block2Y+20,80,200, hwnd, (HMENU)2008, NULL, NULL);
            hBtnAddAllowWeb=CreateWindow("BUTTON", "ADD", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c3+w-60,block2Y+20,60,25, hwnd, (HMENU)ID_BTN_ADD_ALLOW_WEB, NULL, NULL);
            hAllowWebList=CreateWindow("LISTBOX", "", WS_VISIBLE|WS_CHILD|WS_BORDER|WS_VSCROLL, c3,block2Y+50,w,lH, hwnd, (HMENU)2006, NULL, NULL);
            hBtnRemAllowWeb=CreateWindow("BUTTON", "Remove", WS_VISIBLE|WS_CHILD|BS_OWNERDRAW, c3,block2Y+155,w,30, hwnd, (HMENU)ID_BTN_REM_ALLOW_WEB, NULL, NULL);

            hLblAdminMsg = CreateWindow("STATIC", "", WS_VISIBLE|WS_CHILD, 15, 510, 900, 40, hwnd, NULL, NULL, NULL);

            const char* popSites[] = {"Select..", "facebook.com", "youtube.com", "instagram.com", "tiktok.com", "reddit.com", "netflix.com", "twitter.com", "pinterest.com", "twitch.tv"};
            for(auto site : popSites) { 
                SendMessage(hComboWeb, CB_ADDSTRING, 0, (LPARAM)site); 
                SendMessage(hComboAllowWeb, CB_ADDSTRING, 0, (LPARAM)site); 
            }
            SendMessage(hComboWeb, CB_SETCURSEL, 0, 0); SendMessage(hComboAllowWeb, CB_SETCURSEL, 0, 0);

            const char* popApps[] = {"Select..", "chrome.exe", "msedge.exe", "firefox.exe", "brave.exe", "opera.exe", "vivaldi.exe", "yandex.exe", "safari.exe", "waterfox.exe", "code.exe", "pycharm64.exe", "python.exe", "idea64.exe", "studio64.exe", "vlc.exe", "telegram.exe", "whatsapp.exe", "discord.exe", "zoom.exe", "skype.exe", "obs64.exe", "steam.exe", "epicgameslauncher.exe", "winword.exe", "excel.exe", "powerpnt.exe", "notepad.exe", "spotify.exe"};
            for(auto app : popApps) { 
                SendMessage(hComboApp, CB_ADDSTRING, 0, (LPARAM)app); 
                SendMessage(hComboAllowApp, CB_ADDSTRING, 0, (LPARAM)app); 
            }
            SendMessage(hComboApp, CB_SETCURSEL, 0, 0); SendMessage(hComboAllowApp, CB_SETCURSEL, 0, 0);

            EnumChildWindows(hwnd, [](HWND c, LPARAM l)->BOOL{SendMessage(c,WM_SETFONT,l,TRUE);return TRUE;}, (LPARAM)hFont);
            SendMessage(lblSelfPass, WM_SETFONT, (WPARAM)hFontBold, TRUE); SendMessage(lblSelfTime, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            SendMessage(hLblTimeLeft, WM_SETFONT, (WPARAM)hFontBold, TRUE); SendMessage(hLblTrialStatus, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            SendMessage(hLblAdminMsg, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            SendMessage(hLblFocusStatus, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            SetWindowText(hEditProfileName, userProfileName.c_str());

            int vW_Sc=GetSystemMetrics(SM_CXSCREEN), vH_Sc=GetSystemMetrics(SM_CYSCREEN);
            hExpiredPanel=CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW, "ExpCls", "", WS_POPUP | WS_BORDER, (vW_Sc-500)/2, (vH_Sc-250)/2, 500, 250, NULL, NULL, hInstance, NULL);

            LoadData(); LoadSessionData(); 
            SendMessage(hRadioBlock, BM_SETCHECK, useAllowMode?BST_UNCHECKED:BST_CHECKED, 0); SendMessage(hRadioAllow, BM_SETCHECK, useAllowMode?BST_CHECKED:BST_UNCHECKED, 0); 
            SendMessage(hChkReels, BM_SETCHECK, blockReels?BST_CHECKED:BST_UNCHECKED, 0); SendMessage(hChkShorts, BM_SETCHECK, blockShorts?BST_CHECKED:BST_UNCHECKED, 0); 
            SendMessage(hChkAdBlock, BM_SETCHECK, isAdblockActive?BST_CHECKED:BST_UNCHECKED, 0);
            
            for(auto a:blockedApps) SendMessage(hAppList, LB_ADDSTRING, 0, (LPARAM)a.c_str()); for(auto w:blockedWebs) SendMessage(hWebList, LB_ADDSTRING, 0, (LPARAM)w.c_str());
            for(auto a:allowedApps) SendMessage(hAllowAppList, LB_ADDSTRING, 0, (LPARAM)a.c_str()); for(auto w:allowedWebs) SendMessage(hAllowWebList, LB_ADDSTRING, 0, (LPARAM)w.c_str());
            vector<string> rApps = GetAppListForUI(); for(const auto& rA : rApps) SendMessage(hRunningList, LB_ADDSTRING, 0, (LPARAM)rA.c_str());

            ValidateLicenseAndTrial(); CreateThread(NULL, 0, LicenseCheckThread, NULL, 0, NULL);
            if (isSessionActive) { SetTimer(hwnd, 1, 1000, NULL); UpdateTimerDisplay(); UpdateMainUIState(); }
            
            SetTimer(hwnd, 3, 1000, NULL); 
            SetTimer(hwnd, 4, 3000, NULL); 
            SetTimer(hwnd, 5, 200, NULL); // -----------------> পরিবর্তন: ফাস্ট টাইমার যুক্ত করা হলো
            break;
        }
        case WM_WAKEUP: { ShowWindow(hwnd, SW_SHOW); ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); return 0; }
        case WM_TRAYICON: { if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) { ShowWindow(hwnd, SW_SHOW); ShowWindow(hwnd, SW_RESTORE); SetForegroundWindow(hwnd); } break; }
        case WM_CTLCOLORSTATIC: { 
            HDC hdc = (HDC)wParam; HWND hw = (HWND)lParam; SetBkMode(hdc, TRANSPARENT); 
            if (hw == hLblTimeLeft) SetTextColor(hdc, colBtnRed); 
            else if (hw == hLblTrialStatus) SetTextColor(hdc, isLicenseValid ? colBtnGreen : (isTrialExpired ? colBtnRed : RGB(230, 126, 34))); 
            else if (hw == hLblAdminMsg) SetTextColor(hdc, colBtnPurple); 
            else if (hw == hLblFocusStatus) SetTextColor(hdc, colBtnRed);
            else SetTextColor(hdc, colTextDark); 
            return (LRESULT)hbrBg; 
        }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p=(LPDRAWITEMSTRUCT)lParam; if(p->CtlType!=ODT_BUTTON) return DefWindowProc(hwnd, msg, wParam, lParam); COLORREF c=colBtnGray; int id=p->CtlID;
            
            if(id==ID_BTN_START_FOCUS) c = (isTrialExpired || isSessionActive) ? colBtnGray : colBtnGreen; 
            else if(id==ID_BTN_STOP_FOCUS) c = isSessionActive ? colBtnRed : colBtnGray; 
            else if(id==ID_BTN_EYE_CURE) c=colBtnPurple; else if(id==ID_BTN_UPGRADE) c=colBtnGold; else if(id==ID_BTN_LIVE_CHAT) c=RGB(236, 72, 153); else if(id==ID_BTN_POMODORO) c=RGB(239, 68, 68); else if(id==ID_BTN_SAVE_PROFILE) c=colBtnGreen; else if(id==ID_BTN_STOPWATCH) c=colTextDark; else c=colBtnBlue;
            
            HBRUSH br=CreateSolidBrush(c); SelectObject(p->hDC, br); RoundRect(p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right, p->rcItem.bottom, 10, 10); 
            char t[256]; GetWindowText(p->hwndItem, t, 256); SetTextColor(p->hDC, RGB(255,255,255)); SetBkMode(p->hDC, TRANSPARENT); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(br); return TRUE;
        }
        case WM_TIMER: {
            // ---------------------------------------------------------
            // ফাস্ট লজিক (২০০ms) - ট্যাবের নাম ও শর্টস/রিলস ধরার জন্য
            // ---------------------------------------------------------
            if (wParam == 5) {
                CheckAlwaysOnAdultFilter();
                if (isSessionActive) {
                    EnforceFocusMode_Window();
                }
            }
            if (wParam==4) { SyncLiveTrackerToFirebase(); }
            if (wParam==3) { 
                SendMessage(hChkReels, BM_SETCHECK, blockReels?BST_CHECKED:BST_UNCHECKED, 0); 
                SendMessage(hChkShorts, BM_SETCHECK, blockShorts?BST_CHECKED:BST_UNCHECKED, 0); 
                SendMessage(hChkAdBlock, BM_SETCHECK, isAdblockActive?BST_CHECKED:BST_UNCHECKED, 0);

                char tBuf[100];
                if (isTrialExpired) {
                    SetWindowTextA(hLblTrialStatus, "LICENSE EXPIRED / REVOKED");
                    ShowWindow(hBtnUpgrade, SW_SHOW); 
                } else {
                    if (isLicenseValid) {
                        sprintf(tBuf, "PREMIUM: %d DAYS LEFT", trialDaysLeft);
                        ShowWindow(hBtnUpgrade, SW_HIDE);
                    } else {
                        sprintf(tBuf, "TRIAL: %d DAYS LEFT", trialDaysLeft);
                        ShowWindow(hBtnUpgrade, SW_SHOW);
                    }
                    SetWindowTextA(hLblTrialStatus, tBuf);
                }

                if (isTrialExpired) { 
                    if (!userClosedExpired) { ShowWindow(hExpiredPanel, SW_SHOW); BringWindowToTop(hExpiredPanel); }
                    EnableWindow(hPassEdit, FALSE); EnableWindow(hTimeHrEdit, FALSE); EnableWindow(hTimeMinEdit, FALSE); EnableWindow(hBtnPomodoro, FALSE); 
                    EnableWindow(hChkAdBlock, FALSE); EnableWindow(hChkReels, FALSE); EnableWindow(hChkShorts, FALSE); EnableWindow(hRadioBlock, FALSE); EnableWindow(hRadioAllow, FALSE);
                } else {
                    userClosedExpired = false; 
                    ShowWindow(hExpiredPanel, SW_HIDE);
                    if (!isSessionActive) {
                        EnableWindow(hChkAdBlock, TRUE); EnableWindow(hBtnPomodoro, TRUE); EnableWindow(hChkReels, TRUE); EnableWindow(hChkShorts, TRUE); EnableWindow(hRadioBlock, TRUE); EnableWindow(hRadioAllow, TRUE);
                        int pLen = GetWindowTextLength(hPassEdit); int tLen = GetWindowTextLength(hTimeHrEdit) + GetWindowTextLength(hTimeMinEdit); 
                        EnableWindow(hPassEdit, (tLen == 0)); EnableWindow(hTimeHrEdit, (pLen == 0)); EnableWindow(hTimeMinEdit, (pLen == 0));
                        EnableWindow(hBtnPomodoro, (pLen == 0 && tLen == 0)); 
                    }
                }
                
                if (!safeAdminMsg.empty()) { 
                    string msg = "Admin Notice: " + safeAdminMsg; 
                    SetWindowTextA(hLblAdminMsg, msg.c_str()); 
                } else { 
                    SetWindowTextA(hLblAdminMsg, ""); 
                }

                if (!pendingBroadcastMsg.empty() && pendingBroadcastMsg != "ACK") {
                    currentBroadcastMsg = pendingBroadcastMsg;
                    pendingBroadcastMsg = "";
                    ShowWindow(hBroadcastPanel, SW_SHOWNOACTIVATE); 
                    SetWindowPos(hBroadcastPanel, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE); 
                    InvalidateRect(hBroadcastPanel, NULL, TRUE); 
                }

                if (!pendingAdminChatStr.empty()) {
                    if (hChatLogEdit) { 
                        int len = GetWindowTextLength(hChatLogEdit); SendMessage(hChatLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len); 
                        string msg = "Admin: " + pendingAdminChatStr + "\r\n"; SendMessage(hChatLogEdit, EM_REPLACESEL, 0, (LPARAM)msg.c_str()); 
                    }
                    pendingAdminChatStr = "";
                }

                if (pendingAdminCmd == 1 && !isSessionActive) { 
                    pendingAdminCmd = 0; currentSessionPass = pendingAdminPass; isPassMode = true; isTimeMode = false; isPomodoroMode = false; isSessionActive = true; 
                    SaveSessionData(); SetTimer(hwnd, 1, 1000, NULL); UpdateTimerDisplay(); UpdateMainUIState(); 
                    SetWindowText(hPassEdit, ""); SetWindowText(hTimeHrEdit, ""); SetWindowText(hTimeMinEdit, ""); ShowWindow(hwnd, SW_HIDE); ToggleUIElements(false); 
                    string dId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_START' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\""; SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei); 
                }
                else if (pendingAdminCmd == 2 && isSessionActive) { 
                    pendingAdminCmd = 0; ClearSessionData(); ToggleUIElements(true); UpdateMainUIState(); 
                    string dId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_STOP' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\""; SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei); 
                }
            }
            if(wParam==1 && isSessionActive){ 
                if (isTrialExpired && isSessionActive) { ClearSessionData(); UpdateMainUIState(); MessageBox(hwnd, "License Revoked or Expired. Session Terminated.", "Error", MB_ICONERROR); return 0; }
                UpdateTimerDisplay(); 
                EnforceFocusMode_Processes(); // ব্যাকগ্রাউন্ড প্রসেস চেক (১ সেকেন্ড পর পর)
                
                if(isPomodoroMode){ 
                    pomoTicks++; if(pomoTicks%5==0) SaveSessionData(); 
                    if(!isPomodoroBreak && pomoTicks>=pomoLengthMin*60){ isPomodoroBreak=true; pomoTicks=0; ShowPomodoroBreakPage(); } 
                    else if(isPomodoroBreak && pomoTicks>=2*60){ isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); MessageBox(hwnd, "All Pomodoro Sessions Completed!", "Success", MB_OK); UpdateMainUIState(); } } 
                } 
                else if(isTimeMode && focusTimeTotalSeconds>0){ timerTicks++; if(timerTicks%5==0) SaveSessionData(); if(timerTicks>=focusTimeTotalSeconds){ ClearSessionData(); MessageBox(hwnd, "Focus time is over!", "Success", MB_OK); UpdateMainUIState(); } }
            }
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam); int event = HIWORD(wParam);
            
            if (id == 2009 && event == CBN_SELCHANGE) { char b[256]; int s=SendMessage(hComboApp, CB_GETCURSEL, 0, 0); if (s>0) { SendMessage(hComboApp, CB_GETLBTEXT, s, (LPARAM)b); SetWindowText(hAppInput, b); } }
            if (id == 2010 && event == CBN_SELCHANGE) { char b[256]; int s=SendMessage(hComboAllowApp, CB_GETCURSEL, 0, 0); if (s>0) { SendMessage(hComboAllowApp, CB_GETLBTEXT, s, (LPARAM)b); SetWindowText(hAllowAppInput, b); } }
            if (id == 2007 && event == CBN_SELCHANGE) { char b[256]; int s=SendMessage(hComboWeb, CB_GETCURSEL, 0, 0); if (s>0) { SendMessage(hComboWeb, CB_GETLBTEXT, s, (LPARAM)b); SetWindowText(hWebInput, b); } }
            if (id == 2008 && event == CBN_SELCHANGE) { char b[256]; int s=SendMessage(hComboAllowWeb, CB_GETCURSEL, 0, 0); if (s>0) { SendMessage(hComboAllowWeb, CB_GETLBTEXT, s, (LPARAM)b); SetWindowText(hAllowWebInput, b); } }

            if ((id == ID_CHK_REELS || id == ID_CHK_SHORTS || id == ID_CHK_ADBLOCK || id == ID_RADIO_BLOCK || id == ID_RADIO_ALLOW) && event == BN_CLICKED) {
                if (HandleSettingsWarning()) {
                    SendMessage(hRadioBlock, BM_SETCHECK, useAllowMode?BST_UNCHECKED:BST_CHECKED, 0); 
                    SendMessage(hRadioAllow, BM_SETCHECK, useAllowMode?BST_CHECKED:BST_UNCHECKED, 0); 
                    SendMessage(hChkReels, BM_SETCHECK, blockReels?BST_CHECKED:BST_UNCHECKED, 0); 
                    SendMessage(hChkShorts, BM_SETCHECK, blockShorts?BST_CHECKED:BST_UNCHECKED, 0); 
                    SendMessage(hChkAdBlock, BM_SETCHECK, isAdblockActive?BST_CHECKED:BST_UNCHECKED, 0);
                    return 0;
                }
            }

            if (id == ID_CHK_REELS && event == BN_CLICKED) { blockReels = (SendMessage(hChkReels, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSessionData(); SyncTogglesToFirebase(); }
            if (id == ID_CHK_SHORTS && event == BN_CLICKED) { blockShorts = (SendMessage(hChkShorts, BM_GETCHECK, 0, 0) == BST_CHECKED); SaveSessionData(); SyncTogglesToFirebase(); }
            if (id == ID_BTN_SAVE_PROFILE) { char n[100]; GetWindowText(hEditProfileName, n, 100); userProfileName = string(n); SaveSessionData(); SyncProfileNameToFirebase(userProfileName); MessageBox(hwnd, "Profile Name Saved!", "Success", MB_OK); }
            if (id == ID_CHK_ADBLOCK && event == BN_CLICKED) { isAdblockActive = (SendMessage(hChkAdBlock, BM_GETCHECK, 0, 0) == BST_CHECKED); ToggleAdBlock(isAdblockActive); SaveSessionData(); SyncTogglesToFirebase(); }
            
            if (id==ID_RADIO_BLOCK || id==ID_RADIO_ALLOW) { 
                useAllowMode=(IsDlgButtonChecked(hwnd,ID_RADIO_ALLOW)==BST_CHECKED); 
                ToggleUIElements(true); 
                InvalidateRect(hwnd,NULL,TRUE); 
                SaveSessionData(); // -----------------> পরিবর্তন: Allow List এর রেডিও বাটন ক্লিক করলেই সেভ হবে। 
            }
            
            if (id==ID_BTN_EYE_CURE) { ShowWindow(hEyePanel, SW_SHOW); SetForegroundWindow(hEyePanel); }
            if (id==ID_BTN_UPGRADE) { ShowWindow(hUpgradePanel, SW_SHOW); SetForegroundWindow(hUpgradePanel); }
            if (id==ID_BTN_POMODORO) { ShowWindow(hPomodoroPanel, SW_SHOW); SetForegroundWindow(hPomodoroPanel); }
            if (id==ID_BTN_LIVE_CHAT) { ShowWindow(hLiveChatPanel, SW_SHOW); SetForegroundWindow(hLiveChatPanel); SetFocus(hChatInputEdit); }
            if (id==ID_BTN_STOPWATCH) { ShowWindow(hStopwatchWnd, SW_SHOW); SetForegroundWindow(hStopwatchWnd); }
            
            if (id==ID_BTN_START_FOCUS && !isTrialExpired && !isSessionActive) { 
                char p[50], h[10], m[10]; GetWindowText(hPassEdit,p,50); GetWindowText(hTimeHrEdit,h,10); GetWindowText(hTimeMinEdit,m,10); focusTimeTotalSeconds = (atoi(h)*3600)+(atoi(m)*60); 
                if (strlen(p)>0 || focusTimeTotalSeconds>0) { isPassMode=(strlen(p)>0); if(isPassMode) currentSessionPass=p; isTimeMode=(focusTimeTotalSeconds>0); isSessionActive=true; timerTicks=0; SaveSessionData(); SetTimer(hwnd,1,1000,NULL); ShowWindow(hwnd, SW_HIDE); ToggleUIElements(false); UpdateMainUIState(); SetWindowText(hPassEdit, ""); if(isPassMode) SyncPasswordToFirebase(currentSessionPass, true); MessageBox(hwnd, "Focus Started!", "Success", MB_OK); }
            }
            if (id==ID_BTN_STOP_FOCUS && isSessionActive) { 
                if (isPassMode) { char p[50]; GetWindowText(hPassEdit,p,50); if (currentSessionPass==string(p)) { ClearSessionData(); SyncPasswordToFirebase("", false); UpdateMainUIState(); SetWindowText(hPassEdit, ""); MessageBox(hwnd,"Stopped.","Success",MB_OK); } else { MessageBox(hwnd,"Wrong password!","Error",MB_ICONERROR); } }
                else if (isTimeMode) { MessageBox(hwnd,"Timer active! Wait.","Locked",MB_ICONWARNING); }
                else if (isPomodoroMode) { ClearSessionData(); UpdateMainUIState(); MessageBox(hwnd,"Pomodoro Stopped.","Success",MB_OK); EnableWindow(hPassEdit, TRUE); EnableWindow(hTimeHrEdit, TRUE); EnableWindow(hTimeMinEdit, TRUE); EnableWindow(hBtnPomodoro, TRUE); }
            }
            if (id==ID_BTN_ADD_APP||id==ID_BTN_ADD_WEB||id==ID_BTN_ADD_ALLOW_APP||id==ID_BTN_ADD_ALLOW_WEB||id==ID_BTN_ADD_RUNNING) {
                if (HandleSettingsWarning()) return 0;
                HWND in=(id==ID_BTN_ADD_APP||id==ID_BTN_ADD_ALLOW_APP)?((id==ID_BTN_ADD_APP)?GetDlgItem(hwnd,2001):GetDlgItem(hwnd,2003)):((id==ID_BTN_ADD_WEB)?GetDlgItem(hwnd,2002):GetDlgItem(hwnd,2005)); HWND ls=(id==ID_BTN_ADD_APP||id==ID_BTN_ADD_ALLOW_APP)?((id==ID_BTN_ADD_APP)?GetDlgItem(hwnd,3):GetDlgItem(hwnd,2004)):((id==ID_BTN_ADD_WEB)?GetDlgItem(hwnd,7):GetDlgItem(hwnd,2006)); vector<string>& t=(id==ID_BTN_ADD_APP||id==ID_BTN_ADD_ALLOW_APP)?((id==ID_BTN_ADD_APP)?blockedApps:allowedApps):((id==ID_BTN_ADD_WEB)?blockedWebs:allowedWebs);
                if(id==ID_BTN_ADD_RUNNING){ HWND hr = GetDlgItem(hwnd, 5); int s=SendMessage(hr,LB_GETCURSEL,0,0); if(s!=LB_ERR){ char b[256]; SendMessage(hr,LB_GETTEXT,s,(LPARAM)b); string a=b; HWND tl=useAllowMode?GetDlgItem(hwnd,2004):GetDlgItem(hwnd,3); vector<string>& tv=useAllowMode?allowedApps:blockedApps; if(find(tv.begin(),tv.end(),a)==tv.end()){ tv.push_back(a); SendMessage(tl,LB_ADDSTRING,0,(LPARAM)a.c_str()); SaveData(); } } } 
                else { char b[256]; GetWindowText(in,b,256); string s=b; if(!s.empty() && s!="Select.."){ t.push_back(s); SendMessage(ls,LB_ADDSTRING,0,(LPARAM)s.c_str()); SaveData(); SetWindowText(in,""); SendMessage((id==ID_BTN_ADD_WEB?hComboWeb:(id==ID_BTN_ADD_ALLOW_WEB?hComboAllowWeb:(id==ID_BTN_ADD_APP?hComboApp:hComboAllowApp))), CB_SETCURSEL, 0, 0); } }
            }
            if (id==ID_BTN_REM_APP||id==ID_BTN_REM_WEB||id==ID_BTN_REM_ALLOW_APP||id==ID_BTN_REM_ALLOW_WEB) { 
                if (HandleSettingsWarning()) return 0;
                HWND ls=(id==ID_BTN_REM_APP||id==ID_BTN_REM_ALLOW_APP)?((id==ID_BTN_REM_APP)?GetDlgItem(hwnd,3):GetDlgItem(hwnd,2004)):((id==ID_BTN_REM_WEB)?GetDlgItem(hwnd,7):GetDlgItem(hwnd,2006)); vector<string>& t=(id==ID_BTN_REM_APP||id==ID_BTN_REM_ALLOW_APP)?((id==ID_BTN_REM_APP)?blockedApps:allowedApps):((id==ID_BTN_REM_WEB)?blockedWebs:allowedWebs); int s=SendMessage(ls,LB_GETCURSEL,0,0); if(s!=LB_ERR){ char b[256]; SendMessage(ls,LB_GETTEXT,s,(LPARAM)b); t.erase(remove(t.begin(),t.end(),string(b)),t.end()); SendMessage(ls,LB_DELETESTRING,s,0); SaveData(); } 
            }
            break;
        }
        case WM_CLOSE: { ShowWindow(hwnd, SW_HIDE); return 0; }
        case WM_DESTROY: { UnhookWindowsHookEx(hKeyboardHook); Shell_NotifyIcon(NIM_DELETE, &nid); PostQuitMessage(0); break; }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!CheckSingleInstance()) return 0; 
    
    CreateDesktopShortcut();
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
    
    string cmdStr = lpCmdLine; bool isAutoStart = (cmdStr.find("-autostart")!=string::npos);
    hbrBg = CreateSolidBrush(colBg); SetupAutoStart(); INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES}; InitCommonControlsEx(&icc);
    
    WNDCLASS wcO={0}; wcO.lpfnWndProc=OverlayProc; wcO.hInstance=hInstance; wcO.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH); wcO.lpszClassName="OverlayApp"; RegisterClass(&wcO);
    int vW=GetSystemMetrics(SM_CXVIRTUALSCREEN), vH=GetSystemMetrics(SM_CYVIRTUALSCREEN), vX=GetSystemMetrics(SM_XVIRTUALSCREEN), vY=GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    hOverlay=CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED, "OverlayApp", "", WS_POPUP, (vW-550)/2, (vH-200)/2, 550, 200, NULL, NULL, hInstance, NULL); SetLayeredWindowAttributes(hOverlay, 0, 245, LWA_ALPHA);

    WNDCLASS wcB={0}; wcB.lpfnWndProc=BroadcastProc; wcB.hInstance=hInstance; wcB.hbrBackground=CreateSolidBrush(RGB(15, 23, 42)); wcB.lpszClassName="BroadcastApp"; RegisterClass(&wcB);
    hBroadcastPanel=CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW, "BroadcastApp", "Admin Message", WS_POPUP, (vW-450)/2, (vH-220)/2, 450, 220, NULL, NULL, hInstance, NULL);

    WNDCLASS wcD={0}; wcD.lpfnWndProc=DefWindowProc; wcD.hInstance=hInstance; wcD.hbrBackground=CreateSolidBrush(RGB(0,0,0)); wcD.lpszClassName="DimCls"; RegisterClass(&wcD);
    hDimFilter=CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT, "DimCls", "", WS_POPUP, vX,vY,vW,vH, NULL, NULL, hInstance, NULL); SetLayeredWindowAttributes(hDimFilter,0,0,LWA_ALPHA);
    
    WNDCLASS wcW={0}; wcW.lpfnWndProc=DefWindowProc; wcW.hInstance=hInstance; wcW.hbrBackground=CreateSolidBrush(RGB(255,130,0)); wcW.lpszClassName="WarmCls"; RegisterClass(&wcW);
    hWarmFilter=CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT, "WarmCls", "", WS_POPUP, vX,vY,vW,vH, NULL, NULL, hInstance, NULL); SetLayeredWindowAttributes(hWarmFilter,0,0,LWA_ALPHA);

    WNDCLASS wcE={0}; wcE.lpfnWndProc=EyePanelProc; wcE.hInstance=hInstance; wcE.hbrBackground=hbrBg; wcE.lpszClassName="EyeCls"; RegisterClass(&wcE);
    hEyePanel=CreateWindowEx(WS_EX_TOOLWINDOW, "EyeCls", "Eye Cure", WS_SYSMENU|WS_CAPTION, (GetSystemMetrics(SM_CXSCREEN)-450)/2, (GetSystemMetrics(SM_CYSCREEN)-320)/2, 450, 320, NULL, NULL, hInstance, NULL);

    WNDCLASS wcU={0}; wcU.lpfnWndProc=UpgradePanelProc; wcU.hInstance=hInstance; wcU.hbrBackground=hbrBg; wcU.lpszClassName="UpgradeCls"; RegisterClass(&wcU);
    hUpgradePanel=CreateWindowEx(WS_EX_TOOLWINDOW, "UpgradeCls", "Premium Upgrade", WS_SYSMENU|WS_CAPTION, (GetSystemMetrics(SM_CXSCREEN)-450)/2, (GetSystemMetrics(SM_CYSCREEN)-450)/2, 450, 450, NULL, NULL, hInstance, NULL);

    WNDCLASS wcC={0}; wcC.lpfnWndProc=LiveChatProc; wcC.hInstance=hInstance; wcC.hbrBackground=hbrBg; wcC.lpszClassName="LiveChatCls"; RegisterClass(&wcC);
    hLiveChatPanel=CreateWindowEx(WS_EX_TOOLWINDOW, "LiveChatCls", "Live Chat", WS_SYSMENU|WS_CAPTION, (GetSystemMetrics(SM_CXSCREEN)-400)/2, (GetSystemMetrics(SM_CYSCREEN)-480)/2, 400, 480, NULL, NULL, hInstance, NULL);

    WNDCLASS wcP={0}; wcP.lpfnWndProc=PomodoroProc; wcP.hInstance=hInstance; wcP.hbrBackground=hbrBg; wcP.lpszClassName="PomoCls"; RegisterClass(&wcP);
    hPomodoroPanel=CreateWindowEx(WS_EX_TOOLWINDOW, "PomoCls", "Pomodoro Setup", WS_SYSMENU|WS_CAPTION, (GetSystemMetrics(SM_CXSCREEN)-350)/2, (GetSystemMetrics(SM_CYSCREEN)-280)/2, 350, 280, NULL, NULL, hInstance, NULL);

    WNDCLASS wcEx={0}; wcEx.lpfnWndProc=ExpiredProc; wcEx.hInstance=hInstance; wcEx.hbrBackground=CreateSolidBrush(RGB(30,41,59)); wcEx.lpszClassName="ExpCls"; RegisterClass(&wcEx);

    WNDCLASS wcSw={0}; wcSw.lpfnWndProc=StopwatchProc; wcSw.hInstance=hInstance; wcSw.hbrBackground=hbrBg; wcSw.lpszClassName="StopwatchCls"; RegisterClass(&wcSw);
    hStopwatchWnd=CreateWindowEx(0, "StopwatchCls", "Pro Stopwatch", WS_OVERLAPPEDWINDOW, (GetSystemMetrics(SM_CXSCREEN)-400)/2, (GetSystemMetrics(SM_CYSCREEN)-250)/2, 400, 250, NULL, NULL, hInstance, NULL);

    WNDCLASS wc={0}; wc.lpfnWndProc=WndProc; wc.hInstance=hInstance; wc.hbrBackground=hbrBg; wc.lpszClassName="FocusApp"; RegisterClass(&wc);
    hMainWnd = CreateWindow("FocusApp", "RasFocus Pro", WS_OVERLAPPEDWINDOW, (GetSystemMetrics(SM_CXSCREEN)-950)/2, (GetSystemMetrics(SM_CYSCREEN)-600)/2, 950, 600, NULL, NULL, hInstance, NULL);
    
    if(isSessionActive||isAutoStart) ShowWindow(hMainWnd,SW_HIDE); else ShowWindow(hMainWnd,SW_SHOW);
    MSG msg; while(GetMessage(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessage(&msg);} 
    UnhookWindowsHookEx(hKeyboardHook); return 0; 
}
