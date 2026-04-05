#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QScreen>
#include <QFontDatabase>
#include <QCloseEvent>
#include <QCheckBox>
#include <QStackedWidget>
#include <QTextEdit>
#include <QStandardPaths>
#include <QGroupBox>
#include <QStyle>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

// Windows Native Headers for Core Logic
#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#include <wininet.h>
#include <shlobj.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

using namespace std;

// --- GLOBALS FOR HOOKS AND WIN32 LOGIC ---
HHOOK hKeyboardHook;
string globalKeyBuffer = "";
vector<string> explicitKeywords = {"porn", "xxx", "sex", "nude", "nsfw", "xvideos", "pornhub", "xnxx", "xhamster", "brazzers", "onlyfans", "playboy", "mia khalifa", "bhabi", "chudai", "bangla choti", "magi", "sexy"};
vector<string> safeBrowserTitles = {"new tab", "start", "blank page", "allowed websites focus mode", "loading", "untitled", "connecting", "pomodoro break", "premium upgrade"};
vector<string> systemApps = {"explorer.exe", "svchost.exe", "taskmgr.exe", "cmd.exe", "conhost.exe", "csrss.exe", "dwm.exe", "lsass.exe", "services.exe", "smss.exe", "wininit.exe", "winlogon.exe", "spoolsv.exe", "fontdrvhost.exe", "searchui.exe", "searchindexer.exe", "sihost.exe", "taskhostw.exe", "ctfmon.exe", "applicationframehost.exe", "system", "registry", "audiodg.exe", "searchapp.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "textinputhost.exe"};

// --- GLOBAL HELPER FUNCTIONS ---
string GetSecretDir() {
    string dir = "C:\\ProgramData\\SysCache_Ras";
    CreateDirectoryA(dir.c_str(), NULL);
    SetFileAttributesA(dir.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "\\";
}
string GetSessionFilePath() { return GetSecretDir() + "session.dat"; }

string GetDeviceID() {
    char compName[MAX_COMPUTERNAME_LENGTH + 1]; DWORD size = sizeof(compName); GetComputerNameA(compName, &size);
    DWORD volSerial = 0; GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char id[256]; sprintf(id, "%s-%X", compName, volSerial); return string(id);
}

string EnsureExe(string n) { if(n.length()<4 || n.substr(n.length()-4)!=".exe") return n+".exe"; return n; }

string GenerateDisplayURL(string url) { 
    string s = url; string e[] = {"https://", "http://", "www.", "/*"}; 
    for(const string& p : e){ size_t pos = s.find(p); if(pos != string::npos) s.erase(pos, p.length()); } 
    return s; 
}

bool CheckMatch(string url, string sTitle) { 
    string t = sTitle; t.erase(::std::remove_if(t.begin(), t.end(), ::isspace), t.end()); 
    string s = url; ::std::transform(s.begin(), s.end(), s.begin(), ::tolower); 
    string exact = s; exact.erase(::std::remove_if(exact.begin(), exact.end(), ::isspace), exact.end()); 
    if (!exact.empty() && t.find(exact) != string::npos) return true; 
    ::std::replace(s.begin(), s.end(), '.', ' '); ::std::replace(s.begin(), s.end(), '/', ' '); ::std::replace(s.begin(), s.end(), ':', ' '); ::std::replace(s.begin(), s.end(), '-', ' '); 
    stringstream ss(s); string word; 
    while(ss >> word) { if (word=="https"||word=="http"||word=="www"||word=="com"||word=="org"||word=="net"||word=="html"||word=="github") continue; if (word.length()>=3 && t.find(word) != string::npos) return true; } 
    return false; 
}

void CloseActiveTabAndMinimize(HWND hBrowser) { 
    ::SetForegroundWindow(hBrowser); Sleep(50); 
    keybd_event(VK_CONTROL,0,0,0); keybd_event('W',0,0,0); keybd_event('W',0,KEYEVENTF_KEYUP,0); keybd_event(VK_CONTROL,0,KEYEVENTF_KEYUP,0); 
    Sleep(100); ::ShowWindow(hBrowser, SW_MINIMIZE); 
}

void ShowAllowedWebsitesPage(const vector<string>& allowedWebs) {
    static DWORD lastTime = 0; if (GetTickCount()-lastTime<3000) return; lastTime=GetTickCount();
    string hPath = GetSecretDir() + "allowed_sites.html"; ofstream html(hPath.c_str());
    html<<"<!DOCTYPE html><html><head><style>body{font-family:sans-serif;text-align:center;}a{background:#007bff;color:white;padding:10px;margin:5px;display:inline-block;}</style></head><body><h2>Focus Active! Allowed:</h2>";
    for (const auto& w:allowedWebs) { html<<"<a href='https://"<<GenerateDisplayURL(w).c_str()<<"' target='_blank'>"<<w.c_str()<<"</a>"; }
    html<<"</body></html>"; html.close(); ShellExecuteA(NULL, "open", hPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void ShowPomodoroBreakPage() {
    char p[MAX_PATH]; ::GetCurrentDirectoryA(MAX_PATH, p); string hPath = string(p) + "\\pomodoro_break.html"; ofstream html(hPath.c_str());
    html<<"<!DOCTYPE html><html><head><style>body{margin:0;height:100vh;display:flex;flex-direction:column;justify-content:center;align-items:center;background:linear-gradient(to bottom, #1e3c72, #2a5298);color:white;font-family:'Segoe UI',sans-serif;}h1{font-size:50px;margin-bottom:10px;}p{font-size:20px;color:#a0c4ff;}</style></head><body><h1>Time to Relax & Drink Water!</h1><p>2 Minutes Break Started.</p></body></html>";
    html.close(); ShellExecuteA(NULL, "open", hPath.c_str(), NULL, NULL, SW_SHOWMAXIMIZED);
}

struct WindowProcData { vector<string> apps; };
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) { 
    if(!::IsWindowVisible(hwnd)) return TRUE; if(::GetWindowTextLengthA(hwnd)==0) return TRUE; 
    DWORD pid = 0; ::GetWindowThreadProcessId(hwnd, &pid); if(pid == 0 || pid == ::GetCurrentProcessId()) return TRUE; 
    HANDLE h = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid); 
    if(h){ 
        char n[MAX_PATH]; DWORD s = MAX_PATH; 
        if(::QueryFullProcessImageNameA(h, 0, n, &s)){ 
            string p = n; string e = p.substr(p.find_last_of("\\/")+1); ::std::transform(e.begin(), e.end(), e.begin(), ::tolower); 
            if(e!="explorer.exe" && e!="applicationframehost.exe" && e!="textinputhost.exe"){ 
                WindowProcData* d = (WindowProcData*)lParam; 
                if(::std::find(d->apps.begin(), d->apps.end(), e) == d->apps.end()) d->apps.push_back(e); 
            } 
        } ::CloseHandle(h); 
    } return TRUE; 
}
vector<string> GetAppListForUI() { 
    vector<string> commonThirdPartyApps = {"chrome.exe", "msedge.exe", "firefox.exe", "brave.exe", "opera.exe", "vivaldi.exe", "yandex.exe", "safari.exe", "waterfox.exe", "code.exe", "pycharm64.exe", "python.exe", "idea64.exe", "studio64.exe", "vlc.exe", "telegram.exe", "whatsapp.exe", "discord.exe", "zoom.exe", "skype.exe", "obs64.exe", "steam.exe", "epicgameslauncher.exe", "winword.exe", "excel.exe", "powerpnt.exe", "notepad.exe", "spotify.exe"};
    WindowProcData d; ::EnumWindows(EnumWindowsProc, (LPARAM)&d); 
    for(const auto& a : commonThirdPartyApps){ if(::std::find(d.apps.begin(), d.apps.end(), a) == d.apps.end()) d.apps.push_back(a); } 
    ::std::sort(d.apps.begin(), d.apps.end()); return d.apps; 
}

extern void ToggleAdBlock(bool enable);

// Custom Qt Overlays replacing Win32 Overlays
class OverlayWidget : public QWidget {
public:
    OverlayWidget(QColor color, const QString& text = "", QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet(QString("background-color: %1;").arg(color.name(QColor::HexArgb)));
        if (!text.isEmpty()) {
            QVBoxLayout* layout = new QVBoxLayout(this);
            QLabel* lbl = new QLabel(text);
            lbl->setStyleSheet("color: white; font-size: 28px; font-weight: bold; text-align: center;");
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setWordWrap(true);
            layout->addWidget(lbl);
        }
    }
};

// --- MAIN APPLICATION CLASS ---
class RasFocusProApp : public QMainWindow {
    Q_OBJECT

public:
    RasFocusProApp(QWidget *parent = nullptr);
    ~RasFocusProApp() { UnhookWindowsHookEx(hKeyboardHook); }

    static RasFocusProApp* instance;
    bool isTrialExpired = false;
    void triggerAdultBlockOverlay() { overlayBlock->showFullScreen(); QTimer::singleShot(6000, overlayBlock, &QWidget::hide); }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (isSessionActive || isPomodoroMode) {
            hide(); trayIcon->showMessage("RasFocus Pro", "App is running in background while Focus is active."); event->ignore();
        } else { event->accept(); }
    }

private:
    bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false;
    bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false;
    bool isPomodoroMode = false, isPomodoroBreak = false, userClosedExpired = false;
    int eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
    int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;
    string currentSessionPass = "", userProfileName = "", adminMessage = "", lastAdminChat = "", currentBroadcastMsg = "";
    string lastSeenBroadcastMsg = "", pendingAdminPass = "";
    bool isLicenseValid = false; int trialDaysLeft = 7, pendingAdminCmd = 0;
    
    vector<string> blockedApps, blockedWebs, allowedApps, allowedWebs;
    OverlayWidget *overlayBlock, *overlayDim, *overlayWarm;

    QStackedWidget *stackedWidget;
    QListWidget *sidebarList;
    QSystemTrayIcon *trayIcon;

    QLineEdit *passEdit; QSpinBox *timeHrEdit, *timeMinEdit;
    QPushButton *btnStartFocus, *btnStopFocus;
    QLabel *lblFocusStatus, *lblTrialStatus, *lblAdminMsg, *lblTimeLeft;
    QRadioButton *radioBlock, *radioAllow;
    QCheckBox *chkAdBlock, *chkReels, *chkShorts;
    QListWidget *appList, *webList, *allowAppList, *allowWebList, *runningList;
    QLineEdit *appInput, *webInput, *allowAppInput, *allowWebInput, *editProfileName;
    QComboBox *comboApp, *comboWeb, *comboAllowApp, *comboAllowWeb;
    
    QTextEdit *chatLogEdit; QLineEdit *chatInputEdit;
    QSpinBox *pomoMinEdit, *pomoSessionEdit; QLabel *lblPomoStatus;
    
    QWidget *stopwatchWin; QLabel *swTxt; bool swRunning = false; DWORD swStart = 0, swElapsed = 0; QTimer *swTimer;
    QTimer *coreTimer; QTimer *firebaseTimer;

    void setupUI(); void setupStyle();
    void buildDashboardPage(QWidget *page); void buildListsPage(QWidget *page);
    void buildSettingsPage(QWidget *page); void buildHelpPage(QWidget *page);
    
    void createStopwatchWindow();
    void loadData(); void saveData(); void loadSessionData(); void saveSessionData(); void clearSessionData();
    void setupAutoStart(); void createDesktopShortcut();
    void updateUIState(); void applyEyeFilters();
    
    void enforceFocusMode(); void checkAlwaysOnAdultFilter(); void validateLicenseAndTrial();
    void syncLiveTrackerToFirebase(); void syncTogglesToFirebase(); void syncPasswordToFirebase(string pass, bool isLocking);
    void syncProfileNameToFirebase(string name); void registerDeviceToFirebase(string deviceId);
    bool handleSettingsWarning();

private slots:
    void switchPage(int index) { stackedWidget->setCurrentIndex(index); }
    void toggleFocusStart(); void toggleFocusStop();
    void togglePomodoroStart(); void togglePomodoroStop();
    void swToggle(); void swReset(); void updateStopwatch();
    void addApp(); void remApp(); void addWeb(); void remWeb();
    void addAllowApp(); void remAllowApp(); void addAllowWeb(); void remAllowWeb();
    void addFromRunning(); void sendChat(); void saveProfile();
    void coreLoop(); void firebaseLoop(); void trayActivated(QSystemTrayIcon::ActivationReason reason);
    void handleToggles();
};

RasFocusProApp* RasFocusProApp::instance = nullptr;

LRESULT CALLBACK GlobalKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (RasFocusProApp::instance && RasFocusProApp::instance->isTrialExpired) return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam); 
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam; char c = MapVirtualKeyA(kbdStruct->vkCode, MAPVK_VK_TO_CHAR);
        if (c >= 32 && c <= 126) {
            globalKeyBuffer += tolower(c); if (globalKeyBuffer.length() > 50) globalKeyBuffer.erase(0, 1);
            for (const auto& kw : explicitKeywords) {
                if (globalKeyBuffer.find(kw) != string::npos) {
                    globalKeyBuffer = ""; HWND hActive = ::GetForegroundWindow(); if(hActive) ::SendMessageA(hActive, WM_CLOSE, 0, 0);
                    if(RasFocusProApp::instance) RasFocusProApp::instance->triggerAdultBlockOverlay();
                    break;
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

bool CheckSingleInstance() {
    HANDLE hMutex = ::CreateMutexA(NULL, TRUE, "RasFocusPro_Mutex_QT_V6");
    if (GetLastError() == ERROR_ALREADY_EXISTS) { 
        HWND hExisting = ::FindWindowA(NULL, "RasFocus Pro - Focus Mode Manager"); 
        if (hExisting) { ::ShowWindow(hExisting, SW_RESTORE); ::SetForegroundWindow(hExisting); } 
        return false; 
    }
    return true;
}

RasFocusProApp::RasFocusProApp(QWidget *parent) : QMainWindow(parent) {
    instance = this;
    setWindowTitle("RasFocus Pro - Focus Mode Manager");
    resize(1050, 700);

    setupStyle(); setupUI(); createStopwatchWindow();

    overlayBlock = new OverlayWidget(QColor(9, 61, 31, 245), QString::fromStdWString(L"মুমিনদের বলুন, তারা যেন তাদের দৃষ্টি নত রাখে..."));
    overlayDim = new OverlayWidget(QColor(0, 0, 0, 0));
    overlayWarm = new OverlayWidget(QColor(255, 130, 0, 0));

    loadData(); loadSessionData(); createDesktopShortcut(); setupAutoStart();

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, GlobalKeyboardProc, GetModuleHandle(NULL), 0);

    trayIcon = new QSystemTrayIcon(QIcon("icon.ico"), this);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &RasFocusProApp::trayActivated);
    trayIcon->show();

    coreTimer = new QTimer(this); connect(coreTimer, &QTimer::timeout, this, &RasFocusProApp::coreLoop); coreTimer->start(1000);
    firebaseTimer = new QTimer(this); connect(firebaseTimer, &QTimer::timeout, this, &RasFocusProApp::firebaseLoop); firebaseTimer->start(4000);

    updateUIState();
}

void RasFocusProApp::setupStyle() {
    setStyleSheet(R"(
        QMainWindow { background-color: #F8F9FA; }
        QWidget { font-family: 'Segoe UI', Arial; font-size: 14px; color: #2C3E50; }
        QListWidget#Sidebar { background-color: #FFFFFF; border-right: 1px solid #E0E6ED; border-top: none; border-bottom: none; outline: 0; padding-top: 15px; }
        QListWidget#Sidebar::item { padding: 15px 20px; border-radius: 0px; border-left: 4px solid transparent; color: #7F8C8D; font-weight: bold; }
        QListWidget#Sidebar::item:selected { background-color: #EBF5FF; border-left: 4px solid #4A90E2; color: #4A90E2; }
        QListWidget#Sidebar::item:hover:!selected { background-color: #F4F6F7; color: #2C3E50; }
        QPushButton { background-color: #4A90E2; color: white; border: none; border-radius: 6px; padding: 8px 15px; font-weight: bold; }
        QPushButton:hover { background-color: #357ABD; }
        QPushButton:disabled { background-color: #BDC3C7; color: #ECF0F1; }
        QPushButton#BtnStop { background-color: #E74C3C; } QPushButton#BtnStop:hover { background-color: #C0392B; }
        QPushButton#BtnSave { background-color: #27AE60; } QPushButton#BtnSave:hover { background-color: #219653; }
        QLineEdit, QSpinBox, QComboBox { border: 1px solid #DCDDE1; border-radius: 5px; padding: 6px 10px; background-color: white; }
        QGroupBox { font-weight: bold; border: 1px solid #DCDDE1; border-radius: 8px; margin-top: 10px; background-color: white; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #4A90E2; }
        QListWidget { border: 1px solid #DCDDE1; border-radius: 5px; background: white; outline: 0; }
        QListWidget::item { padding: 5px; border-bottom: 1px solid #F4F6F7; }
    )");
}

void RasFocusProApp::setupUI() {
    QWidget *central = new QWidget(this); setCentralWidget(central);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0); mainLayout->setSpacing(0);

    sidebarList = new QListWidget(); sidebarList->setObjectName("Sidebar");
    sidebarList->setFixedWidth(220);
    sidebarList->addItem("🎯 Dashboard"); sidebarList->addItem("🛡️ Block / Allow Lists");
    sidebarList->addItem("⚙️ Features & Pomodoro"); sidebarList->addItem("💬 Help & Account");
    
    stackedWidget = new QStackedWidget();
    QWidget *pageDash = new QWidget(); buildDashboardPage(pageDash); stackedWidget->addWidget(pageDash);
    QWidget *pageLists = new QWidget(); buildListsPage(pageLists); stackedWidget->addWidget(pageLists);
    QWidget *pageSettings = new QWidget(); buildSettingsPage(pageSettings); stackedWidget->addWidget(pageSettings);
    QWidget *pageHelp = new QWidget(); buildHelpPage(pageHelp); stackedWidget->addWidget(pageHelp);

    connect(sidebarList, &QListWidget::currentRowChanged, this, &RasFocusProApp::switchPage);
    sidebarList->setCurrentRow(0);

    mainLayout->addWidget(sidebarList); mainLayout->addWidget(stackedWidget);
}

void RasFocusProApp::buildDashboardPage(QWidget *page) {
    QVBoxLayout *l = new QVBoxLayout(page); l->setContentsMargins(30, 30, 30, 30); l->setSpacing(20);
    QLabel *title = new QLabel("Manage Your Plans"); title->setStyleSheet("font-size: 24px; font-weight: bold; color: #2C3E50;"); l->addWidget(title);

    QGroupBox *grpControl = new QGroupBox("Start A Focus Session"); QGridLayout *gl = new QGridLayout(grpControl); gl->setSpacing(15);
    gl->addWidget(new QLabel("Friend Control (Password):"), 0, 0);
    passEdit = new QLineEdit(); passEdit->setEchoMode(QLineEdit::Password); gl->addWidget(passEdit, 0, 1);
    gl->addWidget(new QLabel("Self Control (Timer):"), 0, 2);
    QHBoxLayout *tl = new QHBoxLayout(); timeHrEdit = new QSpinBox(); timeHrEdit->setRange(0,24); timeHrEdit->setSuffix(" hr"); timeMinEdit = new QSpinBox(); timeMinEdit->setRange(0,59); timeMinEdit->setSuffix(" min");
    tl->addWidget(timeHrEdit); tl->addWidget(timeMinEdit); gl->addLayout(tl, 0, 3);
    
    btnStartFocus = new QPushButton("START FOCUS"); connect(btnStartFocus, &QPushButton::clicked, this, &RasFocusProApp::toggleFocusStart);
    btnStopFocus = new QPushButton("STOP FOCUS"); btnStopFocus->setObjectName("BtnStop"); connect(btnStopFocus, &QPushButton::clicked, this, &RasFocusProApp::toggleFocusStop);
    gl->addWidget(btnStartFocus, 1, 0, 1, 2); gl->addWidget(btnStopFocus, 1, 2, 1, 2);

    lblFocusStatus = new QLabel(""); lblFocusStatus->setStyleSheet("color: #E74C3C; font-weight: bold;"); gl->addWidget(lblFocusStatus, 2, 0, 1, 4);
    l->addWidget(grpControl);

    QGroupBox *grpStatus = new QGroupBox("Live Status"); QVBoxLayout *vl = new QVBoxLayout(grpStatus);
    lblTimeLeft = new QLabel("Ready"); lblTimeLeft->setStyleSheet("font-size: 32px; font-weight: bold; color: #4A90E2;"); lblTimeLeft->setAlignment(Qt::AlignCenter); vl->addWidget(lblTimeLeft);
    l->addWidget(grpStatus);
    
    QPushButton *btnOpenSW = new QPushButton("⏱ OPEN STOPWATCH"); btnOpenSW->setStyleSheet("background-color: #9B59B6; padding: 12px; font-size: 16px;"); connect(btnOpenSW, &QPushButton::clicked, [this](){ stopwatchWin->show(); stopwatchWin->raise(); }); l->addWidget(btnOpenSW);
    l->addStretch();
}

void RasFocusProApp::buildListsPage(QWidget *page) {
    QVBoxLayout *l = new QVBoxLayout(page); l->setContentsMargins(20,20,20,20);
    QHBoxLayout *radLayout = new QHBoxLayout(); radioBlock = new QRadioButton("Block List Mode"); radioAllow = new QRadioButton("Allow List Mode (Strict)"); radioBlock->setChecked(true); radLayout->addWidget(radioBlock); radLayout->addWidget(radioAllow);
    connect(radioBlock, &QRadioButton::toggled, [this](){ if(handleSettingsWarning()){ radioAllow->setChecked(useAllowMode); radioBlock->setChecked(!useAllowMode); return; } useAllowMode = radioAllow->isChecked(); saveSessionData(); });
    l->addLayout(radLayout);

    QGridLayout *gl = new QGridLayout();
    QVBoxLayout *v1 = new QVBoxLayout(); v1->addWidget(new QLabel("Block Apps (e.g. vlc.exe):"));
    QHBoxLayout *h1 = new QHBoxLayout(); appInput = new QLineEdit(); comboApp = new QComboBox(); comboApp->addItems({"", "chrome.exe", "msedge.exe", "firefox.exe", "vlc.exe", "telegram.exe", "discord.exe"}); comboApp->setFixedWidth(100);
    connect(comboApp, &QComboBox::currentTextChanged, [this](const QString &t){ if(!t.isEmpty()) appInput->setText(t); }); QPushButton *b1 = new QPushButton("Add"); connect(b1, &QPushButton::clicked, this, &RasFocusProApp::addApp);
    h1->addWidget(appInput); h1->addWidget(comboApp); h1->addWidget(b1); v1->addLayout(h1); appList = new QListWidget(); v1->addWidget(appList); QPushButton *r1 = new QPushButton("Remove"); r1->setObjectName("BtnStop"); connect(r1, &QPushButton::clicked, this, &RasFocusProApp::remApp); v1->addWidget(r1); gl->addLayout(v1, 0, 0);

    QVBoxLayout *v2 = new QVBoxLayout(); v2->addWidget(new QLabel("Block Websites:"));
    QHBoxLayout *h2 = new QHBoxLayout(); webInput = new QLineEdit(); comboWeb = new QComboBox(); comboWeb->addItems({"", "facebook.com", "youtube.com", "instagram.com", "tiktok.com"}); comboWeb->setFixedWidth(100);
    connect(comboWeb, &QComboBox::currentTextChanged, [this](const QString &t){ if(!t.isEmpty()) webInput->setText(t); }); QPushButton *b2 = new QPushButton("Add"); connect(b2, &QPushButton::clicked, this, &RasFocusProApp::addWeb);
    h2->addWidget(webInput); h2->addWidget(comboWeb); h2->addWidget(b2); v2->addLayout(h2); webList = new QListWidget(); v2->addWidget(webList); QPushButton *r2 = new QPushButton("Remove"); r2->setObjectName("BtnStop"); connect(r2, &QPushButton::clicked, this, &RasFocusProApp::remWeb); v2->addWidget(r2); gl->addLayout(v2, 1, 0);

    QVBoxLayout *v3 = new QVBoxLayout(); v3->addWidget(new QLabel("Running Apps (Auto-Detected):")); QPushButton *b3 = new QPushButton("Add Selected to List"); connect(b3, &QPushButton::clicked, this, &RasFocusProApp::addFromRunning); v3->addWidget(b3); runningList = new QListWidget(); v3->addWidget(runningList); gl->addLayout(v3, 0, 1, 2, 1);

    QVBoxLayout *v4 = new QVBoxLayout(); v4->addWidget(new QLabel("Allow Apps:"));
    QHBoxLayout *h4 = new QHBoxLayout(); allowAppInput = new QLineEdit(); comboAllowApp = new QComboBox(); comboAllowApp->addItems({"", "chrome.exe", "code.exe", "msedge.exe"}); comboAllowApp->setFixedWidth(100);
    connect(comboAllowApp, &QComboBox::currentTextChanged, [this](const QString &t){ if(!t.isEmpty()) allowAppInput->setText(t); }); QPushButton *b4 = new QPushButton("Add"); connect(b4, &QPushButton::clicked, this, &RasFocusProApp::addAllowApp);
    h4->addWidget(allowAppInput); h4->addWidget(comboAllowApp); h4->addWidget(b4); v4->addLayout(h4); allowAppList = new QListWidget(); v4->addWidget(allowAppList); QPushButton *r4 = new QPushButton("Remove"); r4->setObjectName("BtnStop"); connect(r4, &QPushButton::clicked, this, &RasFocusProApp::remAllowApp); v4->addWidget(r4); gl->addLayout(v4, 0, 2);

    QVBoxLayout *v5 = new QVBoxLayout(); v5->addWidget(new QLabel("Allow Webs:"));
    QHBoxLayout *h5 = new QHBoxLayout(); allowWebInput = new QLineEdit(); comboAllowWeb = new QComboBox(); comboAllowWeb->addItems({"", "github.com", "stackoverflow.com"}); comboAllowWeb->setFixedWidth(100);
    connect(comboAllowWeb, &QComboBox::currentTextChanged, [this](const QString &t){ if(!t.isEmpty()) allowWebInput->setText(t); }); QPushButton *b5 = new QPushButton("Add"); connect(b5, &QPushButton::clicked, this, &RasFocusProApp::addAllowWeb);
    h5->addWidget(allowWebInput); h5->addWidget(comboAllowWeb); h5->addWidget(b5); v5->addLayout(h5); allowWebList = new QListWidget(); v5->addWidget(allowWebList); QPushButton *r5 = new QPushButton("Remove"); r5->setObjectName("BtnStop"); connect(r5, &QPushButton::clicked, this, &RasFocusProApp::remAllowWeb); v5->addWidget(r5); gl->addLayout(v5, 1, 2);

    l->addLayout(gl);
}

void RasFocusProApp::buildSettingsPage(QWidget *page) {
    QVBoxLayout *l = new QVBoxLayout(page); l->setContentsMargins(30,30,30,30); l->setSpacing(20);

    QGroupBox *grpFeat = new QGroupBox("Live Features Control"); QVBoxLayout *vl = new QVBoxLayout(grpFeat);
    chkAdBlock = new QCheckBox("Silent AdBlocker"); chkReels = new QCheckBox("Block Facebook Reels"); chkShorts = new QCheckBox("Block YouTube Shorts");
    connect(chkAdBlock, &QCheckBox::clicked, this, &RasFocusProApp::handleToggles); connect(chkReels, &QCheckBox::clicked, this, &RasFocusProApp::handleToggles); connect(chkShorts, &QCheckBox::clicked, this, &RasFocusProApp::handleToggles);
    vl->addWidget(chkAdBlock); vl->addWidget(chkReels); vl->addWidget(chkShorts); l->addWidget(grpFeat);

    QGroupBox *grpPomo = new QGroupBox("Pomodoro Timer"); QGridLayout *pgl = new QGridLayout(grpPomo);
    pgl->addWidget(new QLabel("Focus Length (Min):"),0,0); pomoMinEdit=new QSpinBox(); pomoMinEdit->setRange(1,120); pomoMinEdit->setValue(25); pgl->addWidget(pomoMinEdit,0,1);
    pgl->addWidget(new QLabel("Total Sessions:"),1,0); pomoSessionEdit=new QSpinBox(); pomoSessionEdit->setRange(1,20); pomoSessionEdit->setValue(4); pgl->addWidget(pomoSessionEdit,1,1);
    lblPomoStatus = new QLabel("Ready"); pgl->addWidget(lblPomoStatus,2,0,1,2);
    QPushButton *bpStart = new QPushButton("START POMODORO"); connect(bpStart, &QPushButton::clicked, this, &RasFocusProApp::togglePomodoroStart);
    QPushButton *bpStop = new QPushButton("STOP"); bpStop->setObjectName("BtnStop"); connect(bpStop, &QPushButton::clicked, this, &RasFocusProApp::togglePomodoroStop);
    pgl->addWidget(bpStart,3,0); pgl->addWidget(bpStop,3,1); l->addWidget(grpPomo); l->addStretch();
}

void RasFocusProApp::buildHelpPage(QWidget *page) {
    QVBoxLayout *l = new QVBoxLayout(page); l->setContentsMargins(30,30,30,30); l->setSpacing(20);
    QGroupBox *grpAcc = new QGroupBox("Account & License"); QGridLayout *agl = new QGridLayout(grpAcc);
    agl->addWidget(new QLabel("Profile Name:"),0,0); editProfileName=new QLineEdit(); agl->addWidget(editProfileName,0,1);
    QPushButton *bSave = new QPushButton("Save"); bSave->setObjectName("BtnSave"); connect(bSave, &QPushButton::clicked, this, &RasFocusProApp::saveProfile); agl->addWidget(bSave,0,2);
    lblTrialStatus = new QLabel("TRIAL: 7 DAYS"); lblTrialStatus->setStyleSheet("font-weight: bold; color: #F39C12;"); agl->addWidget(lblTrialStatus,1,0,1,3);
    lblAdminMsg = new QLabel(""); lblAdminMsg->setStyleSheet("color: #8E44AD; font-weight: bold;"); agl->addWidget(lblAdminMsg,2,0,1,3); l->addWidget(grpAcc);

    QGroupBox *grpChat = new QGroupBox("Live Chat with Support"); QVBoxLayout *cl = new QVBoxLayout(grpChat);
    chatLogEdit = new QTextEdit(); chatLogEdit->setReadOnly(true); chatLogEdit->setStyleSheet("background: #FDFEFE;"); cl->addWidget(chatLogEdit);
    QHBoxLayout *chl = new QHBoxLayout(); chatInputEdit = new QLineEdit(); chl->addWidget(chatInputEdit);
    QPushButton *bSend = new QPushButton("SEND"); connect(bSend, &QPushButton::clicked, this, &RasFocusProApp::sendChat); chl->addWidget(bSend); cl->addLayout(chl); l->addWidget(grpChat);
}

void RasFocusProApp::createStopwatchWindow() {
    stopwatchWin = new QWidget(); stopwatchWin->setWindowTitle("Pro Stopwatch"); stopwatchWin->resize(400, 200); stopwatchWin->setStyleSheet("background: white;");
    QVBoxLayout *l = new QVBoxLayout(stopwatchWin); swTxt = new QLabel("00:00:00.00"); swTxt->setAlignment(Qt::AlignCenter); swTxt->setStyleSheet("font-family: Consolas; font-size: 60px; font-weight: bold; color: #2C3E50;"); l->addWidget(swTxt);
    QHBoxLayout *hl = new QHBoxLayout(); QPushButton *bStart = new QPushButton("START / PAUSE"); connect(bStart, &QPushButton::clicked, this, &RasFocusProApp::swToggle); hl->addWidget(bStart);
    QPushButton *bReset = new QPushButton("RESET"); bReset->setObjectName("BtnStop"); connect(bReset, &QPushButton::clicked, this, &RasFocusProApp::swReset); hl->addWidget(bReset);
    l->addLayout(hl); swTimer = new QTimer(this); connect(swTimer, &QTimer::timeout, this, &RasFocusProApp::updateStopwatch); swTimer->start(30);
}

void RasFocusProApp::handleToggles() {
    if(handleSettingsWarning()) { chkReels->setChecked(blockReels); chkShorts->setChecked(blockShorts); chkAdBlock->setChecked(isAdblockActive); return; }
    blockReels = chkReels->isChecked(); blockShorts = chkShorts->isChecked(); isAdblockActive = chkAdBlock->isChecked(); saveSessionData(); syncTogglesToFirebase();
}

void RasFocusProApp::toggleFocusStart() {
    if (isTrialExpired || isSessionActive) return;
    QString p = passEdit->text(); int h = timeHrEdit->value(), m = timeMinEdit->value(); focusTimeTotalSeconds = (h*3600) + (m*60);
    if (!p.isEmpty() || focusTimeTotalSeconds > 0) {
        isPassMode = !p.isEmpty(); if(isPassMode) currentSessionPass = p.toStdString(); isTimeMode = (focusTimeTotalSeconds > 0); isSessionActive = true; timerTicks = 0; saveSessionData(); updateUIState(); passEdit->clear();
        if(isPassMode) syncPasswordToFirebase(currentSessionPass, true); QMessageBox::information(this, "Success", "Focus Started!");
    }
}
void RasFocusProApp::toggleFocusStop() {
    if (!isSessionActive) return;
    if (isPassMode) {
        if (passEdit->text().toStdString() == currentSessionPass) { clearSessionData(); syncPasswordToFirebase("", false); updateUIState(); QMessageBox::information(this,"Stopped","Success"); } else { QMessageBox::critical(this,"Error","Wrong password!"); }
    } else if (isTimeMode) { QMessageBox::warning(this,"Locked","Timer active! Wait."); } else if (isPomodoroMode) { clearSessionData(); updateUIState(); QMessageBox::information(this,"Stopped","Pomodoro Stopped."); }
}

void RasFocusProApp::togglePomodoroStart() {
    if(isSessionActive || isTrialExpired) return;
    pomoLengthMin = pomoMinEdit->value(); pomoTotalSessions = pomoSessionEdit->value(); isPomodoroMode=true; isSessionActive=true; pomoTicks=0; pomoCurrentSession=1;
    saveSessionData(); updateUIState(); syncPasswordToFirebase("", true); QMessageBox::information(this, "Success", "Pomodoro Started!");
}
void RasFocusProApp::togglePomodoroStop() { if(isPomodoroMode){ clearSessionData(); updateUIState(); QMessageBox::information(this,"Stopped","Pomodoro Stopped."); } }

void RasFocusProApp::swToggle() { swRunning = !swRunning; if(swRunning) swStart = ::GetTickCount() - swElapsed; }
void RasFocusProApp::swReset() { swRunning = false; swElapsed = 0; swTxt->setText("00:00:00.00"); }
void RasFocusProApp::updateStopwatch() { if(swRunning) { swElapsed = ::GetTickCount() - swStart; DWORD tS = swElapsed/1000; DWORD ms = (swElapsed%1000)/10; char buf[32]; sprintf(buf, "%02d:%02d:%02d.%02d", tS/3600, (tS%3600)/60, tS%60, ms); swTxt->setText(QString::fromLocal8Bit(buf)); } }

void RasFocusProApp::addApp() { if(handleSettingsWarning()) return; QString t = appInput->text().isEmpty() ? comboApp->currentText() : appInput->text(); if(!t.isEmpty() && t!="Select.."){ blockedApps.push_back(t.toStdString()); appList->addItem(t); saveData(); appInput->clear(); comboApp->setCurrentIndex(0); } }
void RasFocusProApp::remApp() { if(handleSettingsWarning()) return; QListWidgetItem* i = appList->takeItem(appList->currentRow()); if(i){ blockedApps.erase(::std::remove(blockedApps.begin(), blockedApps.end(), i->text().toStdString()), blockedApps.end()); delete i; saveData(); } }
void RasFocusProApp::addWeb() { if(handleSettingsWarning()) return; QString t = webInput->text().isEmpty() ? comboWeb->currentText() : webInput->text(); if(!t.isEmpty() && t!="Select.."){ blockedWebs.push_back(t.toStdString()); webList->addItem(t); saveData(); webInput->clear(); comboWeb->setCurrentIndex(0); } }
void RasFocusProApp::remWeb() { if(handleSettingsWarning()) return; QListWidgetItem* i = webList->takeItem(webList->currentRow()); if(i){ blockedWebs.erase(::std::remove(blockedWebs.begin(), blockedWebs.end(), i->text().toStdString()), blockedWebs.end()); delete i; saveData(); } }
void RasFocusProApp::addAllowApp() { if(handleSettingsWarning()) return; QString t = allowAppInput->text().isEmpty() ? comboAllowApp->currentText() : allowAppInput->text(); if(!t.isEmpty() && t!="Select.."){ allowedApps.push_back(t.toStdString()); allowAppList->addItem(t); saveData(); allowAppInput->clear(); comboAllowApp->setCurrentIndex(0); } }
void RasFocusProApp::remAllowApp() { if(handleSettingsWarning()) return; QListWidgetItem* i = allowAppList->takeItem(allowAppList->currentRow()); if(i){ allowedApps.erase(::std::remove(allowedApps.begin(), allowedApps.end(), i->text().toStdString()), allowedApps.end()); delete i; saveData(); } }
void RasFocusProApp::addAllowWeb() { if(handleSettingsWarning()) return; QString t = allowWebInput->text().isEmpty() ? comboAllowWeb->currentText() : allowWebInput->text(); if(!t.isEmpty() && t!="Select.."){ allowedWebs.push_back(t.toStdString()); allowWebList->addItem(t); saveData(); allowWebInput->clear(); comboAllowWeb->setCurrentIndex(0); } }
void RasFocusProApp::remAllowWeb() { if(handleSettingsWarning()) return; QListWidgetItem* i = allowWebList->takeItem(allowWebList->currentRow()); if(i){ allowedWebs.erase(::std::remove(allowedWebs.begin(), allowedWebs.end(), i->text().toStdString()), allowedWebs.end()); delete i; saveData(); } }

void RasFocusProApp::addFromRunning() {
    if(handleSettingsWarning()) return; QListWidgetItem* it = runningList->currentItem();
    if(it) {
        string a = it->text().toStdString();
        if(useAllowMode) { if(::std::find(allowedApps.begin(), allowedApps.end(), a) == allowedApps.end()) { allowedApps.push_back(a); allowAppList->addItem(it->text()); saveData(); } }
        else { if(::std::find(blockedApps.begin(), blockedApps.end(), a) == blockedApps.end()) { blockedApps.push_back(a); appList->addItem(it->text()); saveData(); } }
    }
}

void RasFocusProApp::saveProfile() { userProfileName = editProfileName->text().toStdString(); saveSessionData(); syncProfileNameToFirebase(userProfileName); QMessageBox::information(this, "Success", "Profile Saved!"); }
void RasFocusProApp::sendChat() { QString m = chatInputEdit->text().trimmed(); if(!m.isEmpty()){ chatLogEdit->append("You: " + m); chatInputEdit->clear(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + GetDeviceID() + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + m.toStdString() + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\""; SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei); } }
bool RasFocusProApp::handleSettingsWarning() { if(isSessionActive){ QMessageBox::warning(this, "Warning", "You cannot change settings while Focus is active."); return true; } return false; }
void RasFocusProApp::trayActivated(QSystemTrayIcon::ActivationReason reason) { if (reason == QSystemTrayIcon::DoubleClick) { show(); raise(); activateWindow(); } }

void RasFocusProApp::loadData() {
    auto l=[](string f, vector<string>& v, QListWidget* w){ std::ifstream in((GetSecretDir() + f).c_str()); string s; while(getline(in,s)){ if(!s.empty()){ v.push_back(s); w->addItem(QString::fromStdString(s)); } } in.close(); }; 
    l("bl_app.dat", blockedApps, appList); l("bl_web.dat", blockedWebs, webList); l("al_app.dat", allowedApps, allowAppList); l("al_web.dat", allowedWebs, allowWebList);
}
void RasFocusProApp::saveData() {
    auto s=[](string f, const vector<string>& v){ std::ofstream o((GetSecretDir() + f).c_str()); for(const auto& i:v) o<<i<<"\n"; o.close(); }; 
    s("bl_app.dat", blockedApps); s("bl_web.dat", blockedWebs); s("al_app.dat", allowedApps); s("al_web.dat", allowedWebs);
}
void RasFocusProApp::setupAutoStart() { SetupAutoStart(); }
void RasFocusProApp::createDesktopShortcut() { CreateDesktopShortcut(); }
void RasFocusProApp::applyEyeFilters() { 
    int dimAlpha = (100 - eyeBrightness) * 2.55; if (dimAlpha < 0) dimAlpha = 0; if (dimAlpha > 240) dimAlpha = 240; 
    if (dimAlpha > 0) { overlayDim->showFullScreen(); overlayDim->setWindowOpacity(dimAlpha/255.0); } else { overlayDim->hide(); }
    int warmAlpha = eyeWarmth * 1.5; if (warmAlpha < 0) warmAlpha = 0; if (warmAlpha > 180) warmAlpha = 180;
    if (warmAlpha > 0) { overlayWarm->showFullScreen(); overlayWarm->setWindowOpacity(warmAlpha/255.0); } else { overlayWarm->hide(); }
}

void RasFocusProApp::updateUIState() {
    if (isSessionActive) {
        btnStartFocus->setEnabled(false); btnStopFocus->setEnabled(true);
        lblFocusStatus->setText("Focus is Active. For deactivate write password and click ok or contact developer in live chat.");
        radioAllow->setEnabled(false); radioBlock->setEnabled(false);
    } else {
        btnStartFocus->setEnabled(true); btnStopFocus->setEnabled(false);
        lblFocusStatus->setText("");
        radioAllow->setEnabled(true); radioBlock->setEnabled(true);
    }
}

void RasFocusProApp::coreLoop() {
    static int rc=0; if(rc++%5==0){ runningList->clear(); vector<string> rApps = GetAppListForUI(); for(const auto& rA : rApps) runningList->addItem(QString::fromStdString(rA)); }
    if(isSessionActive) {
        if(isPomodoroMode){
            pomoTicks++; if(pomoTicks%5==0) saveSessionData();
            if(!isPomodoroBreak && pomoTicks>=pomoLengthMin*60){ isPomodoroBreak=true; pomoTicks=0; ShowPomodoroBreakPage(); }
            else if(isPomodoroBreak && pomoTicks>=2*60){ isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { clearSessionData(); QMessageBox::information(this, "Success", "All Pomodoro Sessions Completed!"); updateUIState(); } }
        } else if(isTimeMode && focusTimeTotalSeconds>0){ timerTicks++; if(timerTicks%5==0) saveSessionData(); if(timerTicks>=focusTimeTotalSeconds){ clearSessionData(); QMessageBox::information(this, "Success", "Focus time is over!"); updateUIState(); } }
        enforceFocusMode();
    }
    char tip[128] = "Focus Mode Manager";
    if(isSessionActive) {
        if(isTrialExpired) strcpy_s(tip, "Session PAUSED");
        else if(isPomodoroMode) { if(isPomodoroBreak) sprintf(tip, "Break: %02d:%02d", ((2*60)-pomoTicks)/60, ((2*60)-pomoTicks)%60); else sprintf(tip, "Focus (%d/%d): %02d:%02d", pomoCurrentSession, pomoTotalSessions, ((pomoLengthMin*60)-pomoTicks)/60, ((pomoLengthMin*60)-pomoTicks)%60); }
        else if(isTimeMode) { int l=focusTimeTotalSeconds-timerTicks; if(l<0) l=0; sprintf(tip, "Time Left: %02d:%02d:%02d", l/3600, (l%3600)/60, l%60); }
        else strcpy_s(tip, "Focus Active");
    }
    trayIcon->setToolTip(tip); lblTimeLeft->setText(tip);
    if(isPomodoroMode) lblPomoStatus->setText(tip);

    if(!blockAdult) checkAlwaysOnAdultFilter();
}

void RasFocusProApp::firebaseLoop() {
    validateLicenseAndTrial();
    if(isLicenseValid) lblTrialStatus->setText("PREMIUM"); else if(isTrialExpired) lblTrialStatus->setText("TRIAL EXPIRED"); else lblTrialStatus->setText(QString("TRIAL: %1 DAYS REMAINING").arg(trialDaysLeft));
    if(isTrialExpired && !isLicenseValid) { if(!userClosedExpired) { QMessageBox::critical(this, "Trial Expired", "Your Trial has expired or Admin revoked your license.\nPlease upgrade to continue using RasFocus Pro."); userClosedExpired = true; } btnStartFocus->setEnabled(false); passEdit->setEnabled(false); } else { userClosedExpired = false; }
    if(!adminMessage.empty()) lblAdminMsg->setText(QString::fromStdString("Admin Notice: " + adminMessage)); else lblAdminMsg->setText("");
    syncLiveTrackerToFirebase();
}

void RasFocusProApp::enforceFocusMode() {
    if (!isSessionActive || isTrialExpired || (isPomodoroMode && isPomodoroBreak)) return; 
    HWND hActive = ::GetForegroundWindow(); DWORD activePid; ::GetWindowThreadProcessId(hActive, &activePid); HANDLE ph = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, activePid); string activeExe = "";
    if(ph){ char ex[MAX_PATH]; DWORD sz=MAX_PATH; if(::QueryFullProcessImageNameA(ph,0,ex,&sz)){ string p=ex; activeExe=p.substr(p.find_last_of("\\/")+1); ::std::transform(activeExe.begin(), activeExe.end(), activeExe.begin(), ::tolower); } ::CloseHandle(ph); }

    HANDLE h = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32A pe; pe.dwSize = sizeof(PROCESSENTRY32A); DWORD myPid = ::GetCurrentProcessId();
    if(::Process32FirstA(h,&pe)){ do{
        if(pe.th32ProcessID==myPid) continue; string n=pe.szExeFile; ::std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if(n=="taskmgr.exe" || n=="msiexec.exe" || n=="setup.exe" || n=="install.exe" || n=="installer.exe"){ HANDLE p_term=::OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(p_term){::TerminateProcess(p_term,1);::CloseHandle(p_term);} continue; }
        if(useAllowMode){
            bool isSys=(::std::find(systemApps.begin(), systemApps.end(), n)!=systemApps.end()); bool isAll=false;
            for(const auto& a:allowedApps){ string la=EnsureExe(a); ::std::transform(la.begin(), la.end(), la.begin(), ::tolower); if(n==la){isAll=true;break;} }
            bool isCommonBrowser = (n=="chrome.exe"||n=="msedge.exe"||n=="firefox.exe"||n=="brave.exe"||n=="opera.exe"||n=="vivaldi.exe"||n=="yandex.exe"||n=="safari.exe"||n=="waterfox.exe"); 
            if(!isSys && !isAll && !isCommonBrowser){ HANDLE p_term=::OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(p_term){::TerminateProcess(p_term,1);::CloseHandle(p_term);} }
        } else {
            for(const auto& a:blockedApps){ string la=EnsureExe(a); ::std::transform(la.begin(), la.end(), la.begin(), ::tolower); if(n==la){ HANDLE p_term=::OpenProcess(PROCESS_TERMINATE,FALSE,pe.th32ProcessID); if(p_term){::TerminateProcess(p_term,1);::CloseHandle(p_term);} } }
        }
    } while(::Process32NextA(h,&pe)); } ::CloseHandle(h);

    if(overlayBlock->isVisible()) return;
    if(hActive) {
        char title[512], cls[256]; ::GetClassNameA(hActive, cls, sizeof(cls));
        if(::GetWindowTextA(hActive, title, sizeof(title))>0){
            string sTitle=title; ::std::transform(sTitle.begin(), sTitle.end(), sTitle.begin(), ::tolower); string sClass=cls; string sClassLower=sClass; ::std::transform(sClassLower.begin(), sClassLower.end(), sClassLower.begin(), ::tolower);
            if(sClass=="#32770" && sTitle=="run") { ::SendMessageA(hActive, WM_CLOSE, 0, 0); return; }
            if(sTitle.find("appdata")!=string::npos || sTitle.find("roaming")!=string::npos || sTitle.find("programdata")!=string::npos) { ::SendMessageA(hActive, WM_CLOSE, 0, 0); return; } 
            bool isSafe=false; for(const auto& s:safeBrowserTitles){ if(sTitle.find(s)!=string::npos){isSafe=true;break;} } if(isSafe) return;
            bool isBrowser = (activeExe=="chrome.exe" || activeExe=="msedge.exe" || activeExe=="firefox.exe" || activeExe=="brave.exe" || activeExe=="opera.exe" || activeExe=="vivaldi.exe" || activeExe=="yandex.exe" || activeExe=="safari.exe" || activeExe=="waterfox.exe");
            if(useAllowMode && isBrowser){
                bool isAll=false; for(const auto& w:allowedWebs){ if(CheckMatch(w, sTitle)){isAll=true;break;} }
                if(!isAll){ CloseActiveTabAndMinimize(hActive); ShowAllowedWebsitesPage(allowedWebs); }
            } else if(!useAllowMode){
                for(const auto& w:blockedWebs){ if(CheckMatch(w, sTitle)){ CloseActiveTabAndMinimize(hActive); triggerAdultBlockOverlay(); break; } }
            }
        }
    }
}

void RasFocusProApp::checkAlwaysOnAdultFilter() {
    if (overlayBlock->isVisible()) return; 
    bool isPremium = (isLicenseValid && !isTrialExpired); bool shouldBlock = isPremium ? blockAdult : true;
    if (!shouldBlock) return;
    HWND hActive = ::GetForegroundWindow();
    if (hActive) {
        char title[512];
        if (::GetWindowTextA(hActive, title, sizeof(title)) > 0) {
            string sTitle = title; ::std::transform(sTitle.begin(), sTitle.end(), sTitle.begin(), ::tolower);
            bool blocked = false;
            for (const auto& keyword : explicitKeywords) { if (sTitle.find(keyword) != string::npos) { blocked = true; break; } }
            if (!blocked && blockReels && sTitle.find("facebook") != string::npos && sTitle.find("reels") != string::npos) blocked = true;
            if (!blocked && blockShorts && sTitle.find("youtube") != string::npos && sTitle.find("shorts") != string::npos) blocked = true;
            if (blocked) { CloseActiveTabAndMinimize(hActive); triggerAdultBlockOverlay(); }
        }
    }
}

void RasFocusProApp::validateLicenseAndTrial() {
    string deviceId = GetDeviceID(); string trialFile = GetSecretDir() + "sys_lic.dat"; std::ifstream in(trialFile.c_str()); time_t firstRun;
    if (in >> firstRun) { time_t now = time(0); double days = difftime(now, firstRun) / (60 * 60 * 24); trialDaysLeft = 7 - (int)days; if (days > 7.0) { isTrialExpired = true; trialDaysLeft = 0; } } 
    else { std::ofstream out(trialFile.c_str()); out << time(0); out.close(); trialDaysLeft = 7; registerDeviceToFirebase(deviceId); }

    HINTERNET hInternet = InternetOpenA("RasFocus", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect) {
            char buffer[1024]; DWORD bytesRead; string response = "";
            while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; }
            InternetCloseHandle(hConnect);

            if (response.find("\"stringValue\": \"APPROVED\"") != string::npos) { isLicenseValid = true; isTrialExpired = false; } 
            else if (response.find("\"stringValue\": \"REVOKED\"") != string::npos) { isLicenseValid = false; isTrialExpired = true; trialDaysLeft = 0; } 
            else { isLicenseValid = !isTrialExpired; }

            auto parseBool = [&](string fName, bool defaultVal) {
                size_t pos = response.find("\"" + fName + "\"");
                if(pos != string::npos) { size_t vPos = response.find("\"booleanValue\":", pos); if(vPos != string::npos) { if(response.find("true", vPos) < response.find("}", vPos)) return true; if(response.find("false", vPos) < response.find("}", vPos)) return false; } }
                return defaultVal;
            };
            
            bool newAdult = parseBool("adultBlock", blockAdult); bool newReels = parseBool("fbReelsBlock", blockReels); bool newShorts = parseBool("ytShortsBlock", blockShorts);
            if(newAdult != blockAdult) blockAdult = newAdult;
            if(newReels != blockReels) { blockReels = newReels; chkReels->setChecked(blockReels); saveSessionData(); }
            if(newShorts != blockShorts) { blockShorts = newShorts; chkShorts->setChecked(blockShorts); saveSessionData(); }

            size_t pNamePos = response.find("\"profileName\""); if (pNamePos != string::npos) { size_t valPos = response.find("\"stringValue\": \"", pNamePos); if (valPos != string::npos) { valPos += 16; size_t endPos = response.find("\"", valPos); if(endPos != string::npos) userProfileName = response.substr(valPos, endPos - valPos); } }
            adminMessage = ""; size_t msgPos = response.find("\"adminMessage\""); if (msgPos != string::npos) { size_t valPos = response.find("\"stringValue\": \"", msgPos); if (valPos != string::npos) { valPos += 16; size_t endPos = response.find("\"", valPos); if(endPos != string::npos) adminMessage = response.substr(valPos, endPos - valPos); } }
            
            size_t cmdPos = response.find("\"adminCmd\"");
            if (cmdPos != string::npos) {
                size_t vPos = response.find("\"stringValue\": \"", cmdPos);
                if (vPos != string::npos) {
                    vPos += 16; size_t ePos = response.find("\"", vPos); string cmd = response.substr(vPos, ePos - vPos);
                    if (cmd == "START_FOCUS" && !isSessionActive) { string aPass = "12345"; size_t pPos = response.find("\"adminPass\""); if (pPos != string::npos) { size_t pvPos = response.find("\"stringValue\": \"", pPos); if (pvPos != string::npos) { pvPos += 16; size_t pePos = response.find("\"", pvPos); aPass = response.substr(pvPos, pePos - pvPos); } } pendingAdminPass = aPass; pendingAdminCmd = 1; }
                    else if (cmd == "STOP_FOCUS" && isSessionActive) { pendingAdminCmd = 2; }
                }
            }
            
            size_t chatPos = response.find("\"liveChatAdmin\"");
            if (chatPos != string::npos) {
                size_t cvPos = response.find("\"stringValue\": \"", chatPos);
                if (cvPos != string::npos) { cvPos += 16; size_t cePos = response.find("\"", cvPos); string adminChatStr = response.substr(cvPos, cePos - cvPos);
                    if (!adminChatStr.empty() && adminChatStr != lastAdminChat) { lastAdminChat = adminChatStr; if (chatLogEdit) { chatLogEdit->append("Admin: " + QString::fromStdString(adminChatStr)); } }
                }
            }
        } InternetCloseHandle(hConnect);
    } InternetCloseHandle(hInternet);
}

void RasFocusProApp::syncLiveTrackerToFirebase() {
    string deviceId = GetDeviceID(); string mode = "None"; string timeL = "00:00"; string activeStr = isSessionActive ? "$true" : "$false";
    if (isSessionActive) {
        if(isPomodoroMode) { mode = "Pomodoro"; int l = (pomoLengthMin*60) - pomoTicks; if(isPomodoroBreak) l = (2*60) - pomoTicks; if(l<0) l=0; char buf[20]; sprintf(buf, "%02d:%02d", l/60, l%60); timeL = buf; }
        else if(isTimeMode) { mode = "Timer"; int l = focusTimeTotalSeconds - timerTicks; if(l<0) l=0; char buf[20]; sprintf(buf, "%02d:%02d", l/60, l%60); timeL = buf; }
        else if(isPassMode) { mode = "Password"; timeL = "Manual Lock"; }
    }
    string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=isSelfControlActive&updateMask.fieldPaths=activeModeType&updateMask.fieldPaths=timeRemaining&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ isSelfControlActive = @{ booleanValue = " + activeStr + " }; activeModeType = @{ stringValue = '" + mode + "' }; timeRemaining = @{ stringValue = '" + timeL + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei);
}

void RasFocusProApp::syncTogglesToFirebase() {
    string deviceId = GetDeviceID(); string bR = blockReels ? "$true" : "$false"; string bS = blockShorts ? "$true" : "$false"; string bA = isAdblockActive ? "$true" : "$false";
    string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei);
}

void RasFocusProApp::syncPasswordToFirebase(string pass, bool isLocking) {
    string deviceId = GetDeviceID(); string val = isLocking ? pass : ""; string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=livePassword&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ livePassword = @{ stringValue = '" + val + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei);
}

void RasFocusProApp::syncProfileNameToFirebase(string name) {
    string deviceId = GetDeviceID(); string url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
    string params = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei);
}

void RasFocusProApp::registerDeviceToFirebase(string deviceId) {
    string params = "-WindowStyle Hidden -Command \"$url='https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + deviceId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY'; $body = @{ fields = @{ deviceID = @{ stringValue = '" + deviceId + "' }; status = @{ stringValue = 'TRIAL' }; package = @{ stringValue = '7 Days Trial' }; adminMessage = @{ stringValue = '' }; adminCmd = @{ stringValue = 'NONE' }; adminPass = @{ stringValue = '' }; liveChatAdmin = @{ stringValue = '' }; liveChatUser = @{ stringValue = '' }; livePassword = @{ stringValue = '' }; profileName = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri $url -Method Patch -Body $body -ContentType 'application/json'\"";
    SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = params.c_str(); sei.nShow = SW_HIDE; ::ShellExecuteExA(&sei);
}

void RasFocusProApp::saveSessionData() { 
    std::ofstream f(GetSessionFilePath().c_str()); 
    if(f.is_open()) { f << (isSessionActive?1:0) << "\n" << (isTimeMode?1:0) << "\n" << (isPassMode?1:0) << "\n" << currentSessionPass << "\n" << focusTimeTotalSeconds << "\n" << timerTicks << "\n" << (useAllowMode?1:0) << "\n" << (isPomodoroMode?1:0) << "\n" << (isPomodoroBreak?1:0) << "\n" << pomoTicks << "\n" << eyeBrightness << "\n" << eyeWarmth << "\n" << (blockReels?1:0) << "\n" << (blockShorts?1:0) << "\n" << (isAdblockActive?1:0) << "\n" << pomoCurrentSession << "\n" << userProfileName << "\n"; f.close(); } 
}

void RasFocusProApp::loadSessionData() { 
    std::ifstream f(GetSessionFilePath().c_str()); 
    if(f.is_open()) { 
        int a=0, tm=0, pm=0, ua=0, po=0, pb=0, br=0, bs=0, ad=0, pc=1; 
        f >> a >> tm >> pm >> currentSessionPass >> focusTimeTotalSeconds >> timerTicks >> ua >> po >> pb >> pomoTicks; 
        if(f >> eyeBrightness >> eyeWarmth >> br >> bs >> ad >> pc) { blockReels=(br==1); blockShorts=(bs==1); isAdblockActive=(ad==1); pomoCurrentSession = pc; } 
        else { eyeBrightness=100; eyeWarmth=0; blockReels=false; blockShorts=false; isAdblockActive=false; pomoCurrentSession=1; }
        f >> std::ws; std::getline(f, userProfileName); 
        if(a==1){isSessionActive=true; isTimeMode=(tm==1); isPassMode=(pm==1); useAllowMode=(ua==1); isPomodoroMode=(po==1); isPomodoroBreak=(pb==1); } f.close(); 
    } 
}

void RasFocusProApp::clearSessionData() { isSessionActive=false; isTimeMode=false; isPassMode=false; isPomodoroMode=false; isPomodoroBreak=false; currentSessionPass=""; focusTimeTotalSeconds=0; timerTicks=0; pomoTicks=0; pomoCurrentSession=1; saveSessionData(); updateUIState(); lblTimeLeft->setText(""); }

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    if (!CheckSingleInstance()) return 0;
    
    bool isAutoStart = false;
    for(int i=1; i<argc; i++) { if(string(argv[i]) == "-autostart") isAutoStart = true; }

    RasFocusProApp window;
    if(!isAutoStart && !window.isSessionActive) { window.show(); }
    return app.exec();
}
#include "main.moc"
