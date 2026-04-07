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
#include <QSharedMemory>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

// Windows API
#include <windows.h>
#include <tlhelp32.h>
#include <wininet.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")

using namespace std;

// ==========================================
// GLOBALS & DATA
// ==========================================
QStringList blockedApps, blockedWebs, allowedApps, allowedWebs;
QStringList systemApps = { "explorer.exe", "svchost.exe", "taskmgr.exe", "cmd.exe", "conhost.exe", "csrss.exe", "dwm.exe", "lsass.exe", "services.exe", "smss.exe", "wininit.exe", "winlogon.exe", "spoolsv.exe", "fontdrvhost.exe", "searchui.exe", "searchindexer.exe", "sihost.exe", "taskhostw.exe", "ctfmon.exe", "applicationframehost.exe", "system", "registry", "audiodg.exe", "searchapp.exe", "startmenuexperiencehost.exe", "shellexperiencehost.exe", "textinputhost.exe" };
QStringList explicitKeywords = { "porn", "xxx", "sex", "nude", "nsfw", "xvideos", "pornhub", "xnxx", "xhamster", "brazzers", "onlyfans", "playboy", "mia khalifa", "bhabi", "chudai", "bangla choti", "magi", "sexy" };
QStringList timeQuotes = { "\"যারা সময়কে মূল্যায়ন করে না, সময়ও তাদেরকে মূল্যায়ন করে না।\" - এ.পি.জে. আবদুল কালাম" };

bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false, isOverlayVisible = false;
bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false;
bool isPomodoroMode = false, isPomodoroBreak = false;

int eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;

QString currentSessionPass = "", userProfileName = "", safeAdminMsg = "", currentDisplayQuote = "";
QString lastAdminChat = "", pendingAdminChatStr = "", currentBroadcastMsg = "", pendingBroadcastMsg = "";
bool isLicenseValid = false, isTrialExpired = false;
int trialDaysLeft = 7, pendingAdminCmd = 0;

QWidget *dimFilterWidget = nullptr, *warmFilterWidget = nullptr, *overlayWidget = nullptr;
QLabel *overlayLabel = nullptr;
QTimer *overlayHideTimer = nullptr;
HHOOK hKeyboardHook;
QString globalKeyBuffer = "";
QMap<QString, int> usageStats;
DWORD lastUsageUpdate = 0;

// ==========================================
// UTILITY FUNCTIONS & PREMIUM SHADOWS
// ==========================================
void applyShadow(QWidget* widget) {
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setXOffset(0); shadow->setYOffset(5);
    shadow->setColor(QColor(0, 0, 0, 40));
    widget->setGraphicsEffect(shadow);
}

void runPowerShell(QString cmdBody) {
    QProcess::startDetached("powershell.exe", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << cmdBody);
}

QString GetDeviceID() {
    char compName[MAX_COMPUTERNAME_LENGTH + 1]; DWORD size = sizeof(compName); GetComputerNameA(compName, &size);
    DWORD volSerial = 0; GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char id[256]; sprintf(id, "%s-%X", compName, volSerial); return QString(id);
}

QString GetSecretDir() {
    QString dir = "C:/ProgramData/SysCache_Ras";
    QDir().mkpath(dir); SetFileAttributesA(dir.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "/";
}

bool CheckMatch(QString url, QString sTitle) { 
    QString t = sTitle.remove(' '); QString exact = url.toLower().remove(' '); 
    if (!exact.isEmpty() && t.contains(exact)) return true; 
    QString s = url.toLower(); s.replace('.', ' ').replace('/', ' ').replace(':', ' ').replace('-', ' '); 
    QStringList words = s.split(' ', Qt::SkipEmptyParts); 
    for(const QString& word : words) { 
        if (word == "https" || word == "http" || word == "www" || word == "com") continue; 
        if (word.length() >= 3 && t.contains(word)) return true; 
    } 
    return false; 
}

void CloseActiveTabAndMinimize(HWND hBrowser) {
    if (GetForegroundWindow() == hBrowser) {
        keybd_event(VK_CONTROL, 0, 0, 0); keybd_event('W', 0, 0, 0);
        keybd_event('W', 0, KEYEVENTF_KEYUP, 0); keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(50);
    }
    ShowWindow(hBrowser, SW_MINIMIZE);
}

void ShowCustomOverlay(int type) {
    if(!overlayWidget) {
        overlayWidget = new QWidget();
        overlayWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
        overlayWidget->setAttribute(Qt::WA_TranslucentBackground);
        overlayWidget->resize(900, 400); 
        
        QVBoxLayout* l = new QVBoxLayout(overlayWidget);
        QWidget* bg = new QWidget(); bg->setObjectName("bg");
        QVBoxLayout* bl = new QVBoxLayout(bg); bl->setContentsMargins(50, 50, 50, 50);
        
        overlayLabel = new QLabel(); overlayLabel->setAlignment(Qt::AlignCenter); overlayLabel->setWordWrap(true);
        overlayLabel->setFont(QFont("Segoe UI", 28, QFont::Bold)); overlayLabel->setStyleSheet("color: white;"); 
        bl->addWidget(overlayLabel); l->addWidget(bg);
        
        overlayHideTimer = new QTimer();
        QObject::connect(overlayHideTimer, &QTimer::timeout, [](){ overlayWidget->hide(); isOverlayVisible = false; overlayHideTimer->stop(); });
    }
    
    currentDisplayQuote = timeQuotes[0]; overlayLabel->setText(currentDisplayQuote);
    if(type == 1) overlayWidget->setStyleSheet("#bg { background-color: #0f172a; border: 6px solid #ef4444; border-radius: 20px; }");
    else overlayWidget->setStyleSheet("#bg { background-color: #1e293b; border: 6px solid #3b82f6; border-radius: 20px; }");
    
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    overlayWidget->move((screenRect.width() - overlayWidget->width()) / 2, (screenRect.height() - overlayWidget->height()) / 2);
    
    isOverlayVisible = true; overlayWidget->show(); overlayHideTimer->start(6000);
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

// -------------------------------
// EYE CURE LOGIC 
// -------------------------------
void ApplyEyeFilters() {
    if(!dimFilterWidget) {
        dimFilterWidget = new QWidget();
        dimFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput); 
        dimFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        dimFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); 
        dimFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    if(!warmFilterWidget) {
        warmFilterWidget = new QWidget();
        warmFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput); 
        warmFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        warmFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); 
        warmFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    
    int dimAlpha = (100 - eyeBrightness) * 2.5; 
    if(dimAlpha < 0) dimAlpha = 0; if(dimAlpha > 240) dimAlpha = 240;
    if(dimAlpha > 0) { 
        dimFilterWidget->setStyleSheet(QString("background-color: rgba(0, 0, 0, %1);").arg(dimAlpha)); 
        dimFilterWidget->show(); 
    } else {
        dimFilterWidget->hide();
    }

    int warmAlpha = eyeWarmth * 1.5; 
    if(warmAlpha < 0) warmAlpha = 0; if(warmAlpha > 180) warmAlpha = 180;
    if(warmAlpha > 0) { 
        warmFilterWidget->setStyleSheet(QString("background-color: rgba(255, 130, 0, %1);").arg(warmAlpha)); 
        warmFilterWidget->show(); 
    } else {
        warmFilterWidget->hide(); 
    }
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
// FLOATING STOPWATCH
// ==========================================
class StopwatchWindow : public QWidget {
private:
    QPoint m_dragPosition;
    bool m_dragActive = false;

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && event->pos().y() < 40) { 
            m_dragActive = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (m_dragActive && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPos() - m_dragPosition);
            event->accept();
        }
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        m_dragActive = false;
    }

