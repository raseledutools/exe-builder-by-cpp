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
#include <QStyle>
#include <QMap>
#include <QElapsedTimer>

// Windows API
#include <windows.h>
#include <tlhelp32.h>
#include <wininet.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "dwmapi.lib")

using namespace std;

extern void ToggleAdBlock(bool enable); 

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

bool isSessionActive = false, isTimeMode = false, useAllowMode = false, isOverlayVisible = false;
bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false;
bool isPomodoroMode = false, isPomodoroBreak = false, userClosedExpired = false;

int eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;

QString userProfileName = "", safeAdminMsg = "", currentDisplayQuote = "";
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

// Usage Tracker
QMap<QString, int> usageStats;
DWORD lastUsageUpdate = 0;

// ==========================================
// UTILITY FUNCTIONS
// ==========================================
void runPowerShell(QString cmdBody) {
    QProcess::startDetached("powershell.exe", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << cmdBody);
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
        overlayWidget->resize(850, 350); 
        
        QVBoxLayout* l = new QVBoxLayout(overlayWidget);
        QWidget* bg = new QWidget();
        bg->setObjectName("bg");
        QVBoxLayout* bl = new QVBoxLayout(bg);
        bl->setContentsMargins(40, 40, 40, 40); 
        
        overlayLabel = new QLabel();
        overlayLabel->setAlignment(Qt::AlignCenter);
        overlayLabel->setWordWrap(true);
        overlayLabel->setFont(QFont("Segoe UI", 24, QFont::Bold)); 
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
                    QMetaObject::invokeMethod(qApp, [=](){ ShowCustomOverlay(1); }, Qt::QueuedConnection); 
                    break;
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void ApplyEyeFilters() {
    if(!dimFilterWidget) {
        dimFilterWidget = new QWidget();
        dimFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
        dimFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        dimFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    if(!warmFilterWidget) {
        warmFilterWidget = new QWidget();
        warmFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
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

// ==========================================
// STOPWATCH WINDOW CLASS (Floating)
// ==========================================
class StopwatchWindow : public QWidget {
public:
    QLabel *lblSw;
    QElapsedTimer timer;
    QTimer *updateTimer;
    bool isRunning = false;
    qint64 pausedTime = 0;

    StopwatchWindow() {
        setWindowTitle("Pro Stopwatch");
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        resize(400, 150);
        setStyleSheet("background-color: #ffffff; border-radius: 10px;");

        QVBoxLayout* l = new QVBoxLayout(this);
        lblSw = new QLabel("00:00:00.00");
        lblSw->setAlignment(Qt::AlignCenter);
        lblSw->setStyleSheet("font-size: 45px; font-family: 'Consolas'; font-weight: bold; color: #0F172A;");
        l->addWidget(lblSw);

        QHBoxLayout* h = new QHBoxLayout();
        QPushButton* btnStart = new QPushButton("Start / Pause");
        QPushButton* btnReset = new QPushButton("Reset");
        btnStart->setStyleSheet("background: #3B82F6; color: white; padding: 10px; font-weight: bold; border-radius: 5px; font-size: 14px;");
        btnReset->setStyleSheet("background: #64748B; color: white; padding: 10px; font-weight: bold; border-radius: 5px; font-size: 14px;");
        h->addWidget(btnStart); h->addWidget(btnReset);
        l->addLayout(h);

        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, [=](){
            if(isRunning) {
                qint64 el = timer.elapsed() + pausedTime;
                int ms = (el % 1000) / 10;
                int s = (el / 1000) % 60;
                int m = (el / 60000) % 60;
                int hr = (el / 3600000);
                lblSw->setText(QString("%1:%2:%3.%4").arg(hr, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ms, 2, 10, QChar('0')));
            }
        });

        connect(btnStart, &QPushButton::clicked, [=](){
            if(isRunning) { isRunning = false; pausedTime += timer.elapsed(); updateTimer->stop(); }
            else { isRunning = true; timer.start(); updateTimer->start(30); }
        });
        connect(btnReset, &QPushButton::clicked, [=](){
            isRunning = false; pausedTime = 0; updateTimer->stop(); lblSw->setText("00:00:00.00");
        });
    }
};

StopwatchWindow* swWindow = nullptr;

// ==========================================
// MAIN GUI CLASS
// ==========================================
class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack;
    QListWidget* sidebar;
    QTimer *fastTimer, *slowTimer, *syncTimer;
    QSystemTrayIcon* trayIcon;
    
    // UI Elements
    QLineEdit *editName;
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
    
    QTextEdit *chatLog; QLineEdit *chatIn;
    QLineEdit *upgEmail, *upgPhone, *upgTrx; QComboBox *upgPkg;

    RasFocusApp() {
        setWindowTitle("RasFocus Pro - Dashboard");
        resize(1050, 680);
        setStyleSheet("QMainWindow { background-color: #F4F7F6; }");
        
        QWidget* central = new QWidget();
        setCentralWidget(central);
        QHBoxLayout* mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        
        // 1. SIDEBAR
        sidebar = new QListWidget();
        sidebar->setFixedWidth(260);
        sidebar->setStyleSheet(R"(
            QListWidget { background-color: #ffffff; color: #475569; border-right: 1px solid #E2E8F0; padding-top: 15px; font-size: 16px; font-weight: bold; font-family: 'Segoe UI'; }
            QListWidget::item { padding: 18px 20px; border-left: 5px solid transparent; }
            QListWidget::item:hover { background-color: #F1F5F9; color: #0F172A; }
            QListWidget::item:selected { background-color: #EFF6FF; color: #2563EB; border-left: 5px solid #3B82F6; }
        )");
        sidebar->addItem("  📊 Overview");
        sidebar->addItem("  🛡️ Blocks & Allows");
        sidebar->addItem("  ⚙️ Advanced Features");
        sidebar->addItem("  ⏱️ Tools & Pomodoro");
        sidebar->addItem("  💬 Live Chat");
        sidebar->addItem("  ⭐ Premium Upgrade");
        
        // 2. STACKED WIDGET
        stack = new QStackedWidget();
        stack->setStyleSheet("QStackedWidget { background-color: #F4F7F6; } QLabel { color: #333; font-size: 15px; font-family: 'Segoe UI'; }");
        
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
        ApplyEyeFilters();
        
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (isSessionActive) {
            QMessageBox::warning(this, "Focus Active", "You cannot close the app while a focus session is running. It will be hidden to the tray.");
            hide();
            event->ignore();
        } else {
            hide(); // Minimize to tray instead of full exit
            event->ignore();
        }
    }

private:
    void setupTray() {
        trayIcon = new QSystemTrayIcon(this);
        // Ensure you have an icon set here for it to appear properly in the tray
        // trayIcon->setIcon(QIcon(":/icons/main.png")); 
        
        QMenu *trayMenu = new QMenu(this);
        QAction *showAction = new QAction("Open Dashboard", this);
        QAction *quitAction = new QAction("Quit App", this);

        connect(showAction, &QAction::triggered, this, &RasFocusApp::showNormal);
        connect(quitAction, &QAction::triggered, [=](){
            if(isSessionActive) {
                QMessageBox::warning(nullptr, "Access Denied", "Cannot quit while focus mode is active.");
            } else {
                QCoreApplication::quit();
            }
        });

        trayMenu->addAction(showAction);
        trayMenu->addAction(quitAction);
        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();

        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason){
            if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
                this->showNormal();
                this->activateWindow();
            }
        });
    }

    void setupOverviewPage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(40, 40, 40, 40);
        
        QHBoxLayout* h1 = new QHBoxLayout();
        h1->addWidget(new QLabel("Profile Name:")); 
        editName = new QLineEdit(); editName->setStyleSheet("padding: 10px; border: 1px solid #ccc; border-radius: 6px; background: #fff;"); h1->addWidget(editName);
        QPushButton* btnSave = new QPushButton("Save Profile"); btnSave->setStyleSheet("background-color: #2563EB; color: white; padding: 10px 20px; border-radius: 6px; font-weight: bold;"); h1->addWidget(btnSave);
        lblLicense = new QLabel("Checking License..."); lblLicense->setStyleSheet("font-weight: bold; font-size: 16px; color: #10B981; margin-left: 40px;"); h1->addWidget(lblLicense);
        h1->addStretch();
        l->addLayout(h1);
        connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); QMessageBox::information(this, "Success", "Profile Saved!"); });
        
        l->addSpacing(30);
        
        QGridLayout* g = new QGridLayout();
        g->setSpacing(20);
        
        g->addWidget(new QLabel("<b>Self Control Timer (Hr : Min):</b>"), 0, 0); 
        QHBoxLayout* th = new QHBoxLayout(); 
        spinHr = new QSpinBox(); spinHr->setStyleSheet("padding: 10px; font-size: 16px;"); 
        spinMin = new QSpinBox(); spinMin->setMaximum(59); spinMin->setStyleSheet("padding: 10px; font-size: 16px;"); 
        th->addWidget(spinHr); th->addWidget(spinMin); g->addLayout(th, 1, 0);
        l->addLayout(g);
        
        l->addSpacing(30);
        
        QHBoxLayout* h2 = new QHBoxLayout();
        btnStart = new QPushButton("▶ START FOCUS"); btnStart->setCursor(Qt::PointingHandCursor); 
        btnStart->setStyleSheet("background-color: #10B981; color: white; padding: 15px 30px; font-size: 16px; font-weight: bold; border-radius: 8px; border: none;");
        
        btnStop = new QPushButton("⏹ STOP FOCUS"); btnStop->setCursor(Qt::PointingHandCursor); 
        btnStop->setStyleSheet("background-color: #EF4444; color: white; padding: 15px 30px; font-size: 16px; font-weight: bold; border-radius: 8px; border: none;");
        
        h2->addWidget(btnStart); h2->addWidget(btnStop); h2->addStretch();
        l->addLayout(h2);
        
        lblStatus = new QLabel(""); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold; margin-top: 15px;"); l->addWidget(lblStatus);
        lblTimer = new QLabel("Status: Ready to Focus"); lblTimer->setStyleSheet("font-size: 20px; font-weight: bold; color: #3B82F6; margin-top: 20px;"); l->addWidget(lblTimer);
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
        connect(rbBlock, &QRadioButton::toggled, [=](){ useAllowMode = rbAllow->isChecked(); });
        
        l->addSpacing(15);
        QGridLayout* g = new QGridLayout();
        g->setSpacing(15);
        
        QString inputStyle = "padding: 8px; border: 1px solid #ccc; border-radius: 4px; background: #fff;";
        QString btnStyle = "background-color: #3B82F6; color: white; font-weight: bold; border-radius: 4px; padding: 8px 15px;";
        QString listStyle = "border: 1px solid #ccc; border-radius: 4px; background: #fff; padding: 5px;";
        
        auto makeList = [&](QString title, QLineEdit*& inA, QComboBox*& inW, QListWidget*& lA, QListWidget*& lW, int col) {
            g->addWidget(new QLabel("<b>" + title + " Apps (.exe):</b>"), 0, col);
            QHBoxLayout* h = new QHBoxLayout(); inA = new QLineEdit(); inA->setStyleSheet(inputStyle); QPushButton* bA = new QPushButton("Add"); bA->setStyleSheet(btnStyle); h->addWidget(inA); h->addWidget(bA); g->addLayout(h, 1, col);
            lA = new QListWidget(); lA->setStyleSheet(listStyle); g->addWidget(lA, 2, col);
            QPushButton* rA = new QPushButton("Remove Selected"); rA->setStyleSheet("background-color: #64748B; color: white; border-radius: 4px; padding: 5px;"); g->addWidget(rA, 3, col);
            
            g->addWidget(new QLabel("<b>" + title + " Websites:</b>"), 4, col);
            QHBoxLayout* h2 = new QHBoxLayout(); inW = new QComboBox(); inW->setEditable(true); inW->setStyleSheet(inputStyle); QPushButton* bW = new QPushButton("Add"); bW->setStyleSheet(btnStyle); h2->addWidget(inW); h2->addWidget(bW); g->addLayout(h2, 5, col);
            lW = new QListWidget(); lW->setStyleSheet(listStyle); g->addWidget(lW, 6, col);
            QPushButton* rW = new QPushButton("Remove Selected"); rW->setStyleSheet("background-color: #64748B; color: white; border-radius: 4px; padding: 5px;"); g->addWidget(rW, 7, col);
            
            connect(bA, &QPushButton::clicked, [=](){ if(!inA->text().isEmpty()){ lA->addItem(inA->text()); inA->clear(); } });
            connect(bW, &QPushButton::clicked, [=](){ if(!inW->currentText().isEmpty()){ lW->addItem(inW->currentText()); inW->setCurrentText(""); } });
            connect(rA, &QPushButton::clicked, [=](){ delete lA->takeItem(lA->currentRow()); });
            connect(rW, &QPushButton::clicked, [=](){ delete lW->takeItem(lW->currentRow()); });
        };
        
        makeList("Block", inBlockApp, inBlockWeb, listBlockApp, listBlockWeb, 0);
        
        g->addWidget(new QLabel("<b>Running Apps (Auto-Detected):</b>"), 0, 1);
        listRunning = new QListWidget(); listRunning->setStyleSheet(listStyle); g->addWidget(listRunning, 1, 1, 6, 1);
        QPushButton* bRun = new QPushButton("Add Selected to Current List"); bRun->setStyleSheet(btnStyle); g->addWidget(bRun, 7, 1);
        connect(bRun, &QPushButton::clicked, [=](){ 
            if(!listRunning->currentItem()) return; 
            QString app = listRunning->currentItem()->text();
            if(useAllowMode) { listAllowApp->addItem(app); } else { listBlockApp->addItem(app); }
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
        
        QString toggleStyle = R"(
            QCheckBox { font-size: 16px; padding: 15px; background: #ffffff; border: 1px solid #E2E8F0; border-radius: 8px; margin-bottom: 10px; font-weight: bold; color: #334155; }
            QCheckBox::indicator { width: 45px; height: 24px; border-radius: 12px; }
            QCheckBox::indicator:unchecked { image: none; background-color: #CBD5E1; }
            QCheckBox::indicator:checked { image: none; background-color: #10B981; }
            QCheckBox:hover { background: #F1F5F9; }
        )";
        
        chkReels = new QCheckBox("   Block Facebook Reels, Watch & Videos (Strict Mode)"); chkReels->setStyleSheet(toggleStyle); chkReels->setCursor(Qt::PointingHandCursor);
        chkShorts = new QCheckBox("   Block YouTube Shorts"); chkShorts->setStyleSheet(toggleStyle); chkShorts->setCursor(Qt::PointingHandCursor);
        chkAdblock = new QCheckBox("   System-wide AD BLOCKER (Silent Install)"); chkAdblock->setStyleSheet(toggleStyle); chkAdblock->setCursor(Qt::PointingHandCursor);
        
        l->addWidget(chkReels); l->addWidget(chkShorts); l->addWidget(chkAdblock);
        
        connect(chkReels, &QCheckBox::clicked, [=](bool c){ blockReels = c; });
        connect(chkShorts, &QCheckBox::clicked, [=](bool c){ blockShorts = c; });
        connect(chkAdblock, &QCheckBox::clicked, [=](bool c){ isAdblockActive = c; ToggleAdBlock(c); });
        
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
            if(!isSessionActive) {
                pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value();
                isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1;
                QMessageBox::information(this, "Started", "Pomodoro Started!"); 
            }
        });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { isSessionActive = false; isPomodoroMode = false; QMessageBox::information(this, "Stopped", "Pomodoro Stopped!"); } });
        
        // STOPWATCH
        QFrame* line1 = new QFrame(); line1->setFrameShape(QFrame::HLine); line1->setStyleSheet("color: #ccc; margin: 20px 0;"); l->addWidget(line1);
        QLabel* st = new QLabel("⏱️ Pro Stopwatch"); st->setStyleSheet("font-size: 20px; font-weight: bold; color: #334155; margin-bottom: 10px;"); l->addWidget(st);
        
        QPushButton* bOpenSw = new QPushButton("Open Pro Stopwatch");
        bOpenSw->setStyleSheet("background: #3B82F6; color: white; padding: 12px; font-weight: bold; border-radius: 5px; width: 200px;");
        l->addWidget(bOpenSw, 0, Qt::AlignLeft);
        
        connect(bOpenSw, &QPushButton::clicked, [=](){
            if(!swWindow) swWindow = new StopwatchWindow();
            swWindow->showNormal();
            swWindow->activateWindow();
        });
        
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
        
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; ApplyEyeFilters(); });
        connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; ApplyEyeFilters(); });
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
        chatLog->setStyleSheet("background: #ffffff; border: 1px solid #CBD5E1; border-radius: 8px; padding: 10px; font-size: 14px;");
        l->addWidget(chatLog);

        QHBoxLayout* inputLayout = new QHBoxLayout();
        chatIn = new QLineEdit();
        chatIn->setStyleSheet("padding: 10px; border: 1px solid #ccc; border-radius: 6px; background: #fff;");
        QPushButton* btnSend = new QPushButton("Send");
        btnSend->setStyleSheet("background-color: #2563EB; color: white; padding: 10px 20px; border-radius: 6px; font-weight: bold;");
        
        inputLayout->addWidget(chatIn);
        inputLayout->addWidget(btnSend);
        l->addLayout(inputLayout);

        connect(btnSend, &QPushButton::clicked, [=]() {
            if(!chatIn->text().isEmpty()) {
                chatLog->append("<b>You:</b> " + chatIn->text());
                chatIn->clear();
                // Network call to send logic here
            }
        });

        stack->addWidget(page);
    }

    void setupUpgradePage() {
        QWidget* page = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(page);
        l->setContentsMargins(40, 40, 40, 40);
        
        QLabel* title = new QLabel("⭐ Premium Upgrade");
        title->setStyleSheet("font-size: 24px; font-weight: bold; color: #F59E0B; margin-bottom: 20px;");
        l->addWidget(title);

        QLabel* info = new QLabel("Upgrade to PRO to unlock lifetime access, admin chat, and cloud sync.");
        info->setStyleSheet("font-size: 15px; margin-bottom: 20px;");
        l->addWidget(info);

        l->addStretch();
        stack->addWidget(page);
    }

    void onStartFocus() {
        if(isSessionActive) return;

        int hrs = spinHr->value();
        int mins = spinMin->value();
        
        if(hrs == 0 && mins == 0) {
            QMessageBox::warning(this, "Error", "Please set a valid time duration.");
            return;
        }

        focusTimeTotalSeconds = (hrs * 3600) + (mins * 60);
        timerTicks = 0;
        isSessionActive = true;
        isTimeMode = true;
        
        lblStatus->setText("");
        lblTimer->setText("Focusing... Please minimize distractions.");
        
        // Hide to tray automatically upon starting
        this->hide();
        trayIcon->showMessage("Focus Started", "RasFocus Pro is running in the background.", QSystemTrayIcon::Information, 3000);
    }

    void onStopFocus() {
        if(!isSessionActive) return;
        
        isSessionActive = false;
        isTimeMode = false;
        isPomodoroMode = false;
        lblTimer->setText("Status: Stopped");
    }

    // Dummy loops just to keep the structure complete
    void fastLoop() {
        // App monitoring, tab closing logic goes here
    }

    void slowLoop() {
        if(isSessionActive && isTimeMode) {
            timerTicks++;
            int remaining = focusTimeTotalSeconds - timerTicks;
            if(remaining <= 0) {
                onStopFocus();
                QMessageBox::information(this, "Success", "Focus session completed!");
            } else {
                int h = remaining / 3600;
                int m = (remaining % 3600) / 60;
                int s = remaining % 60;
                lblTimer->setText(QString("Time Left: %1h %2m %3s").arg(h).arg(m).arg(s));
            }
        }
    }

    void syncLoop() {
        // Background sync logic goes here
    }
};

// ==========================================
// MAIN FUNCTION & SINGLE INSTANCE
// ==========================================
int main(int argc, char *argv[]) {
    // Single Instance Check (Wake Up Bug Fix)
    HWND existingApp = FindWindowW(NULL, L"RasFocus Pro - Dashboard");
    if (existingApp) {
        // If app is already running, restore it and bring to front
        ShowWindow(existingApp, SW_RESTORE);
        SetForegroundWindow(existingApp);
        return 0; // Exit this new instance
    }

    QApplication a(argc, argv);
    
    // Set modern application font globally
    QFont modernFont("Segoe UI", 10);
    a.setFont(modernFont);

    RasFocusApp w;
    w.show();

    return a.exec();
}
