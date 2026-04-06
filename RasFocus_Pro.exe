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
#include <QCheckBox>
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
#include <QStackedWidget>
#include <QSlider>
#include <QTextEdit>
#include <QPainter>

// Windows API
#include <windows.h>
#include <tlhelp32.h>
#include <wininet.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace std;

extern void ToggleAdBlock(bool enable); // From adblocker.cpp

// ==========================================
// GLOBALS & DATA
// ==========================================
QStringList blockedApps, blockedWebs, allowedApps, allowedWebs;
QStringList systemApps = { "explorer.exe", "svchost.exe", "taskmgr.exe", "cmd.exe", "conhost.exe", "csrss.exe", "dwm.exe", "lsass.exe", "services.exe", "smss.exe", "wininit.exe", "winlogon.exe", "spoolsv.exe", "fontdrvhost.exe", "searchui.exe", "searchindexer.exe", "sihost.exe", "taskhostw.exe", "ctfmon.exe", "applicationframehost.exe", "system", "registry", "audiodg.exe", "searchapp.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "textinputhost.exe" };
QStringList commonThirdPartyApps = { "chrome.exe", "msedge.exe", "firefox.exe", "brave.exe", "opera.exe", "vivaldi.exe", "yandex.exe", "safari.exe", "code.exe", "pycharm64.exe", "python.exe", "idea64.exe", "studio64.exe", "vlc.exe", "telegram.exe", "whatsapp.exe", "discord.exe", "zoom.exe", "skype.exe", "obs64.exe", "steam.exe", "winword.exe", "excel.exe", "powerpnt.exe", "notepad.exe", "spotify.exe" };
QStringList explicitKeywords = { "porn", "xxx", "sex", "nude", "nsfw", "xvideos", "pornhub", "xnxx", "xhamster", "brazzers", "onlyfans", "playboy", "mia khalifa", "bhabi", "chudai", "bangla choti", "magi", "sexy" };
QStringList safeBrowserTitles = { "new tab", "start", "blank page", "allowed websites", "loading", "untitled", "connecting", "pomodoro break" };

QStringList islamicQuotes = { "\"মুমিনদের বলুন, তারা যেন তাদের দৃষ্টি নত রাখে এবং তাদের যৌনাঙ্গর হেফাযত করে।\" - (সূরা আন-নূর: ৩০)", "\"লজ্জাশীলতা কল্যাণ ছাড়া আর কিছুই বয়ে আনে না।\" - (সহীহ বুখারী)" };
QStringList timeQuotes = { "\"যারা সময়কে মূল্যায়ন করে না, সময়ও তাদেরকে মূল্যায়ন করে না।\" - এ.পি.জে. আবদুল কালাম" };

bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false, isOverlayVisible = false;
bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false;
bool isPomodoroMode = false, isPomodoroBreak = false, userClosedExpired = false;
bool swRunning = false;

int eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;
DWORD swStart = 0, swElapsed = 0;

QString currentSessionPass = "", userProfileName = "", safeAdminMsg = "", currentDisplayQuote = "";
QString lastAdminChat = "", pendingAdminChatStr = "", currentBroadcastMsg = "", pendingBroadcastMsg = "";
bool isLicenseValid = false, isTrialExpired = false;
int trialDaysLeft = 7, pendingAdminCmd = 0;

QWidget *dimFilterWidget = nullptr;
QWidget *warmFilterWidget = nullptr;
QWidget *overlayWidget = nullptr;
QLabel *overlayLabel = nullptr;
QTimer *overlayHideTimer = nullptr;
HHOOK hKeyboardHook;
QString globalKeyBuffer = "";

// ==========================================
// UTILITY FUNCTIONS (Win32 & Qt)
// ==========================================
QString GetDeviceID() {
    char compName[MAX_COMPUTERNAME_LENGTH + 1]; DWORD size = sizeof(compName); GetComputerNameA(compName, &size);
    DWORD volSerial = 0; GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char id[256]; sprintf(id, "%s-%X", compName, volSerial); return QString(id);
}

QString GetSecretDir() {
    QString dir = "C:/ProgramData/SysCache_Ras";
    QDir().mkpath(dir);
    SetFileAttributesA(dir.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "/";
}

void CloseActiveTabAndMinimize(HWND hBrowser) {
    if (GetForegroundWindow() == hBrowser) {
        keybd_event(VK_CONTROL, 0, 0, 0); keybd_event('W', 0, 0, 0);
        keybd_event('W', 0, KEYEVENTF_KEYUP, 0); keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(50);
    }
    ShowWindow(hBrowser, SW_MINIMIZE);
}

void SelectRandomQuote(int type) {
    if (type == 1) currentDisplayQuote = islamicQuotes[rand() % islamicQuotes.size()];
    else currentDisplayQuote = timeQuotes[rand() % timeQuotes.size()];
}

void ShowCustomOverlay(int type) {
    if(!overlayWidget) {
        overlayWidget = new QWidget();
        overlayWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        overlayWidget->setAttribute(Qt::WA_TranslucentBackground);
        overlayWidget->resize(700, 200);
        
        QVBoxLayout* l = new QVBoxLayout(overlayWidget);
        QWidget* bg = new QWidget();
        bg->setObjectName("bg");
        QVBoxLayout* bl = new QVBoxLayout(bg);
        overlayLabel = new QLabel();
        overlayLabel->setAlignment(Qt::AlignCenter);
        overlayLabel->setWordWrap(true);
        overlayLabel->setFont(QFont("Segoe UI", 18, QFont::Bold));
        overlayLabel->setStyleSheet("color: white;");
        bl->addWidget(overlayLabel);
        l->addWidget(bg);
        
        overlayHideTimer = new QTimer();
        QObject::connect(overlayHideTimer, &QTimer::timeout, [](){ overlayWidget->hide(); isOverlayVisible = false; overlayHideTimer->stop(); });
    }
    
    SelectRandomQuote(type);
    overlayLabel->setText(currentDisplayQuote);
    if(type == 1) overlayWidget->setStyleSheet("#bg { background-color: #093d1f; border: 6px solid #f1c40f; border-radius: 15px; }");
    else overlayWidget->setStyleSheet("#bg { background-color: #1a252f; border: 6px solid #3498db; border-radius: 15px; }");
    
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    overlayWidget->move((screenRect.width() - overlayWidget->width()) / 2, (screenRect.height() - overlayWidget->height()) / 2);
    
    isOverlayVisible = true;
    overlayWidget->show();
    overlayHideTimer->start(6000);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (!blockAdult && !isSessionActive) return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam); 
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam; char c = MapVirtualKey(kbdStruct->vkCode, MAPVK_VK_TO_CHAR);
        if (c >= 32 && c <= 126) {
            globalKeyBuffer += tolower(c); if (globalKeyBuffer.length() > 50) globalKeyBuffer.remove(0, 1);
            for (const QString& kw : explicitKeywords) {
                if (globalKeyBuffer.contains(kw)) {
                    globalKeyBuffer = ""; HWND hActive = GetForegroundWindow(); if(hActive) SendMessage(hActive, WM_CLOSE, 0, 0);
                    ShowCustomOverlay(1); break;
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void ApplyEyeFilters() {
    if(!dimFilterWidget) {
        dimFilterWidget = new QWidget();
        dimFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
        dimFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        dimFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    if(!warmFilterWidget) {
        warmFilterWidget = new QWidget();
        warmFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
        warmFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        warmFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }

    int dimAlpha = (100 - eyeBrightness) * 2.55; if(dimAlpha < 0) dimAlpha = 0; if(dimAlpha > 240) dimAlpha = 240;
    if(dimAlpha > 0) { dimFilterWidget->setStyleSheet(QString("background-color: rgba(0, 0, 0, %1);").arg(dimAlpha)); dimFilterWidget->show(); } 
    else { dimFilterWidget->hide(); }

    int warmAlpha = eyeWarmth * 1.5; if(warmAlpha < 0) warmAlpha = 0; if(warmAlpha > 180) warmAlpha = 180;
    if(warmAlpha > 0) { warmFilterWidget->setStyleSheet(QString("background-color: rgba(255, 130, 0, %1);").arg(warmAlpha)); warmFilterWidget->show(); } 
    else { warmFilterWidget->hide(); }
}

void SetupAutoStart() {
    char p[MAX_PATH]; GetModuleFileNameA(NULL, p, MAX_PATH);
    QString pathWithArg = "\"" + QString(p) + "\" -autostart";
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "RasFocusPro", 0, REG_SZ, (const BYTE*)pathWithArg.toStdString().c_str(), pathWithArg.length() + 1);
        RegCloseKey(hKey);
    }
}

// ==========================================
// MAIN GUI CLASS (MODERN QT STRUCTURE)
// ==========================================
class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack;
    QListWidget* sidebar;
    QTimer *fastTimer, *slowTimer, *syncTimer;
    QSystemTrayIcon* trayIcon;
    
    // UI Elements
    QLineEdit *editName, *editPass;
    QSpinBox *spinHr, *spinMin;
    QPushButton *btnStart, *btnStop;
    QLabel *lblStatus, *lblTimer, *lblLicense, *lblAdminMsg;
    
    QRadioButton *rbBlock, *rbAllow;
    QListWidget *listBlockApp, *listBlockWeb, *listAllowApp, *listAllowWeb, *listRunning;
    QLineEdit *inBlockApp, *inAllowApp;
    QComboBox *inBlockWeb, *inAllowWeb;
    
    QCheckBox *chkReels, *chkShorts, *chkAdblock;
    QSpinBox *pomoMin, *pomoSes;
    QLabel *lblPomoStatus;
    QSlider *sliderBright, *sliderWarm;
    
    // Chat & Upgrade
    QTextEdit *chatLog; QLineEdit *chatIn;
    QLineEdit *upgEmail, *upgPhone, *upgTrx; QComboBox *upgPkg;
    
    // Stopwatch
    QLabel *lblSw;

    RasFocusApp() {
        setWindowTitle("RasFocus Pro - Dashboard");
        resize(1050, 680);
        setStyleSheet("QMainWindow { background-color: #ffffff; }");
        
        QWidget* central = new QWidget();
        setCentralWidget(central);
        QHBoxLayout* mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        
        // 1. SIDEBAR (Modern Dark Theme)
        sidebar = new QListWidget();
        sidebar->setFixedWidth(240);
        sidebar->setStyleSheet(R"(
            QListWidget { background-color: #2E2E36; color: #A2A3B7; border: none; padding-top: 15px; font-size: 16px; font-weight: bold; font-family: 'Segoe UI'; }
            QListWidget::item { padding: 18px 20px; border-left: 5px solid transparent; }
            QListWidget::item:hover { background-color: #1A1A27; color: #ffffff; }
            QListWidget::item:selected { background-color: #1A1A27; color: #ffffff; border-left: 5px solid #007bff; }
        )");
        sidebar->addItem("  📊 Overview");
        sidebar->addItem("  🛡️ Blocks & Allows");
        sidebar->addItem("  ⚙️ Advanced Features");
        sidebar->addItem("  ⏱️ Tools & Pomodoro");
        sidebar->addItem("  💬 Live Chat");
        sidebar->addItem("  ⭐ Premium Upgrade");
        
        // 2. STACKED WIDGET (Content Pages)
        stack = new QStackedWidget();
        stack->setStyleSheet("QStackedWidget { background-color: #ffffff; } QLabel { color: #333; font-size: 14px; font-family: 'Segoe UI'; }");
        
        setupOverviewPage();
        setupListsPage();
        setupAdvancedPage();
        setupToolsPage();
        setupChatPage();
        setupUpgradePage();
        
        mainLayout->addWidget(sidebar);
        mainLayout->addWidget(stack);
        
        connect(sidebar, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
        sidebar->setCurrentRow(0);
        
        setupTray();
        LoadAllData();
        ApplyEyeFilters();
        
        // Timers
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }

private:
    // ==========================================
    // UI SETUP FUNCTIONS
    // ==========================================
    void setupOverviewPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(40, 40, 40, 40);
        
        // Header Section
        QHBoxLayout* h1 = new QHBoxLayout();
        h1->addWidget(new QLabel("Profile Name:")); 
        editName = new QLineEdit(); editName->setStyleSheet("padding: 6px; border: 1px solid #ccc; border-radius: 4px;"); h1->addWidget(editName);
        QPushButton* btnSave = new QPushButton("Save"); btnSave->setStyleSheet("background-color: #007bff; color: white; padding: 6px 15px; border-radius: 4px; font-weight: bold;"); h1->addWidget(btnSave);
        lblLicense = new QLabel("Checking License..."); lblLicense->setStyleSheet("font-weight: bold; font-size: 16px; color: #28a745; margin-left: 40px;"); h1->addWidget(lblLicense);
        h1->addStretch();
        l->addLayout(h1);
        connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); SaveAllData(); SyncProfileNameToFirebase(userProfileName); QMessageBox::information(this, "Success", "Profile Saved!"); });
        
        l->addSpacing(30);
        
        // Control Section
        QGridLayout* g = new QGridLayout();
        g->setSpacing(20);
        g->addWidget(new QLabel("<b>Friend Control (Password):</b>"), 0, 0); 
        editPass = new QLineEdit(); editPass->setEchoMode(QLineEdit::Password); editPass->setStyleSheet("padding: 8px; border: 1px solid #ccc; border-radius: 4px;"); g->addWidget(editPass, 1, 0);
        
        g->addWidget(new QLabel("<b>Self Control (Hr : Min):</b>"), 0, 1); 
        QHBoxLayout* th = new QHBoxLayout(); 
        spinHr = new QSpinBox(); spinHr->setStyleSheet("padding: 8px;"); 
        spinMin = new QSpinBox(); spinMin->setMaximum(59); spinMin->setStyleSheet("padding: 8px;"); 
        th->addWidget(spinHr); th->addWidget(spinMin); g->addLayout(th, 1, 1);
        l->addLayout(g);
        
        l->addSpacing(30);
        
        QHBoxLayout* h2 = new QHBoxLayout();
        btnStart = new QPushButton("▶ START FOCUS"); btnStart->setCursor(Qt::PointingHandCursor); btnStart->setStyleSheet("background-color: #10B981; color: white; padding: 15px; font-size: 16px; font-weight: bold; border-radius: 8px;");
        btnStop = new QPushButton("⏹ STOP FOCUS"); btnStop->setCursor(Qt::PointingHandCursor); btnStop->setStyleSheet("background-color: #EF4444; color: white; padding: 15px; font-size: 16px; font-weight: bold; border-radius: 8px;");
        h2->addWidget(btnStart); h2->addWidget(btnStop); h2->addStretch();
        l->addLayout(h2);
        
        lblStatus = new QLabel(""); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold; margin-top: 15px;"); l->addWidget(lblStatus);
        lblTimer = new QLabel("Status: Ready"); lblTimer->setStyleSheet("font-size: 20px; font-weight: bold; color: #3B82F6; margin-top: 20px;"); l->addWidget(lblTimer);
        
        lblAdminMsg = new QLabel(""); lblAdminMsg->setStyleSheet("color: #8B5CF6; font-weight: bold; font-size: 15px; margin-top: 20px;"); l->addWidget(lblAdminMsg);

        l->addStretch();
        stack->addWidget(page);
        
        connect(btnStart, &QPushButton::clicked, this, &RasFocusApp::onStartFocus);
        connect(btnStop, &QPushButton::clicked, this, &RasFocusApp::onStopFocus);
    }

    void setupListsPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(30, 30, 30, 30);
        
        QHBoxLayout* radL = new QHBoxLayout();
        rbBlock = new QRadioButton("Block List Mode (Default)"); rbAllow = new QRadioButton("Allow List Mode (Strict)");
        rbBlock->setChecked(true); radL->addWidget(rbBlock); radL->addWidget(rbAllow); l->addLayout(radL);
        connect(rbBlock, &QRadioButton::toggled, [=](){ useAllowMode = rbAllow->isChecked(); SaveAllData(); });
        
        l->addSpacing(15);
        QGridLayout* g = new QGridLayout();
        g->setSpacing(15);
        
        QString inputStyle = "padding: 5px; border: 1px solid #ccc; border-radius: 4px;";
        QString btnStyle = "background-color: #3B82F6; color: white; font-weight: bold; border-radius: 4px; padding: 5px 10px;";
        QString listStyle = "border: 1px solid #ccc; border-radius: 4px; background: #F8FAFC;";
        
        auto makeList = [&](QString title, QLineEdit*& inA, QComboBox*& inW, QListWidget*& lA, QListWidget*& lW, int col) {
            g->addWidget(new QLabel("<b>" + title + " Apps (.exe):</b>"), 0, col);
            QHBoxLayout* h = new QHBoxLayout(); inA = new QLineEdit(); inA->setStyleSheet(inputStyle); QPushButton* bA = new QPushButton("Add"); bA->setStyleSheet(btnStyle); h->addWidget(inA); h->addWidget(bA); g->addLayout(h, 1, col);
            lA = new QListWidget(); lA->setStyleSheet(listStyle); g->addWidget(lA, 2, col);
            QPushButton* rA = new QPushButton("Remove Selected"); rA->setStyleSheet("background-color: #64748B; color: white; border-radius: 4px; padding: 5px;"); g->addWidget(rA, 3, col);
            
            g->addWidget(new QLabel("<b>" + title + " Websites:</b>"), 4, col);
            QHBoxLayout* h2 = new QHBoxLayout(); inW = new QComboBox(); inW->setEditable(true); inW->setStyleSheet(inputStyle); QPushButton* bW = new QPushButton("Add"); bW->setStyleSheet(btnStyle); h2->addWidget(inW); h2->addWidget(bW); g->addLayout(h2, 5, col);
            lW = new QListWidget(); lW->setStyleSheet(listStyle); g->addWidget(lW, 6, col);
            QPushButton* rW = new QPushButton("Remove Selected"); rW->setStyleSheet("background-color: #64748B; color: white; border-radius: 4px; padding: 5px;"); g->addWidget(rW, 7, col);
            
            connect(bA, &QPushButton::clicked, [=](){ if(!inA->text().isEmpty()){ lA->addItem(inA->text()); inA->clear(); SyncListsFromUI(); } });
            connect(bW, &QPushButton::clicked, [=](){ if(!inW->currentText().isEmpty()){ lW->addItem(inW->currentText()); inW->setCurrentText(""); SyncListsFromUI(); } });
            connect(rA, &QPushButton::clicked, [=](){ delete lA->takeItem(lA->currentRow()); SyncListsFromUI(); });
            connect(rW, &QPushButton::clicked, [=](){ delete lW->takeItem(lW->currentRow()); SyncListsFromUI(); });
        };
        
        makeList("Block", inBlockApp, inBlockWeb, listBlockApp, listBlockWeb, 0);
        
        g->addWidget(new QLabel("<b>Running Apps (Auto-Detected):</b>"), 0, 1);
        listRunning = new QListWidget(); listRunning->setStyleSheet(listStyle); g->addWidget(listRunning, 1, 1, 6, 1);
        QPushButton* bRun = new QPushButton("Add Selected to Current List"); bRun->setStyleSheet(btnStyle); g->addWidget(bRun, 7, 1);
        connect(bRun, &QPushButton::clicked, [=](){ 
            if(!listRunning->currentItem()) return; 
            QString app = listRunning->currentItem()->text();
            if(useAllowMode) { listAllowApp->addItem(app); } else { listBlockApp->addItem(app); }
            SyncListsFromUI();
        });
        
        makeList("Allow", inAllowApp, inAllowWeb, listAllowApp, listAllowWeb, 2);
        
        l->addLayout(g);
        
        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com", "reddit.com", "netflix.com"};
        inBlockWeb->addItems(popSites); inAllowWeb->addItems(popSites);
        inBlockWeb->setCurrentText(""); inAllowWeb->setCurrentText("");
        
        stack->addWidget(page);
    }

    void setupAdvancedPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(50, 50, 50, 50);
        
        QLabel* title = new QLabel("Advanced Blocking Features");
        title->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 10px; color: #1e1e2d;");
        l->addWidget(title);
        
        QLabel* sub = new QLabel("Enable these strict settings to prevent distractions completely.");
        sub->setStyleSheet("color: #64748B; margin-bottom: 30px;");
        l->addWidget(sub);
        
        // CSS to make checkboxes look like professional toggle switches
        QString toggleStyle = R"(
            QCheckBox { font-size: 16px; padding: 15px; background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px; margin-bottom: 10px; font-weight: bold; color: #334155; }
            QCheckBox::indicator { width: 45px; height: 24px; border-radius: 12px; }
            QCheckBox::indicator:unchecked { image: none; background-color: #CBD5E1; }
            QCheckBox::indicator:checked { image: none; background-color: #10B981; }
            QCheckBox:hover { background: #F1F5F9; }
        )";
        
        chkReels = new QCheckBox("   Block Facebook Reels, Watch & Videos (Strict Mode)"); chkReels->setStyleSheet(toggleStyle); chkReels->setCursor(Qt::PointingHandCursor);
        chkShorts = new QCheckBox("   Block YouTube Shorts"); chkShorts->setStyleSheet(toggleStyle); chkShorts->setCursor(Qt::PointingHandCursor);
        chkAdblock = new QCheckBox("   System-wide AD BLOCKER (Silent Install)"); chkAdblock->setStyleSheet(toggleStyle); chkAdblock->setCursor(Qt::PointingHandCursor);
        
        l->addWidget(chkReels); l->addWidget(chkShorts); l->addWidget(chkAdblock);
        
        connect(chkReels, &QCheckBox::clicked, [=](bool c){ blockReels = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkShorts, &QCheckBox::clicked, [=](bool c){ blockShorts = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkAdblock, &QCheckBox::clicked, [=](bool c){ isAdblockActive = c; ToggleAdBlock(c); SaveAllData(); SyncTogglesToFirebase(); });
        
        l->addStretch();
        stack->addWidget(page);
    }

    void setupToolsPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(40, 40, 40, 40);
        
        // POMODORO
        QLabel* pt = new QLabel("🍅 Pomodoro Timer"); pt->setStyleSheet("font-size: 20px; font-weight: bold; color: #EF4444; margin-bottom: 10px;"); l->addWidget(pt);
        QHBoxLayout* ph = new QHBoxLayout();
        ph->addWidget(new QLabel("Focus Length (Min):")); pomoMin = new QSpinBox(); pomoMin->setValue(25); pomoMin->setStyleSheet("padding: 5px;"); ph->addWidget(pomoMin);
        ph->addSpacing(20);
        ph->addWidget(new QLabel("Total Sessions:")); pomoSes = new QSpinBox(); pomoSes->setValue(4); pomoSes->setStyleSheet("padding: 5px;"); ph->addWidget(pomoSes);
        ph->addStretch();
        l->addLayout(ph);
        
        QHBoxLayout* pBtnH = new QHBoxLayout();
        QPushButton* bPStart = new QPushButton("Start Pomodoro"); bPStart->setStyleSheet("background: #10B981; color: white; padding: 10px; font-weight: bold; border-radius: 5px;");
        QPushButton* bPStop = new QPushButton("Stop"); bPStop->setStyleSheet("background: #EF4444; color: white; padding: 10px; font-weight: bold; border-radius: 5px;");
        pBtnH->addWidget(bPStart); pBtnH->addWidget(bPStop); pBtnH->addStretch();
        l->addLayout(pBtnH);
        
        lblPomoStatus = new QLabel("Status: Ready"); lblPomoStatus->setStyleSheet("color: #64748B; font-weight: bold; margin-bottom: 30px;"); l->addWidget(lblPomoStatus);
        
        connect(bPStart, &QPushButton::clicked, [=](){
            if(!isSessionActive && !isTrialExpired) {
                pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value();
                isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1;
                SaveAllData(); QMessageBox::information(this, "Started", "Pomodoro Started!"); updateUIStates();
            }
        });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { ClearSessionData(); updateUIStates(); QMessageBox::information(this, "Stopped", "Pomodoro Stopped!"); } });
        
        // STOPWATCH
        QFrame* line1 = new QFrame(); line1->setFrameShape(QFrame::HLine); line1->setStyleSheet("color: #ccc; margin: 20px 0;"); l->addWidget(line1);
        QLabel* st = new QLabel("⏱️ Pro Stopwatch"); st->setStyleSheet("font-size: 20px; font-weight: bold; color: #334155; margin-bottom: 10px;"); l->addWidget(st);
        
        lblSw = new QLabel("00:00:00.00"); lblSw->setStyleSheet("font-size: 40px; font-family: 'Consolas'; font-weight: bold; color: #0F172A;"); l->addWidget(lblSw);
        QHBoxLayout* sh = new QHBoxLayout();
        QPushButton* bSwStart = new QPushButton("Start / Pause"); bSwStart->setStyleSheet("background: #3B82F6; color: white; padding: 10px; font-weight: bold; border-radius: 5px;");
        QPushButton* bSwReset = new QPushButton("Reset"); bSwReset->setStyleSheet("background: #64748B; color: white; padding: 10px; font-weight: bold; border-radius: 5px;");
        sh->addWidget(bSwStart); sh->addWidget(bSwReset); sh->addStretch();
        l->addLayout(sh);
        
        connect(bSwStart, &QPushButton::clicked, [=](){ swRunning = !swRunning; if(swRunning) swStart = GetTickCount() - swElapsed; });
        connect(bSwReset, &QPushButton::clicked, [=](){ swRunning = false; swElapsed = 0; lblSw->setText("00:00:00.00"); });
        
        // EYE CURE
        QFrame* line2 = new QFrame(); line2->setFrameShape(QFrame::HLine); line2->setStyleSheet("color: #ccc; margin: 20px 0;"); l->addWidget(line2);
        QLabel* et = new QLabel("👁️ Eye Cure Filters"); et->setStyleSheet("font-size: 20px; font-weight: bold; color: #8B5CF6; margin-bottom: 10px;"); l->addWidget(et);
        
        sliderBright = new QSlider(Qt::Horizontal); sliderBright->setRange(10, 100); sliderBright->setValue(100);
        sliderWarm = new QSlider(Qt::Horizontal); sliderWarm->setRange(0, 100); sliderWarm->setValue(0);
        l->addWidget(new QLabel("Brightness:")); l->addWidget(sliderBright);
        l->addWidget(new QLabel("Warmth (Night Light):")); l->addWidget(sliderWarm);
        
        QHBoxLayout* eh = new QHBoxLayout();
        QPushButton* bD = new QPushButton("Day Mode"); bD->setStyleSheet("background: #F59E0B; color: white; padding: 8px; border-radius: 4px;");
        QPushButton* bR = new QPushButton("Reading"); bR->setStyleSheet("background: #8B5CF6; color: white; padding: 8px; border-radius: 4px;");
        QPushButton* bN = new QPushButton("Night Time"); bN->setStyleSheet("background: #1E293B; color: white; padding: 8px; border-radius: 4px;");
        eh->addWidget(bD); eh->addWidget(bR); eh->addWidget(bN); eh->addStretch();
        l->addLayout(eh);
        
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; ApplyEyeFilters(); SaveAllData(); });
        connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; ApplyEyeFilters(); SaveAllData(); });
        connect(bD, &QPushButton::clicked, [=](){ sliderBright->setValue(100); sliderWarm->setValue(0); });
        connect(bR, &QPushButton::clicked, [=](){ sliderBright->setValue(85); sliderWarm->setValue(30); });
        connect(bN, &QPushButton::clicked, [=](){ sliderBright->setValue(60); sliderWarm->setValue(75); });

        l->addStretch();
        stack->addWidget(page);
    }

    void setupChatPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(40, 40, 40, 40);
        
        QLabel* title = new QLabel("💬 Live Chat with Admin");
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #EC4899; margin-bottom: 20px;");
        l->addWidget(title);
        
        chatLog = new QTextEdit(); chatLog->setReadOnly(true); 
        chatLog->setStyleSheet("background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; padding: 10px; font-size: 15px;");
        l->addWidget(chatLog);
        
        QHBoxLayout* ch = new QHBoxLayout();
        chatIn = new QLineEdit(); chatIn->setStyleSheet("padding: 12px; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 15px;"); ch->addWidget(chatIn);
        QPushButton* bSend = new QPushButton("Send"); bSend->setStyleSheet("background: #EC4899; color: white; font-weight: bold; padding: 12px 20px; border-radius: 8px;"); ch->addWidget(bSend);
        l->addLayout(ch);
        
        connect(bSend, &QPushButton::clicked, [=](){
            QString msg = chatIn->text().trimmed();
            if(!msg.isEmpty()) {
                chatLog->append("<b>You:</b> " + msg); chatIn->clear();
                QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
                QString cmd = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + msg + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
                SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = cmd.toStdString().c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
            }
        });
        
        stack->addWidget(page);
    }
    
    void setupUpgradePage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(60, 40, 60, 40);
        
        QLabel* title = new QLabel("⭐ Premium Upgrade");
        title->setStyleSheet("font-size: 26px; font-weight: bold; color: #F59E0B; margin-bottom: 10px;");
        l->addWidget(title);
        l->addWidget(new QLabel("Send payment via Nagad/bKash to <b>01566054963</b> and submit the form below."));
        l->addSpacing(20);
        
        QString inS = "padding: 10px; border: 1px solid #ccc; border-radius: 5px; font-size: 15px; margin-bottom: 15px;";
        
        l->addWidget(new QLabel("Email / Name:")); upgEmail = new QLineEdit(); upgEmail->setStyleSheet(inS); l->addWidget(upgEmail);
        l->addWidget(new QLabel("Sender Number:")); upgPhone = new QLineEdit(); upgPhone->setStyleSheet(inS); l->addWidget(upgPhone);
        l->addWidget(new QLabel("Transaction ID (TrxID):")); upgTrx = new QLineEdit(); upgTrx->setStyleSheet(inS); l->addWidget(upgTrx);
        
        l->addWidget(new QLabel("Select Package:"));
        upgPkg = new QComboBox(); upgPkg->addItems({"7 Days Trial", "6 Months (50 BDT)", "1 Year (100 BDT)"}); upgPkg->setStyleSheet(inS); l->addWidget(upgPkg);
        
        QPushButton* bSub = new QPushButton("SUBMIT REQUEST"); 
        bSub->setStyleSheet("background: #10B981; color: white; padding: 15px; font-weight: bold; font-size: 16px; border-radius: 8px; margin-top: 10px;");
        l->addWidget(bSub);
        
        connect(bSub, &QPushButton::clicked, [=](){
            if(upgEmail->text().isEmpty() || upgPhone->text().isEmpty() || upgTrx->text().isEmpty()) { QMessageBox::warning(this, "Error", "Fill all fields!"); return; }
            QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            QString cmd = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ deviceID = @{ stringValue = '" + dId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + upgPkg->currentText() + "' }; userEmail = @{ stringValue = '" + upgEmail->text() + "' }; senderPhone = @{ stringValue = '" + upgPhone->text() + "' }; trxId = @{ stringValue = '" + upgTrx->text() + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
            SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = cmd.toStdString().c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
            QMessageBox::information(this, "Success", "Request Sent!");
        });
        
        l->addStretch();
        stack->addWidget(page);
    }

    void setupTray() {
        trayIcon = new QSystemTrayIcon(QIcon("icon.ico"), this);
        QMenu* menu = new QMenu(this);
        QAction* res = new QAction("Restore", this); connect(res, &QAction::triggered, this, &QWidget::showNormal); menu->addAction(res);
        QAction* ex = new QAction("Exit", this); connect(ex, &QAction::triggered, qApp, &QCoreApplication::quit); menu->addAction(ex);
        trayIcon->setContextMenu(menu);
        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason r){ if(r == QSystemTrayIcon::DoubleClick) showNormal(); });
        trayIcon->show();
    }

    // ==========================================
    // LOGIC & SYNC FUNCTIONS
    // ==========================================
    QStringList GetRunningAppsUI() {
        QStringList p; HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)};
        if(Process32FirstW(h, &pe)) { do { QString n = QString::fromWCharArray(pe.szExeFile).toLower(); if(!systemApps.contains(n)) p << n; } while(Process32NextW(h, &pe)); }
        CloseHandle(h); p.removeDuplicates(); p.sort(Qt::CaseInsensitive); return p;
    }

    void SyncListsFromUI() {
        auto getList = [](QListWidget* lw) { QStringList l; for(int i=0; i<lw->count(); ++i) l << lw->item(i)->text(); return l; };
        blockedApps = getList(listBlockApp); blockedWebs = getList(listBlockWeb);
        allowedApps = getList(listAllowApp); allowedWebs = getList(listAllowWeb);
        SaveAllData();
    }

    void LoadAllData() {
        auto lF = [](QString fn, QStringList& l, QListWidget* lw) { 
            QFile f(GetSecretDir() + fn); if(f.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream in(&f); while(!in.atEnd()) { QString v=in.readLine().trimmed(); if(!v.isEmpty()){ l<<v; lw->addItem(v); } } f.close(); } 
        };
        lF("bl_app.dat", blockedApps, listBlockApp); lF("bl_web.dat", blockedWebs, listBlockWeb);
        lF("al_app.dat", allowedApps, listAllowApp); lF("al_web.dat", allowedWebs, listAllowWeb);
        listRunning->addItems(GetRunningAppsUI());
        
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QTextStream in(&f);
            int a=0, tm=0, pm=0, ua=0, po=0, pb=0, br=0, bs=0, ad=0, pc=1;
            in >> a >> tm >> pm >> currentSessionPass >> focusTimeTotalSeconds >> timerTicks >> ua >> po >> pb >> pomoTicks >> eyeBrightness >> eyeWarmth >> br >> bs >> ad >> pc;
            userProfileName = in.readLine().trimmed(); if(userProfileName.isEmpty()) userProfileName = in.readLine().trimmed(); // Next line
            
            isSessionActive=(a==1); isTimeMode=(tm==1); isPassMode=(pm==1); useAllowMode=(ua==1); isPomodoroMode=(po==1); isPomodoroBreak=(pb==1); pomoCurrentSession=pc;
            blockReels=(br==1); blockShorts=(bs==1); isAdblockActive=(ad==1);
            
            rbAllow->setChecked(useAllowMode); chkReels->setChecked(blockReels); chkShorts->setChecked(blockShorts); chkAdblock->setChecked(isAdblockActive);
            sliderBright->setValue(eyeBrightness); sliderWarm->setValue(eyeWarmth); editName->setText(userProfileName);
            f.close();
        }
        updateUIStates();
    }

    void SaveAllData() {
        auto sF = [](QString fn, const QStringList& l) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::WriteOnly|QIODevice::Text)) { QTextStream out(&f); for(auto i:l) out<<i<<"\n"; f.close(); } };
        sF("bl_app.dat", blockedApps); sF("bl_web.dat", blockedWebs); sF("al_app.dat", allowedApps); sF("al_web.dat", allowedWebs);
        
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::WriteOnly|QIODevice::Text)) {
            QTextStream out(&f);
            out << (isSessionActive?1:0) << " " << (isTimeMode?1:0) << " " << (isPassMode?1:0) << " " << currentSessionPass << " " << focusTimeTotalSeconds << " " << timerTicks << " " << (useAllowMode?1:0) << " " << (isPomodoroMode?1:0) << " " << (isPomodoroBreak?1:0) << " " << pomoTicks << " " << eyeBrightness << " " << eyeWarmth << " " << (blockReels?1:0) << " " << (blockShorts?1:0) << " " << (isAdblockActive?1:0) << " " << pomoCurrentSession << "\n" << userProfileName << "\n";
            f.close();
        }
    }

    void ClearSessionData() {
        isSessionActive = isTimeMode = isPassMode = isPomodoroMode = isPomodoroBreak = false;
        currentSessionPass = ""; focusTimeTotalSeconds = timerTicks = pomoTicks = 0; pomoCurrentSession = 1;
        lblTimer->setText("Status: Ready"); lblStatus->setText(""); lblPomoStatus->setText("Status: Ready");
        SaveAllData(); updateUIStates();
    }

    void updateUIStates() {
        btnStart->setEnabled(!isSessionActive); btnStop->setEnabled(isSessionActive);
        if(isSessionActive) lblStatus->setText("Focus is Active. To deactivate, write password and click STOP or wait for timer.");
        else lblStatus->setText("");
    }

    void SyncProfileNameToFirebase(QString name) {
        QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        QString cmd = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
        SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = cmd.toStdString().c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
    }
    
    void SyncTogglesToFirebase() {
        QString dId = GetDeviceID(); QString bR = blockReels ? "$true" : "$false", bS = blockShorts ? "$true" : "$false", bA = isAdblockActive ? "$true" : "$false";
        QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        QString cmd = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
        SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = cmd.toStdString().c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
    }

    // --- TIMERS LOGIC ---
    void fastLoop() { // 200ms
        if(swRunning) {
            swElapsed = GetTickCount() - swStart;
            DWORD ts = swElapsed / 1000; DWORD ms = (swElapsed % 1000) / 10;
            lblSw->setText(QString("%1:%2:%3.%4").arg(ts/3600, 2, 10, QChar('0')).arg((ts%3600)/60, 2, 10, QChar('0')).arg(ts%60, 2, 10, QChar('0')).arg(ms, 2, 10, QChar('0')));
        }

        if(!blockAdult && !blockReels && !blockShorts && !isSessionActive) return;
        if(isOverlayVisible) return;

        HWND hActive = GetForegroundWindow();
        if(hActive && (!overlayWidget || hActive != (HWND)overlayWidget->winId())) {
            WCHAR title[512];
            if(GetWindowTextW(hActive, title, 512) > 0) {
                QString sTitle = QString::fromWCharArray(title).toLower();
                
                // Adult Check
                if(blockAdult) { for(const QString& kw : explicitKeywords) { if(sTitle.contains(kw)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(1); return; } } }
                
                // Fast Reels & Shorts Check
                if(blockReels && sTitle.contains("facebook") && (sTitle.contains("reels") || sTitle.contains("video") || sTitle.contains("watch"))) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                if(blockShorts && sTitle.contains("youtube") && sTitle.contains("shorts")) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                
                // Active Session Check (Browser Tabs)
                if(isSessionActive) {
                    bool isBrowser = sTitle.contains("chrome") || sTitle.contains("edge") || sTitle.contains("firefox") || sTitle.contains("brave") || sTitle.contains("opera");
                    if(isBrowser) {
                        if(useAllowMode) {
                            bool ok = false; for(const QString& w : allowedWebs) { if(sTitle.contains(w.toLower())) { ok=true; break; } }
                            if(!ok && !sTitle.contains("allowed websites")) { 
                                CloseActiveTabAndMinimize(hActive); 
                                
                                // Generate & Open Allowed Websites HTML
                                QString p = GetSecretDir() + "allowed_sites.html"; 
                                QFile f(p); 
                                if(f.open(QIODevice::WriteOnly)){ 
                                    QTextStream out(&f); 
                                    out<<"<html><body style='text-align:center; font-family:sans-serif; margin-top:50px;'><h2>Focus Mode is Active!</h2><p>Allowed Sites:</p>"; 
                                    for(auto x:allowedWebs) out<<"<a href='https://"<<x<<"' style='display:inline-block; margin:10px; padding:10px; background:#007bff; color:white; text-decoration:none; border-radius:5px;'>"<<x<<"</a><br>"; 
                                    out<<"</body></html>"; 
                                    f.close(); 
                                } 
                                QDesktopServices::openUrl(QUrl::fromLocalFile(p)); 
                            }
                        } else {
                            for(const QString& w : blockedWebs) { if(sTitle.contains(w.toLower())) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; } }
                        }
                    }
                }
            }
        }
    }

    void slowLoop() { // 1000ms
        if(isSessionActive) {
            if(isTrialExpired) { ClearSessionData(); QMessageBox::critical(this, "Expired", "License Expired!"); return; }
            
            if(isPomodoroMode) {
                pomoTicks++; if(pomoTicks%5==0) SaveAllData();
                if(!isPomodoroBreak && pomoTicks >= pomoLengthMin*60) { 
                    isPomodoroBreak=true; pomoTicks=0; 
                    
                    // Generate & Open Pomodoro Break HTML
                    QString p = GetSecretDir() + "pomodoro_break.html"; 
                    QFile f(p); 
                    if(f.open(QIODevice::WriteOnly)){ 
                        QTextStream out(&f); 
                        out<<"<html><body style='background:#1e3c72; color:white; text-align:center; padding-top:100px; font-family:sans-serif;'><h1>Time to Relax & Drink Water!</h1><p>Break Started.</p></body></html>"; 
                        f.close(); 
                    } 
                    QDesktopServices::openUrl(QUrl::fromLocalFile(p));
                }
                else if(isPomodoroBreak && pomoTicks >= 2*60) { isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); QMessageBox::information(this, "Done", "Pomodoro Complete!"); } }
                
                int l = isPomodoroBreak ? (2*60)-pomoTicks : (pomoLengthMin*60)-pomoTicks; if(l<0) l=0;
                QString st = isPomodoroBreak ? "Break" : QString("Focus %1/%2").arg(pomoCurrentSession).arg(pomoTotalSessions);
                QString tt = QString("%1: %2:%3").arg(st).arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0'));
                lblTimer->setText(tt); lblPomoStatus->setText(tt); trayIcon->setToolTip(tt);
            }
            else if(isTimeMode) {
                timerTicks++; int left = focusTimeTotalSeconds - timerTicks;
                if(left <= 0) { ClearSessionData(); QMessageBox::information(this, "Done", "Focus Time Over!"); return; }
                QString tt = QString("Time Left: %1:%2:%3").arg(left/3600, 2, 10, QChar('0')).arg((left%3600)/60, 2, 10, QChar('0')).arg(left%60, 2, 10, QChar('0'));
                lblTimer->setText(tt); trayIcon->setToolTip(tt);
            } else { trayIcon->setToolTip("Focus Active (Password)"); lblTimer->setText("Focus Active (Password)"); }
            
            // Kill Processes
            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)}; DWORD myPid = GetCurrentProcessId();
            if(Process32FirstW(h, &pe)) {
                do {
                    if(pe.th32ProcessID == myPid) continue;
                    QString n = QString::fromWCharArray(pe.szExeFile).toLower();
                    if(n == "taskmgr.exe") { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } continue; }
                    
                    if(useAllowMode) {
                        if(!systemApps.contains(n) && !allowedApps.contains(n, Qt::CaseInsensitive) && !commonThirdPartyApps.contains(n)) {
                            HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); }
                        }
                    } else {
                        if(blockedApps.contains(n, Qt::CaseInsensitive)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } }
                    }
                } while(Process32NextW(h, &pe));
            } CloseHandle(h);
        }
    }

    void syncLoop() { // 4000ms
        ValidateLicenseAndTrial(); 
        SyncLiveTrackerToFirebase(); // <--- Added Live Tracker Missing Fix
        
        if (isTrialExpired) { lblLicense->setText("LICENSE EXPIRED"); lblLicense->setStyleSheet("color: red; font-weight: bold; margin-left: 30px;"); }
        else if (isLicenseValid) { 
            lblLicense->setText(QString("PREMIUM: %1 DAYS LEFT").arg(trialDaysLeft)); 
            lblLicense->setStyleSheet("color: green; font-weight: bold; margin-left: 30px;"); 
            if(sidebar->count() > 5) sidebar->item(5)->setHidden(true); // Hide Upgrade if Premium
        }
        else { lblLicense->setText(QString("TRIAL: %1 DAYS LEFT").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: orange; font-weight: bold; margin-left: 30px;"); }
        
        if(!safeAdminMsg.isEmpty()) lblAdminMsg->setText("Admin Notice: " + safeAdminMsg); else lblAdminMsg->setText("");
        
        if(!pendingAdminChatStr.isEmpty()) { chatLog->append("<b>Admin:</b> " + pendingAdminChatStr); pendingAdminChatStr = ""; }
        
        // Broadcast Popup Fix
        if(!pendingBroadcastMsg.isEmpty() && pendingBroadcastMsg != "ACK") {
            currentBroadcastMsg = pendingBroadcastMsg;
            pendingBroadcastMsg = "";
            QMessageBox::information(this, "Admin Broadcast", currentBroadcastMsg);
            
            // Send ACK
            QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=broadcastMsg&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            QString cmd = "-WindowStyle Hidden -Command \"$body = @{ fields = @{ broadcastMsg = @{ stringValue = 'ACK' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'\"";
            SHELLEXECUTEINFOA sei = { sizeof(sei) }; sei.lpVerb = "open"; sei.lpFile = "powershell.exe"; sei.lpParameters = cmd.toStdString().c_str(); sei.nShow = SW_HIDE; ShellExecuteExA(&sei);
        }
        
        if(pendingAdminCmd == 1 && !isSessionActive) { pendingAdminCmd = 0; currentSessionPass = "12345"; isPassMode = true; isTimeMode = false; isPomodoroMode = false; isSessionActive = true; SaveAllData(); updateUIStates(); hide(); }
        else if(pendingAdminCmd == 2 && isSessionActive) { pendingAdminCmd = 0; ClearSessionData(); updateUIStates(); }
    }

    void closeEvent(QCloseEvent *event) override {
        if(isSessionActive) { event->ignore(); hide(); } else { event->accept(); }
    }
};

int main(int argc, char *argv[]) {
    // Single instance check
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "RasFocusPro_Mutex_QT_V1");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
    
    SetupAutoStart(); 
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false); // Keeps running in tray
    app.setFont(QFont("Segoe UI", 10));
    
    RasFocusApp window;
    
    QString cmdStr = ""; for(int i=1; i<argc; i++) cmdStr += argv[i];
    if(cmdStr.contains("-autostart") || isSessionActive) window.hide(); else window.show();
    
    int ret = app.exec();
    UnhookWindowsHookEx(hKeyboardHook);
    return ret;
}