public:
    QLabel *lblSw; QElapsedTimer timer; QTimer *updateTimer; bool isRunning = false; qint64 pausedTime = 0;
    StopwatchWindow() {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint); 
        resize(450, 200);
        setStyleSheet("background-color: #0F172A; border-radius: 12px;");
        
        QVBoxLayout* mainL = new QVBoxLayout(this);
        mainL->setContentsMargins(0,0,0,0);
        
        QWidget* topBar = new QWidget();
        topBar->setFixedHeight(40);
        topBar->setStyleSheet("background-color: #1E293B; border-top-left-radius: 12px; border-top-right-radius: 12px;");
        QHBoxLayout* barL = new QHBoxLayout(topBar);
        barL->setContentsMargins(15,0,15,0);
        QLabel* barTitle = new QLabel("⏱️ Pro Stopwatch");
        barTitle->setStyleSheet("color: white; font-weight: bold; font-size: 15px;");
        barL->addWidget(barTitle);
        barL->addStretch();
        
        QPushButton* btnMin = new QPushButton("—");
        btnMin->setFixedSize(30, 30);
        btnMin->setStyleSheet("QPushButton { background: transparent; color: white; font-weight: bold; font-size: 18px; border: none; } QPushButton:hover { background: #334155; border-radius: 5px; }");
        barL->addWidget(btnMin);
        mainL->addWidget(topBar);

        QWidget* body = new QWidget();
        QVBoxLayout* l = new QVBoxLayout(body);
        l->setContentsMargins(20,20,20,20);
        
        lblSw = new QLabel("00:00:00.00"); lblSw->setAlignment(Qt::AlignCenter);
        lblSw->setStyleSheet("font-size: 54px; font-family: 'Consolas'; font-weight: bold; color: #38BDF8;"); l->addWidget(lblSw);
        
        QHBoxLayout* h = new QHBoxLayout();
        QPushButton* btnStart = new QPushButton("Start / Pause"); QPushButton* btnReset = new QPushButton("Reset");
        btnStart->setStyleSheet("background: #3B82F6; color: white; padding: 15px; font-weight: bold; border-radius: 8px; font-size: 16px;");
        btnReset->setStyleSheet("background: #64748B; color: white; padding: 15px; font-weight: bold; border-radius: 8px; font-size: 16px;");
        h->addWidget(btnStart); h->addWidget(btnReset); l->addLayout(h);
        mainL->addWidget(body);

        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, [=](){
            if(isRunning) {
                qint64 el = timer.elapsed() + pausedTime; int ms = (el % 1000) / 10; int s = (el / 1000) % 60; int m = (el / 60000) % 60; int hr = (el / 3600000);
                lblSw->setText(QString("%1:%2:%3.%4").arg(hr,2,10,QChar('0')).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')).arg(ms,2,10,QChar('0')));
            }
        });
        
        connect(btnStart, &QPushButton::clicked, [=](){ if(isRunning){isRunning=false; pausedTime+=timer.elapsed(); updateTimer->stop();} else{isRunning=true; timer.start(); updateTimer->start(30);} });
        connect(btnReset, &QPushButton::clicked, [=](){ isRunning=false; pausedTime=0; updateTimer->stop(); lblSw->setText("00:00:00.00"); });
        connect(btnMin, &QPushButton::clicked, [=](){ this->hide(); });
    }
};
StopwatchWindow* swWindow = nullptr;

// ==========================================
// MAIN GUI CLASS
// ==========================================
class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack; QListWidget* sidebar; QSystemTrayIcon* trayIcon;
    QTimer *fastTimer, *slowTimer, *syncTimer;
    QLineEdit *editName, *editPass; QSpinBox *spinHr, *spinMin;
    QPushButton *btnStart, *btnStop; QLabel *lblStatus, *lblTimer, *lblLicense, *lblAdminMsg;
    QRadioButton *rbBlock, *rbAllow; QListWidget *listBlockApp, *listBlockWeb, *listAllowApp, *listAllowWeb, *listRunning;
    QLineEdit *inBlockApp, *inAllowApp; QComboBox *inBlockWeb, *inAllowWeb;
    QCheckBox *chkReels, *chkShorts, *chkAdblock; QSpinBox *pomoMin, *pomoSes; QLabel *lblPomoStatus;
    QSlider *sliderBright, *sliderWarm; QTextEdit *chatLog; QLineEdit *chatIn;
    QLineEdit *upgEmail, *upgPhone, *upgTrx; QComboBox *upgPkg;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override {
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override {
#endif
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_USER + 2) {
            this->setWindowState((this->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            this->showNormal(); this->activateWindow(); this->raise(); 
            if(swWindow && swWindow->isRunning) swWindow->showNormal(); 
            return true;
        }
        return QMainWindow::nativeEvent(eventType, message, result);
    }

    RasFocusApp() {
        setWindowTitle("RasFocus Pro - Dashboard"); 
        resize(1200, 800); 
        setStyleSheet("QMainWindow { background-color: #F8FAFC; } QLabel { color: #1E293B; font-family: 'Segoe UI'; }");
        
        QWidget* central = new QWidget(); setCentralWidget(central);
        QHBoxLayout* mainLayout = new QHBoxLayout(central); mainLayout->setContentsMargins(0,0,0,0); mainLayout->setSpacing(0);
        
        sidebar = new QListWidget(); sidebar->setFixedWidth(300); 
        sidebar->setStyleSheet(R"(
            QListWidget { background-color: #0F172A; color: #94A3B8; border: none; font-size: 18px; padding-top: 30px; outline: none; }
            QListWidget::item { padding: 25px 30px; border-left: 5px solid transparent; }
            QListWidget::item:hover { background-color: #1E293B; color: #F8FAFC; }
            QListWidget::item:selected { background-color: #1E293B; color: #38BDF8; border-left: 5px solid #38BDF8; font-weight: bold; }
        )");
        sidebar->addItem("  📊 Overview"); sidebar->addItem("  🛡️ Focus Lists"); sidebar->addItem("  ⚙️ Strict Controls");
        sidebar->addItem("  ⏱️ Productivity Tools"); sidebar->addItem("  💬 Live Support"); sidebar->addItem("  ⭐ Premium");
        
        stack = new QStackedWidget(); stack->setStyleSheet("QStackedWidget { background-color: #F8FAFC; border-top-left-radius: 15px; }");
        
        setupOverviewPage(); setupListsPage(); setupAdvancedPage(); setupToolsPage(); setupChatPage(); setupUpgradePage();
        
        mainLayout->addWidget(sidebar); mainLayout->addWidget(stack);
        connect(sidebar, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex); sidebar->setCurrentRow(0);
        
        setupTray(); LoadAllData(); ApplyEyeFilters();
        
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }

private:
    void setupOverviewPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(60, 50, 60, 50);
        
        QWidget* profileCard = new QWidget(); profileCard->setStyleSheet("background: white; border-radius: 12px;"); applyShadow(profileCard);
        QHBoxLayout* h1 = new QHBoxLayout(profileCard); h1->setContentsMargins(25,25,25,25);
        h1->addWidget(new QLabel("<b>Profile Name:</b>")); 
        editName = new QLineEdit(); editName->setStyleSheet("padding: 12px; border: 1px solid #E2E8F0; border-radius: 6px; font-size: 16px;"); h1->addWidget(editName);
        QPushButton* btnSave = new QPushButton("Save"); btnSave->setStyleSheet("background: #3B82F6; color: white; padding: 12px 25px; border-radius: 6px; font-weight: bold; font-size: 16px;"); h1->addWidget(btnSave);
        lblLicense = new QLabel("Checking License..."); lblLicense->setStyleSheet("font-weight: bold; font-size: 18px; color: #10B981; margin-left: 20px;"); h1->addWidget(lblLicense);
        l->addWidget(profileCard); connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); SaveAllData(); SyncProfileNameToFirebase(userProfileName); });
        
        l->addSpacing(40);
        
        QWidget* controlCard = new QWidget(); controlCard->setStyleSheet("background: white; border-radius: 12px;"); applyShadow(controlCard);
        QGridLayout* g = new QGridLayout(controlCard); g->setContentsMargins(25,25,25,25); g->setSpacing(25);
        g->addWidget(new QLabel("<b>Lock with Password:</b>"), 0, 0); 
        editPass = new QLineEdit(); editPass->setEchoMode(QLineEdit::Password); editPass->setStyleSheet("padding: 15px; border: 1px solid #E2E8F0; border-radius: 8px; font-size: 16px;"); g->addWidget(editPass, 1, 0);
        g->addWidget(new QLabel("<b>Time Limit (Hr : Min):</b>"), 0, 1); 
        QHBoxLayout* th = new QHBoxLayout(); 
        spinHr = new QSpinBox(); spinHr->setStyleSheet("padding: 15px; border: 1px solid #E2E8F0; border-radius: 8px; font-size: 16px;"); 
        spinMin = new QSpinBox(); spinMin->setMaximum(59); spinMin->setStyleSheet("padding: 15px; border: 1px solid #E2E8F0; border-radius: 8px; font-size: 16px;"); 
        th->addWidget(spinHr); th->addWidget(spinMin); g->addLayout(th, 1, 1);
        l->addWidget(controlCard);
        
        l->addSpacing(40);
        
        QHBoxLayout* h2 = new QHBoxLayout();
        btnStart = new QPushButton("▶ START FOCUS"); btnStart->setCursor(Qt::PointingHandCursor); btnStart->setStyleSheet("background: #10B981; color: white; padding: 20px 40px; font-size: 18px; font-weight: bold; border-radius: 10px;"); applyShadow(btnStart);
        btnStop = new QPushButton("⏹ STOP SESSION"); btnStop->setCursor(Qt::PointingHandCursor); btnStop->setStyleSheet("background: #EF4444; color: white; padding: 20px 40px; font-size: 18px; font-weight: bold; border-radius: 10px;"); applyShadow(btnStop);
        h2->addWidget(btnStart); h2->addWidget(btnStop); h2->addStretch(); l->addLayout(h2);
        
        lblStatus = new QLabel(""); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold; font-size: 16px; margin-top: 20px;"); l->addWidget(lblStatus);
        lblTimer = new QLabel("Status: Ready"); lblTimer->setStyleSheet("font-size: 28px; font-weight: bold; color: #3B82F6; margin-top: 25px;"); l->addWidget(lblTimer);
        lblAdminMsg = new QLabel(""); lblAdminMsg->setStyleSheet("color: #8B5CF6; font-weight: bold; font-size: 18px; margin-top: 20px;"); l->addWidget(lblAdminMsg);
        
        l->addStretch(); stack->addWidget(page);
        connect(btnStart, &QPushButton::clicked, this, &RasFocusApp::onStartFocus); connect(btnStop, &QPushButton::clicked, this, &RasFocusApp::onStopFocus);
    }

    void setupListsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 40, 50, 40);
        
        QWidget* modeCard = new QWidget(); modeCard->setStyleSheet("background: white; border-radius: 10px; padding: 15px;"); applyShadow(modeCard);
        QHBoxLayout* radL = new QHBoxLayout(modeCard);
        rbBlock = new QRadioButton("Block List Mode (Default)"); rbAllow = new QRadioButton("Allow List Mode (Strict)");
        rbBlock->setChecked(true); radL->addWidget(rbBlock); radL->addWidget(rbAllow); l->addWidget(modeCard);
        connect(rbBlock, &QRadioButton::toggled, [=](){ useAllowMode = rbAllow->isChecked(); SaveAllData(); });
        
        l->addSpacing(20);
        QGridLayout* g = new QGridLayout(); g->setSpacing(20);
        QString inStyle = "padding: 12px; border: 1px solid #CBD5E1; border-radius: 8px; background: white; font-size: 16px;";
        QString btnStyle = "background: #3B82F6; color: white; font-weight: bold; border-radius: 8px; padding: 12px 20px; font-size: 16px;";
        QString lstStyle = "border: 1px solid #CBD5E1; border-radius: 8px; background: white; padding: 8px; font-size: 16px;";
        
        auto makeList = [&](QString title, QLineEdit*& inA, QComboBox*& inW, QListWidget*& lA, QListWidget*& lW, int col) {
            g->addWidget(new QLabel("<b>" + title + " Apps (.exe):</b>"), 0, col);
            QHBoxLayout* h = new QHBoxLayout(); inA = new QLineEdit(); inA->setStyleSheet(inStyle); QPushButton* bA = new QPushButton("Add"); bA->setStyleSheet(btnStyle); h->addWidget(inA); h->addWidget(bA); g->addLayout(h, 1, col);
            lA = new QListWidget(); lA->setStyleSheet(lstStyle); g->addWidget(lA, 2, col);
            QPushButton* rA = new QPushButton("Remove Selected"); rA->setStyleSheet("background: #64748B; color: white; border-radius: 8px; padding: 10px; font-size: 15px;"); g->addWidget(rA, 3, col);
            g->addWidget(new QLabel("<b>" + title + " Websites:</b>"), 4, col);
            QHBoxLayout* h2 = new QHBoxLayout(); inW = new QComboBox(); inW->setEditable(true); inW->setStyleSheet(inStyle); QPushButton* bW = new QPushButton("Add"); bW->setStyleSheet(btnStyle); h2->addWidget(inW); h2->addWidget(bW); g->addLayout(h2, 5, col);
            lW = new QListWidget(); lW->setStyleSheet(lstStyle); g->addWidget(lW, 6, col);
            QPushButton* rW = new QPushButton("Remove Selected"); rW->setStyleSheet("background: #64748B; color: white; border-radius: 8px; padding: 10px; font-size: 15px;"); g->addWidget(rW, 7, col);
            connect(bA, &QPushButton::clicked, [=](){ if(!inA->text().isEmpty()){ lA->addItem(inA->text()); inA->clear(); SyncListsFromUI(); } });
            connect(bW, &QPushButton::clicked, [=](){ if(!inW->currentText().isEmpty()){ lW->addItem(inW->currentText()); inW->setCurrentText(""); SyncListsFromUI(); } });
            connect(rA, &QPushButton::clicked, [=](){ delete lA->takeItem(lA->currentRow()); SyncListsFromUI(); });
            connect(rW, &QPushButton::clicked, [=](){ delete lW->takeItem(lW->currentRow()); SyncListsFromUI(); });
        };
        makeList("Block", inBlockApp, inBlockWeb, listBlockApp, listBlockWeb, 0);
        g->addWidget(new QLabel("<b>Running Apps (Auto-Detect):</b>"), 0, 1);
        listRunning = new QListWidget(); listRunning->setStyleSheet(lstStyle); g->addWidget(listRunning, 1, 1, 6, 1);
        QPushButton* bRun = new QPushButton("Add Selected to List"); bRun->setStyleSheet(btnStyle); g->addWidget(bRun, 7, 1);
        connect(bRun, &QPushButton::clicked, [=](){ if(!listRunning->currentItem()) return; QString app = listRunning->currentItem()->text(); if(useAllowMode) listAllowApp->addItem(app); else listBlockApp->addItem(app); SyncListsFromUI(); });
        makeList("Allow", inAllowApp, inAllowWeb, listAllowApp, listAllowWeb, 2);
        l->addLayout(g);
        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com", "reddit.com", "netflix.com"};
        inBlockWeb->addItems(popSites); inAllowWeb->addItems(popSites); inBlockWeb->setCurrentText(""); inAllowWeb->setCurrentText("");
        stack->addWidget(page);
    }

    void setupAdvancedPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(80, 80, 80, 80);
        QLabel* title = new QLabel("Strict Blocking Features"); title->setStyleSheet("font-size: 30px; font-weight: bold; color: #0F172A; margin-bottom: 10px;"); l->addWidget(title);
        QLabel* sub = new QLabel("Turn on these filters to stop distractions completely."); sub->setStyleSheet("color: #64748B; margin-bottom: 40px; font-size: 18px;"); l->addWidget(sub);
        QString tStyle = "QCheckBox { font-size: 20px; padding: 25px; background: white; border: 1px solid #E2E8F0; border-radius: 12px; margin-bottom: 20px; font-weight: 500; color: #1E293B; } QCheckBox::indicator { width: 45px; height: 25px; } QCheckBox:hover { border-color: #3B82F6; }";
        chkReels = new QCheckBox("  Block Facebook Reels, Watch & Videos"); chkReels->setStyleSheet(tStyle); chkReels->setCursor(Qt::PointingHandCursor); applyShadow(chkReels);
        chkShorts = new QCheckBox("  Block YouTube Shorts"); chkShorts->setStyleSheet(tStyle); chkShorts->setCursor(Qt::PointingHandCursor); applyShadow(chkShorts);
        chkAdblock = new QCheckBox("  System-wide AD BLOCKER (Silent Mode)"); chkAdblock->setStyleSheet(tStyle); chkAdblock->setCursor(Qt::PointingHandCursor); applyShadow(chkAdblock);
        l->addWidget(chkReels); l->addWidget(chkShorts); l->addWidget(chkAdblock);
        connect(chkReels, &QCheckBox::clicked, [=](bool c){ blockReels = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkShorts, &QCheckBox::clicked, [=](bool c){ blockShorts = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkAdblock, &QCheckBox::clicked, [=](bool c){ isAdblockActive = c; ToggleAdBlock(c); SaveAllData(); SyncTogglesToFirebase(); });
        l->addStretch(); stack->addWidget(page);
    }

    void setupToolsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(60, 50, 60, 50);
        
        QWidget* pCard = new QWidget(); pCard->setStyleSheet("background: white; border-radius: 12px;"); applyShadow(pCard);
        QVBoxLayout* pL = new QVBoxLayout(pCard); pL->setContentsMargins(25,25,25,25);
        QLabel* pt = new QLabel("🍅 Pomodoro Productivity"); pt->setStyleSheet("font-size: 26px; font-weight: bold; color: #EF4444; margin-bottom: 20px;"); pL->addWidget(pt);
        QHBoxLayout* ph = new QHBoxLayout();
        ph->addWidget(new QLabel("<b>Focus Length (Min):</b>")); pomoMin = new QSpinBox(); pomoMin->setValue(25); pomoMin->setStyleSheet("padding: 12px; border: 1px solid #ccc; border-radius: 8px; font-size: 16px;"); ph->addWidget(pomoMin);
        ph->addSpacing(30);
        ph->addWidget(new QLabel("<b>Total Sessions:</b>")); pomoSes = new QSpinBox(); pomoSes->setValue(4); pomoSes->setStyleSheet("padding: 12px; border: 1px solid #ccc; border-radius: 8px; font-size: 16px;"); ph->addWidget(pomoSes); ph->addStretch();
        pL->addLayout(ph);
        QHBoxLayout* pBtnH = new QHBoxLayout();
        QPushButton* bPStart = new QPushButton("Start Pomodoro"); bPStart->setStyleSheet("background: #10B981; color: white; padding: 15px 25px; font-weight: bold; font-size: 16px; border-radius: 8px;");
        QPushButton* bPStop = new QPushButton("Stop"); bPStop->setStyleSheet("background: #EF4444; color: white; padding: 15px 25px; font-weight: bold; font-size: 16px; border-radius: 8px;");
        pBtnH->addWidget(bPStart); pBtnH->addWidget(bPStop); pBtnH->addStretch(); pL->addLayout(pBtnH);
        lblPomoStatus = new QLabel("Status: Ready"); lblPomoStatus->setStyleSheet("color: #64748B; font-weight: bold; font-size: 18px; margin-top: 15px;"); pL->addWidget(lblPomoStatus);
        l->addWidget(pCard);
        connect(bPStart, &QPushButton::clicked, [=](){ if(!isSessionActive && !isTrialExpired) { pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value(); isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1; SaveAllData(); QMessageBox::information(this, "Started", "Pomodoro Active!"); updateUIStates(); } });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { ClearSessionData(); updateUIStates(); } });
        
        l->addSpacing(30);
        QWidget* eCard = new QWidget(); eCard->setStyleSheet("background: white; border-radius: 12px;"); applyShadow(eCard);
        QVBoxLayout* eL = new QVBoxLayout(eCard); eL->setContentsMargins(25,25,25,25);
        QLabel* et = new QLabel("👁️ Eye Comfort Filters & Tools"); et->setStyleSheet("font-size: 26px; font-weight: bold; color: #8B5CF6; margin-bottom: 20px;"); eL->addWidget(et);
        
        QPushButton* bOpenSw = new QPushButton("⏱️ Open Pro Stopwatch"); bOpenSw->setStyleSheet("background: #3B82F6; color: white; padding: 15px 25px; font-weight: bold; font-size: 16px; border-radius: 8px; margin-bottom: 25px;"); eL->addWidget(bOpenSw, 0, Qt::AlignLeft);
        connect(bOpenSw, &QPushButton::clicked, [=](){ if(!swWindow) swWindow = new StopwatchWindow(); swWindow->showNormal(); swWindow->activateWindow(); swWindow->raise(); });
        
        eL->addWidget(new QLabel("<b>Screen Brightness:</b>")); sliderBright = new QSlider(Qt::Horizontal); sliderBright->setRange(10, 100); sliderBright->setValue(100); eL->addWidget(sliderBright);
        eL->addWidget(new QLabel("<b>Night Warmth:</b>")); sliderWarm = new QSlider(Qt::Horizontal); sliderWarm->setRange(0, 100); sliderWarm->setValue(0); eL->addWidget(sliderWarm);
        QHBoxLayout* eh = new QHBoxLayout();
        QPushButton* bD = new QPushButton("Day"); bD->setStyleSheet("background: #F59E0B; color: white; padding: 12px 20px; border-radius: 6px; font-size: 16px;");
        QPushButton* bR = new QPushButton("Reading"); bR->setStyleSheet("background: #8B5CF6; color: white; padding: 12px 20px; border-radius: 6px; font-size: 16px;");
        QPushButton* bN = new QPushButton("Night"); bN->setStyleSheet("background: #1E293B; color: white; padding: 12px 20px; border-radius: 6px; font-size: 16px;");
        eh->addWidget(bD); eh->addWidget(bR); eh->addWidget(bN); eh->addStretch(); eL->addLayout(eh);
        l->addWidget(eCard);
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; ApplyEyeFilters(); SaveAllData(); });
        connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; ApplyEyeFilters(); SaveAllData(); });
        connect(bD, &QPushButton::clicked, [=](){ sliderBright->setValue(100); sliderWarm->setValue(0); });
        connect(bR, &QPushButton::clicked, [=](){ sliderBright->setValue(85); sliderWarm->setValue(30); });
        connect(bN, &QPushButton::clicked, [=](){ sliderBright->setValue(60); sliderWarm->setValue(75); });
        
        l->addStretch(); stack->addWidget(page);
    }

    void setupChatPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(60, 50, 60, 50);
        QLabel* title = new QLabel("💬 Live Chat Support"); title->setStyleSheet("font-size: 30px; font-weight: bold; color: #EC4899; margin-bottom: 25px;"); l->addWidget(title);
        chatLog = new QTextEdit(); chatLog->setReadOnly(true); chatLog->setStyleSheet("background: white; border: 1px solid #CBD5E1; border-radius: 12px; padding: 20px; font-size: 18px;"); applyShadow(chatLog); l->addWidget(chatLog);
        QHBoxLayout* ch = new QHBoxLayout();
        chatIn = new QLineEdit(); chatIn->setStyleSheet("padding: 20px; border: 1px solid #CBD5E1; border-radius: 10px; font-size: 18px; background: white;"); applyShadow(chatIn); ch->addWidget(chatIn);
        QPushButton* bSend = new QPushButton("Send Message"); bSend->setStyleSheet("background: #EC4899; color: white; font-weight: bold; font-size: 18px; padding: 20px 30px; border-radius: 10px;"); applyShadow(bSend); ch->addWidget(bSend);
        l->addLayout(ch);
        connect(bSend, &QPushButton::clicked, [=](){
            QString msg = chatIn->text().trimmed(); if(!msg.isEmpty()) { chatLog->append("<b>You:</b> " + msg); chatIn->clear(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + msg + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        });
        stack->addWidget(page);
    }
    
    void setupUpgradePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(100, 60, 100, 60);
        QWidget* card = new QWidget(); card->setStyleSheet("background: white; border-radius: 15px;"); applyShadow(card);
        QVBoxLayout* cL = new QVBoxLayout(card); cL->setContentsMargins(40,40,40,40);
        QLabel* title = new QLabel("⭐ Unlock Premium"); title->setStyleSheet("font-size: 32px; font-weight: bold; color: #F59E0B; margin-bottom: 10px;"); cL->addWidget(title);
        QLabel* sub = new QLabel("Send payment via Nagad/bKash to <b>01566054963</b> and submit the form below.");
        sub->setStyleSheet("font-size: 18px;"); cL->addWidget(sub); cL->addSpacing(30);
        QString inS = "padding: 15px; border: 1px solid #E2E8F0; border-radius: 8px; font-size: 18px; margin-bottom: 20px;";
        cL->addWidget(new QLabel("<b>Email / Profile Name:</b>")); upgEmail = new QLineEdit(); upgEmail->setStyleSheet(inS); cL->addWidget(upgEmail);
        cL->addWidget(new QLabel("<b>Sender Phone Number:</b>")); upgPhone = new QLineEdit(); upgPhone->setStyleSheet(inS); cL->addWidget(upgPhone);
        cL->addWidget(new QLabel("<b>Transaction ID (TrxID):</b>")); upgTrx = new QLineEdit(); upgTrx->setStyleSheet(inS); cL->addWidget(upgTrx);
        cL->addWidget(new QLabel("<b>Select Package:</b>")); upgPkg = new QComboBox(); upgPkg->addItems({"7 Days Trial", "6 Months (50 BDT)", "1 Year (100 BDT)"}); upgPkg->setStyleSheet(inS); cL->addWidget(upgPkg);
        QPushButton* bSub = new QPushButton("SUBMIT VERIFICATION"); bSub->setStyleSheet("background: #10B981; color: white; padding: 22px; font-weight: bold; font-size: 20px; border-radius: 10px; margin-top: 20px;"); cL->addWidget(bSub);
        l->addWidget(card); l->addStretch();
        connect(bSub, &QPushButton::clicked, [=](){
            if(upgEmail->text().isEmpty() || upgPhone->text().isEmpty() || upgTrx->text().isEmpty()) { QMessageBox::warning(this, "Error", "Fill all fields!"); return; }
            QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            runPowerShell("$body = @{ fields = @{ deviceID = @{ stringValue = '" + dId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + upgPkg->currentText() + "' }; userEmail = @{ stringValue = '" + upgEmail->text() + "' }; senderPhone = @{ stringValue = '" + upgPhone->text() + "' }; trxId = @{ stringValue = '" + upgTrx->text() + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
            QMessageBox::information(this, "Success", "Upgrade Request Sent to Admin!");
        });
        stack->addWidget(page);
    }

    void setupTray() {
        QPixmap pixmap(64, 64); pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap); painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#2563EB")); painter.setPen(Qt::NoPen); painter.drawEllipse(2, 2, 60, 60);
        painter.setPen(Qt::white); painter.setFont(QFont("Segoe UI", 32, QFont::Bold)); painter.drawText(pixmap.rect(), Qt::AlignCenter, "R");
        
        QIcon fallbackIcon(pixmap); QIcon fileIcon("icon.ico");
        trayIcon = new QSystemTrayIcon(QFile::exists("icon.ico") ? fileIcon : fallbackIcon, this);
        
        QMenu* menu = new QMenu(this);
        menu->addAction("Open Dashboard", this, [=](){ 
            this->showNormal(); this->activateWindow(); this->raise(); 
            if(swWindow && swWindow->isRunning) swWindow->showNormal(); 
        });
        menu->addAction("Exit completely", qApp, &QCoreApplication::quit);
        trayIcon->setContextMenu(menu);
        trayIcon->show(); 
        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason r){ 
            if(r == QSystemTrayIcon::DoubleClick) { 
                this->showNormal(); this->activateWindow(); this->raise(); 
                if(swWindow && swWindow->isRunning) swWindow->showNormal(); 
            } 
        });
    }

    // ==========================================
    // LOGIC & SYNC
    // ==========================================
    void onStartFocus() {
        if(isSessionActive) return; QString p = editPass->text(); int tSec = (spinHr->value() * 3600) + (spinMin->value() * 60);
        if(p.isEmpty() && tSec == 0) { QMessageBox::warning(this, "Wait", "Set Password or Time limit!"); return; }
        useAllowMode = rbAllow->isChecked();
        if(!p.isEmpty()) { isPassMode = true; currentSessionPass = p; SyncPasswordToFirebase(p, true); } 
        else { isTimeMode = true; focusTimeTotalSeconds = tSec; timerTicks = 0; }
        isSessionActive = true; editPass->clear(); SaveAllData(); updateUIStates();
        trayIcon->showMessage("RasFocus is Active", "App is hidden in the tray to keep you focused.", QSystemTrayIcon::Information, 3000);
        this->hide();
    }
    
    void onStopFocus() {
        if(!isSessionActive) return;
        if(isTimeMode) { QMessageBox::warning(this, "Locked", "You must wait until the timer finishes!"); return; }
        if(isPassMode && editPass->text() == currentSessionPass) { ClearSessionData(); SyncPasswordToFirebase("", false); QMessageBox::information(this, "Done", "Focus session unlocked."); } 
        else { QMessageBox::critical(this, "Error", "Incorrect Password!"); }
    }

    QStringList GetRunningAppsUI() {
        QStringList p; HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)};
        if(Process32FirstW(h, &pe)) { do { QString n = QString::fromWCharArray(pe.szExeFile).toLower(); if(!systemApps.contains(n)) p << n; } while(Process32NextW(h, &pe)); }
        CloseHandle(h); p.removeDuplicates(); p.sort(Qt::CaseInsensitive); return p;
    }

    void SyncListsFromUI() {
        auto getList = [](QListWidget* lw) { QStringList l; for(int i=0; i<lw->count(); ++i) l << lw->item(i)->text(); return l; };
        blockedApps = getList(listBlockApp); blockedWebs = getList(listBlockWeb); allowedApps = getList(listAllowApp); allowedWebs = getList(listAllowWeb); SaveAllData();
    }

    void LoadAllData() {
        auto lF = [](QString fn, QStringList& l, QListWidget* lw) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream in(&f); while(!in.atEnd()) { QString v=in.readLine().trimmed(); if(!v.isEmpty()){ l<<v; lw->addItem(v); } } f.close(); } };
        lF("bl_app.dat", blockedApps, listBlockApp); lF("bl_web.dat", blockedWebs, listBlockWeb);
        lF("al_app.dat", allowedApps, listAllowApp); lF("al_web.dat", allowedWebs, listAllowWeb); listRunning->addItems(GetRunningAppsUI());
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QTextStream in(&f); int a=0, tm=0, pm=0, ua=0, po=0, pb=0, br=0, bs=0, ad=0, pc=1;
            in >> a >> tm >> pm >> currentSessionPass >> focusTimeTotalSeconds >> timerTicks >> ua >> po >> pb >> pomoTicks >> eyeBrightness >> eyeWarmth >> br >> bs >> ad >> pc;
            userProfileName = in.readLine().trimmed(); if(userProfileName.isEmpty()) userProfileName = in.readLine().trimmed(); 
            isSessionActive=(a==1); isTimeMode=(tm==1); isPassMode=(pm==1); useAllowMode=(ua==1); isPomodoroMode=(po==1); isPomodoroBreak=(pb==1); pomoCurrentSession=pc; blockReels=(br==1); blockShorts=(bs==1); isAdblockActive=(ad==1);
            rbAllow->setChecked(useAllowMode); chkReels->setChecked(blockReels); chkShorts->setChecked(blockShorts); chkAdblock->setChecked(isAdblockActive);
            sliderBright->setValue(eyeBrightness); sliderWarm->setValue(eyeWarmth); editName->setText(userProfileName); f.close();
        } updateUIStates();
    }

    void SaveAllData() {
        auto sF = [](QString fn, const QStringList& l) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::WriteOnly|QIODevice::Text)) { QTextStream out(&f); for(auto i:l) out<<i<<"\n"; f.close(); } };
        sF("bl_app.dat", blockedApps); sF("bl_web.dat", blockedWebs); sF("al_app.dat", allowedApps); sF("al_web.dat", allowedWebs);
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::WriteOnly|QIODevice::Text)) {
            QTextStream out(&f);
            out << (isSessionActive?1:0) << " " << (isTimeMode?1:0) << " " << (isPassMode?1:0) << " " << currentSessionPass << " " << focusTimeTotalSeconds << " " << timerTicks << " " << (useAllowMode?1:0) << " " << (isPomodoroMode?1:0) << " " << (isPomodoroBreak?1:0) << " " << pomoTicks << " " << eyeBrightness << " " << eyeWarmth << " " << (blockReels?1:0) << " " << (blockShorts?1:0) << " " << (isAdblockActive?1:0) << " " << pomoCurrentSession << "\n" << userProfileName << "\n"; f.close();
        }
    }

    void ClearSessionData() { isSessionActive = isTimeMode = isPassMode = isPomodoroMode = isPomodoroBreak = false; currentSessionPass = ""; focusTimeTotalSeconds = timerTicks = pomoTicks = 0; pomoCurrentSession = 1; lblTimer->setText("Status: Ready"); lblStatus->setText(""); lblPomoStatus->setText("Status: Ready"); SaveAllData(); updateUIStates(); }
    void updateUIStates() { btnStart->setEnabled(!isSessionActive); btnStop->setEnabled(isSessionActive); if(isSessionActive) lblStatus->setText("System Locked! Write password and click STOP or wait."); else lblStatus->setText(""); }

    void SyncProfileNameToFirebase(QString name) { QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncPasswordToFirebase(QString pass, bool isLocking) { QString dId = GetDeviceID(); QString val = isLocking ? pass : ""; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=livePassword&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ livePassword = @{ stringValue = '" + val + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncTogglesToFirebase() { QString dId = GetDeviceID(); QString bR = blockReels ? "$true" : "$false", bS = blockShorts ? "$true" : "$false", bA = isAdblockActive ? "$true" : "$false"; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }

    void SyncLiveTrackerToFirebase() {
        QString dId = GetDeviceID(); QString mode = "None"; QString timeL = "00:00"; QString activeStr = isSessionActive ? "$true" : "$false";
        if (isSessionActive) {
            if(isPomodoroMode) { mode = "Pomodoro"; int l = (pomoLengthMin*60) - pomoTicks; if(isPomodoroBreak) l = (2*60) - pomoTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0')); }
            else if(isTimeMode) { mode = "Timer"; int l = focusTimeTotalSeconds - timerTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/3600, 2, 10, QChar('0')).arg((l%3600)/60, 2, 10, QChar('0')); }
            else if(isPassMode) { mode = "Password"; timeL = "Locked"; }
        }
        QString usageStr = ""; for(auto i = usageStats.constBegin(); i != usageStats.constEnd(); ++i) { if(i.value() > 60) usageStr += QString("%1: %2m | ").arg(i.key()).arg(i.value()/60); }
        if(usageStr.isEmpty()) usageStr = "Clean";
        QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=isSelfControlActive&updateMask.fieldPaths=activeModeType&updateMask.fieldPaths=timeRemaining&updateMask.fieldPaths=appUsageSummary&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        runPowerShell("$body = @{ fields = @{ isSelfControlActive = @{ booleanValue = " + activeStr + " }; activeModeType = @{ stringValue = '" + mode + "' }; timeRemaining = @{ stringValue = '" + timeL + "' }; appUsageSummary = @{ stringValue = '" + usageStr + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
    }

    void ValidateLicenseAndTrial() {
        QString dId = GetDeviceID(); HINTERNET hInternet = InternetOpenA("RasFocus", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet) {
            DWORD to = 4000; InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &to, sizeof(to)); InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
            QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            HINTERNET hConnect = InternetOpenUrlA(hInternet, url.toStdString().c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hConnect) {
                char buffer[1024]; DWORD bytesRead; QString response = ""; while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; }
                InternetCloseHandle(hConnect);
                QString fbPackage = "7 Days Trial"; int pkgPos = response.indexOf("\"package\""); if (pkgPos != -1) { int valPos = response.indexOf("\"stringValue\": \"", pkgPos); if (valPos != -1) { valPos += 16; int endPos = response.indexOf("\"", valPos); if (endPos != -1) fbPackage = response.mid(valPos, endPos - valPos); } }
                QString trialFile = GetSecretDir() + "sys_lic.dat"; QFile in(trialFile); time_t activationTime = 0; QString savedPackage = "7 Days Trial";
                if (in.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream inStream(&in); inStream >> activationTime; savedPackage = inStream.readLine().trimmed(); in.close(); } 
                else { activationTime = time(0); savedPackage = fbPackage; QFile out(trialFile); if(out.open(QIODevice::WriteOnly|QIODevice::Text)){ QTextStream outStream(&out); outStream << activationTime << " " << savedPackage; out.close(); } }
                int totalDays = (savedPackage.contains("1 Year")) ? 365 : ((savedPackage.contains("6 Months")) ? 180 : 7); double daysPassed = difftime(time(0), activationTime) / 86400.0; trialDaysLeft = totalDays - (int)daysPassed;
                bool explicitlyRevoked = response.contains("\"stringValue\": \"REVOKED\""); bool explicitlyApproved = response.contains("\"stringValue\": \"APPROVED\"");
                if (explicitlyRevoked) { isLicenseValid = false; isTrialExpired = true; trialDaysLeft = 0; } else { if (trialDaysLeft <= 0) { isTrialExpired = true; trialDaysLeft = 0; isLicenseValid = false; } else { isTrialExpired = false; isLicenseValid = explicitlyApproved; } }
                
                auto parseBool = [&](QString fName, bool defaultVal) { int pos = response.indexOf("\"" + fName + "\""); if(pos != -1) { int vPos = response.indexOf("\"booleanValue\":", pos); if(vPos != -1) { if(response.indexOf("true", vPos) < response.indexOf("}", vPos)) return true; if(response.indexOf("false", vPos) < response.indexOf("}", vPos)) return false; } } return defaultVal; };
                blockAdult = parseBool("adultBlock", blockAdult);
                
                int msgPos = response.indexOf("\"adminMessage\""); if (msgPos != -1) { int valPos = response.indexOf("\"stringValue\": \"", msgPos); if (valPos != -1) { valPos += 16; int endPos = response.indexOf("\"", valPos); if(endPos != -1) safeAdminMsg = response.mid(valPos, endPos - valPos); } }
                int cmdPos = response.indexOf("\"adminCmd\""); if (cmdPos != -1) { int vPos = response.indexOf("\"stringValue\": \"", cmdPos); if (vPos != -1) { vPos += 16; int ePos = response.indexOf("\"", vPos); QString cmd = response.mid(vPos, ePos - vPos); if (cmd == "START_FOCUS" && !isSessionActive) { pendingAdminCmd = 1; } else if (cmd == "STOP_FOCUS" && isSessionActive) { pendingAdminCmd = 2; } } }
                int bcastPos = response.indexOf("\"broadcastMsg\""); if (bcastPos != -1) { int vPos = response.indexOf("\"stringValue\": \"", bcastPos); if (vPos != -1) { vPos += 16; int ePos = response.indexOf("\"", vPos); QString bMsg = response.mid(vPos, ePos - vPos); if (!bMsg.isEmpty() && bMsg != "ACK" && bMsg != currentBroadcastMsg) pendingBroadcastMsg = bMsg; } }
                int chatPos = response.indexOf("\"liveChatAdmin\""); if (chatPos != -1) { int cvPos = response.indexOf("\"stringValue\": \"", chatPos); if (cvPos != -1) { cvPos += 16; int cePos = response.indexOf("\"", cvPos); QString adminChatStr = response.mid(cvPos, cePos - cvPos); if (!adminChatStr.isEmpty() && adminChatStr != lastAdminChat) { lastAdminChat = adminChatStr; pendingAdminChatStr = adminChatStr; } } }
            } InternetCloseHandle(hInternet);
        } else { isLicenseValid = !isTrialExpired; }
    }

    void TrackUsage() {
        if(GetTickCount() - lastUsageUpdate < 1000) return; lastUsageUpdate = GetTickCount();
        HWND hActive = GetForegroundWindow(); if(!hActive || (overlayWidget && hActive == (HWND)overlayWidget->winId())) return;
        WCHAR title[512];
        if(GetWindowTextW(hActive, title, 512) > 0) {
            QString sTitle = QString::fromWCharArray(title).toLower();
            if(sTitle.contains("facebook")) usageStats["Facebook"]++; else if(sTitle.contains("youtube")) usageStats["YouTube"]++; else if(sTitle.contains("instagram")) usageStats["Instagram"]++;
            else { DWORD activePid; GetWindowThreadProcessId(hActive, &activePid); HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, activePid); if(ph){ WCHAR ex[MAX_PATH]; DWORD sz = MAX_PATH; if(QueryFullProcessImageNameW(ph, 0, ex, &sz)){ QString p = QString::fromWCharArray(ex); QString exe = p.mid(p.lastIndexOf('\\')+1); usageStats[exe]++; } CloseHandle(ph); } }
        }
    }

    void fastLoop() {
        if(!blockAdult && !blockReels && !blockShorts && !isSessionActive) return;
        if(isOverlayVisible) return;
        HWND hActive = GetForegroundWindow();
        if(hActive && (!overlayWidget || hActive != (HWND)overlayWidget->winId())) {
            WCHAR title[512];
            if(GetWindowTextW(hActive, title, 512) > 0) {
                QString sTitle = QString::fromWCharArray(title).toLower();
                if(blockAdult) { for(const QString& kw : explicitKeywords) { if(sTitle.contains(kw)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(1); return; } } }
                if(blockReels && sTitle.contains("facebook") && (sTitle.contains("reels") || sTitle.contains("video") || sTitle.contains("watch"))) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                if(blockShorts && sTitle.contains("youtube") && sTitle.contains("shorts")) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                if(isSessionActive) {
                    bool isBrowser = sTitle.contains("chrome") || sTitle.contains("edge") || sTitle.contains("firefox") || sTitle.contains("brave") || sTitle.contains("opera");
                    if(isBrowser) {
                        if(useAllowMode) {
                            bool ok = false; for(const QString& w : allowedWebs) { if(CheckMatch(w, sTitle)) { ok=true; break; } }
                            if(!ok && !sTitle.contains("allowed websites")) { CloseActiveTabAndMinimize(hActive); QString p = GetSecretDir() + "allowed.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><body style='text-align:center;'><h2>Focus Mode Active</h2></body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                        } else {
                            for(const QString& w : blockedWebs) { if(CheckMatch(w, sTitle)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; } }
                        }
                    }
                }
            }
        }
    }

    void slowLoop() {
        TrackUsage();
        if(isSessionActive) {
            if(isTrialExpired) { ClearSessionData(); QMessageBox::critical(this, "Expired", "License Expired. App Locked!"); return; }
            if(isPomodoroMode) {
                pomoTicks++; if(pomoTicks%5==0) SaveAllData();
                if(!isPomodoroBreak && pomoTicks >= pomoLengthMin*60) { isPomodoroBreak=true; pomoTicks=0; QString p = GetSecretDir() + "break.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><body><h1>Break!</h1></body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                else if(isPomodoroBreak && pomoTicks >= 2*60) { isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); QMessageBox::information(this, "Done", "Pomodoro Complete!"); } }
                int l = isPomodoroBreak ? (2*60)-pomoTicks : (pomoLengthMin*60)-pomoTicks; if(l<0) l=0;
                QString tt = QString("%1: %2:%3").arg(isPomodoroBreak?"Break":QString("Focus %1/%2").arg(pomoCurrentSession).arg(pomoTotalSessions)).arg(l/60,2,10,QChar('0')).arg(l%60,2,10,QChar('0'));
                lblTimer->setText(tt); lblPomoStatus->setText(tt); trayIcon->setToolTip(tt);
            }
            else if(isTimeMode) {
                timerTicks++; int left = focusTimeTotalSeconds - timerTicks;
                if(left <= 0) { ClearSessionData(); QMessageBox::information(this, "Done", "Focus Time Over!"); return; }
                QString tt = QString("Time Left: %1:%2:%3").arg(left/3600,2,10,QChar('0')).arg((left%3600)/60,2,10,QChar('0')).arg(left%60,2,10,QChar('0'));
                lblTimer->setText(tt); trayIcon->setToolTip(tt);
            } else { trayIcon->setToolTip("Focus Locked"); lblTimer->setText("Locked Manually"); }
            
            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)}; DWORD myPid = GetCurrentProcessId();
            if(Process32FirstW(h, &pe)) {
                do {
                    if(pe.th32ProcessID == myPid) continue; QString n = QString::fromWCharArray(pe.szExeFile).toLower();
                    if(n == "taskmgr.exe") { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } continue; }
                    if(useAllowMode) { if(!systemApps.contains(n) && !allowedApps.contains(n, Qt::CaseInsensitive) && !commonThirdPartyApps.contains(n)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } } 
                    else { if(blockedApps.contains(n, Qt::CaseInsensitive)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } }
                } while(Process32NextW(h, &pe));
            } CloseHandle(h);
        }
    }

    void syncLoop() {
        ValidateLicenseAndTrial(); SyncLiveTrackerToFirebase();
        if (isTrialExpired) { lblLicense->setText("EXPIRED"); lblLicense->setStyleSheet("color: red; font-weight: bold; margin-left: 20px;"); stack->setEnabled(false); if(!isSessionActive) { stack->setEnabled(true); sidebar->setCurrentRow(5); } }
        else if (isLicenseValid) { lblLicense->setText(QString("PREMIUM: %1 DAYS LEFT").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: #10B981; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); if(sidebar->count()>5) sidebar->item(5)->setHidden(true); }
        else { lblLicense->setText(QString("TRIAL: %1 DAYS LEFT").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: #F59E0B; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); }
        
        if(!safeAdminMsg.isEmpty()) lblAdminMsg->setText("Notice: " + safeAdminMsg); else lblAdminMsg->setText("");
        if(!pendingAdminChatStr.isEmpty()) { chatLog->append("<b>Admin:</b> " + pendingAdminChatStr); pendingAdminChatStr = ""; }
        if(!pendingBroadcastMsg.isEmpty() && pendingBroadcastMsg != "ACK") { currentBroadcastMsg = pendingBroadcastMsg; pendingBroadcastMsg = ""; QMessageBox::information(this, "Broadcast", currentBroadcastMsg); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=broadcastMsg&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ broadcastMsg = @{ stringValue = 'ACK' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        if(pendingAdminCmd == 1 && !isSessionActive) { pendingAdminCmd = 0; currentSessionPass = "12345"; isPassMode = true; isTimeMode = false; isPomodoroMode = false; isSessionActive = true; SaveAllData(); updateUIStates(); hide(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_START' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        else if(pendingAdminCmd == 2 && isSessionActive) { pendingAdminCmd = 0; ClearSessionData(); updateUIStates(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_STOP' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    }

    void closeEvent(QCloseEvent *event) override {
        if(isSessionActive) { event->ignore(); hide(); } else { event->accept(); }
    }
};

// --- Check and Request Admin Rights ---
bool IsRunAsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }
    return fIsRunAsAdmin;
}

void RequestAdminRights() {
    char szPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, szPath, ARRAYSIZE(szPath))) {
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas"; 
        sei.lpFile = szPath;
        sei.nShow = SW_NORMAL;
        if (!ShellExecuteExA(&sei)) {
            exit(1); 
        }
    }
}

int main(int argc, char *argv[]) {
    if (!IsRunAsAdmin()) {
        RequestAdminRights();
        return 0; 
    }

    QSharedMemory sharedMem("RasFocusPro_SharedMem_V2");
    if (!sharedMem.create(1)) {
        HWND hExisting = FindWindowA(NULL, "RasFocus Pro - Dashboard");
        if (hExisting) {
            PostMessageA(hExisting, WM_USER + 2, 0, 0); 
            SetForegroundWindow(hExisting);
        }
        return 0; 
    }
    
    SetupAutoStart(); 
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    app.setFont(QFont("Segoe UI", 14)); 
    
    RasFocusApp window;
    
    QString cmdStr = ""; for(int i=1; i<argc; i++) cmdStr += argv[i];
    if(cmdStr.contains("-autostart") || isSessionActive) window.hide(); else window.show();
    
    int ret = app.exec();
    UnhookWindowsHookEx(hKeyboardHook);
    return ret;
}
