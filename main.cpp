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
#include <QPixmap>
#include <QStyle>
#include <QMap>
#include <QElapsedTimer>
#include <QFrame>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsDropShadowEffect>
#include <QSizeGrip>

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

bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false, isOverlayVisible = false;
bool blockReels = false, blockShorts = false, isAdblockActive = false, blockAdult = false, isDarkMode = false;
bool isPomodoroMode = false, isPomodoroBreak = false, userClosedExpired = false;

int eyeBrightness = 100, eyeWarmth = 0, focusTimeTotalSeconds = 0, timerTicks = 0;
int pomoLengthMin = 25, pomoTotalSessions = 4, pomoCurrentSession = 1, pomoTicks = 0;

QString currentSessionPass = "", userProfileName = "", safeAdminMsg = "", currentDisplayQuote = "";
QString lastAdminChat = "", pendingAdminChatStr = "", currentBroadcastMsg = "", pendingBroadcastMsg = "";
bool isLicenseValid = false, isTrialExpired = false;
int trialDaysLeft = 7, pendingAdminCmd = 0;

QLabel *dimFilterWidget = nullptr;
QLabel *warmFilterWidget = nullptr;
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

void RequestAdminAndRestart(QString args) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = path;
    sei.lpParameters = args.toStdString().c_str();
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    if (!ShellExecuteExA(&sei)) { exit(0); }
    exit(0);
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
    QDir().mkpath(dir);
    SetFileAttributesA(dir.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "/";
}

bool CheckMatch(QString url, QString sTitle) { 
    QString t = sTitle.remove(' '); 
    QString exact = url.toLower().remove(' '); 
    if (!exact.isEmpty() && t.contains(exact)) return true; 
    QString s = url.toLower(); 
    s.replace('.', ' ').replace('/', ' ').replace(':', ' ').replace('-', ' '); 
    QStringList words = s.split(' ', Qt::SkipEmptyParts); 
    for(const QString& word : words) { 
        if (word == "https" || word == "http" || word == "www" || word == "com" || word == "org" || word == "net" || word == "html") continue; 
        if (word.length() >= 3 && t.contains(word)) return true; 
    } 
    return false; 
}

void CloseActiveTabAndMinimize(HWND hBrowser) {
    if (GetForegroundWindow() == hBrowser) {
        keybd_event(VK_CONTROL, 0, 0, 0); keybd_event('W', 0, 0, 0);
        keybd_event('W', 0, KEYEVENTF_KEYUP, 0); keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(150); 
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
        dimFilterWidget = new QLabel();
        dimFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
        dimFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        dimFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); 
        dimFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    if(!warmFilterWidget) {
        warmFilterWidget = new QLabel();
        warmFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
        warmFilterWidget->setAttribute(Qt::WA_TranslucentBackground);
        warmFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); 
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
// CUSTOM UI CLASSES (Buttons, Progress, Toasts)
// ==========================================

// Hover Animated Button
class SmoothButton : public QPushButton {
public:
    SmoothButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {}
protected:
    void enterEvent(QEvent* event) override {
        QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
        this->setGraphicsEffect(eff);
        QPropertyAnimation* a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(150); a->setStartValue(1.0); a->setEndValue(0.8);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::enterEvent(event);
    }
    void leaveEvent(QEvent* event) override {
        QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(this);
        this->setGraphicsEffect(eff);
        QPropertyAnimation* a = new QPropertyAnimation(eff, "opacity");
        a->setDuration(150); a->setStartValue(0.8); a->setEndValue(1.0);
        a->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::leaveEvent(event);
    }
};


class ToastNotification : public QWidget {
public:
    ToastNotification(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        QLabel* label = new QLabel(text, this);
        label->setStyleSheet("background-color: #10B981; color: white; padding: 12px 25px; border-radius: 8px; font-weight: bold; font-family: 'Segoe UI'; font-size: 14px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);");
        QVBoxLayout* layout = new QVBoxLayout(this); layout->addWidget(label); layout->setContentsMargins(0,0,0,0);
        adjustSize();
        if (parent) { QPoint center = parent->geometry().center(); move(center.x() - width() / 2, parent->geometry().bottom() - 100); }
        show();
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this); this->setGraphicsEffect(eff);
        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity"); a->setDuration(300); a->setStartValue(0); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped);
        QTimer::singleShot(2500, this, [=]() { QPropertyAnimation *a2 = new QPropertyAnimation(eff, "opacity"); a2->setDuration(300); a2->setStartValue(1); a2->setEndValue(0); connect(a2, &QPropertyAnimation::finished, this, &QWidget::deleteLater); a2->start(QPropertyAnimation::DeleteWhenStopped); });
    }
};

class CircularProgress : public QWidget {
public:
    int progress = 0; QString centerText = "Ready";
    CircularProgress(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(200, 200); }
    void updateProgress(int p, QString text) { progress = p; centerText = text; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        int size = qMin(width(), height()); QRect rect(10, 10, size - 20, size - 20);
        QPen bgPen(QColor("#E2E8F0"), 12); p.setPen(bgPen); p.drawArc(rect, 0, 360 * 16);
        QPen progPen(QColor("#3B82F6"), 12); progPen.setCapStyle(Qt::RoundCap); p.setPen(progPen);
        int spanAngle = -(progress * 360 * 16) / 100; p.drawArc(rect, 90 * 16, spanAngle);
        p.setPen(QColor(isDarkMode ? "#F8FAFC" : "#0F172A")); p.setFont(QFont("Segoe UI", 16, QFont::Bold)); p.drawText(rect, Qt::AlignCenter, centerText);
    }
};

class StopwatchWindow : public QWidget {
public:
    QLabel *lblSw; QWidget *controlPanel; QElapsedTimer timer; QTimer *updateTimer;
    bool isRunning = false; qint64 pausedTime = 0;

    StopwatchWindow() {
        setWindowTitle("Pro Stopwatch"); setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint); setAttribute(Qt::WA_TranslucentBackground); resize(400, 150);
        QFrame* bgFrame = new QFrame(this); bgFrame->setStyleSheet("background-color: rgba(15, 23, 42, 0.9); border-radius: 12px; border: 2px solid #3B82F6;"); bgFrame->setGeometry(0, 0, 400, 150);
        QVBoxLayout* l = new QVBoxLayout(bgFrame);
        lblSw = new QLabel("00:00:00.00"); lblSw->setAlignment(Qt::AlignCenter); lblSw->setStyleSheet("font-size: 45px; font-family: 'Consolas'; font-weight: bold; color: #38BDF8; background: transparent; border: none;"); l->addWidget(lblSw);
        controlPanel = new QWidget(); QHBoxLayout* h = new QHBoxLayout(controlPanel); h->setContentsMargins(0,0,0,0);
        SmoothButton* btnStart = new SmoothButton("Start / Pause"); SmoothButton* btnReset = new SmoothButton("Reset"); SmoothButton* btnClose = new SmoothButton("Close");
        QString bStyle = "background: #3B82F6; color: white; padding: 8px; font-weight: bold; border-radius: 6px; font-size: 13px;";
        btnStart->setStyleSheet(bStyle); btnReset->setStyleSheet(bStyle); btnClose->setStyleSheet("background: #EF4444; color: white; padding: 8px; font-weight: bold; border-radius: 6px; font-size: 13px;");
        h->addWidget(btnStart); h->addWidget(btnReset); h->addWidget(btnClose); l->addWidget(controlPanel);
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(controlPanel); eff->setOpacity(0.0); controlPanel->setGraphicsEffect(eff);
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, [=](){
            if(isRunning) { qint64 el = timer.elapsed() + pausedTime; int ms = (el % 1000) / 10; int s = (el / 1000) % 60; int m = (el / 60000) % 60; int hr = (el / 3600000);
                lblSw->setText(QString("%1:%2:%3.%4").arg(hr, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ms, 2, 10, QChar('0'))); }
        });
        connect(btnStart, &QPushButton::clicked, [=](){ if(isRunning) { isRunning = false; pausedTime += timer.elapsed(); updateTimer->stop(); } else { isRunning = true; timer.start(); updateTimer->start(30); } });
        connect(btnReset, &QPushButton::clicked, [=](){ isRunning = false; pausedTime = 0; updateTimer->stop(); lblSw->setText("00:00:00.00"); });
        connect(btnClose, &QPushButton::clicked, this, &QWidget::hide);
    }
    void enterEvent(QEvent *event) override { QPropertyAnimation *a = new QPropertyAnimation(controlPanel->graphicsEffect(), "opacity"); a->setDuration(200); a->setStartValue(0); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped); QWidget::enterEvent(event); }
    void leaveEvent(QEvent *event) override { QPropertyAnimation *a = new QPropertyAnimation(controlPanel->graphicsEffect(), "opacity"); a->setDuration(200); a->setStartValue(1); a->setEndValue(0); a->start(QPropertyAnimation::DeleteWhenStopped); QWidget::leaveEvent(event); }
    QPoint dragPos;
    void mousePressEvent(QMouseEvent *event) override { dragPos = event->globalPos() - frameGeometry().topLeft(); event->accept(); }
    void mouseMoveEvent(QMouseEvent *event) override { move(event->globalPos() - dragPos); event->accept(); }
};

StopwatchWindow* swWindow = nullptr;

// ==========================================
// MAIN GUI CLASS
// ==========================================
class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack; QListWidget* sidebar; QTimer *fastTimer, *slowTimer, *syncTimer; QSystemTrayIcon* trayIcon;
    QLineEdit *editName, *editPass; QSpinBox *spinHr, *spinMin; SmoothButton *btnStart, *btnStop; QLabel *lblStatus, *lblLicense, *lblAdminMsg; CircularProgress *dashProgress;
    QRadioButton *rbBlock, *rbAllow; QListWidget *listBlockApp, *listBlockWeb, *listAllowApp, *listAllowWeb, *listRunning; QLineEdit *inBlockApp, *inAllowApp; QComboBox *inBlockWeb, *inAllowWeb;
    QCheckBox *chkReels, *chkShorts, *chkAdblock; QSpinBox *pomoMin, *pomoSes; SmoothButton *bPStart, *bPStop; QLabel *lblPomoStatus; QSlider *sliderBright, *sliderWarm;
    QTextEdit *chatLog; QLineEdit *chatIn; QLineEdit *upgEmail, *upgPhone, *upgTrx; QComboBox *upgPkg;
    QPoint dragPosition; bool isDragging = false;

    QFrame* createCard() {
        QFrame* card = new QFrame();
        // Premium Card styling with Soft Drop Shadow simulation (via border/bg) and actual QGraphicsDropShadowEffect
        card->setStyleSheet(isDarkMode ? 
            "QFrame { background-color: #1E293B; border: 1px solid #334155; border-radius: 12px; }" : 
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 12px; }"
        );
        
        // Add Soft Drop Shadow
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(15);
        shadow->setXOffset(0);
        shadow->setYOffset(4);
        shadow->setColor(isDarkMode ? QColor(0, 0, 0, 100) : QColor(0, 0, 0, 20));
        card->setGraphicsEffect(shadow);
        
        return card;
    }

    void applyTheme() {
        QString bgMain = isDarkMode ? "#0F172A" : "#F8FAFC";
        QString bgCard = isDarkMode ? "#1E293B" : "#ffffff";
        QString textMain = isDarkMode ? "#F8FAFC" : "#1E293B";
        QString textBold = isDarkMode ? "#38BDF8" : "#0F172A";
        QString borderCol = isDarkMode ? "#334155" : "#E2E8F0";
        QString inputBg = isDarkMode ? "#0F172A" : "#F1F5F9";
        QString sidebarHover = isDarkMode ? "#1E293B" : "#F1F5F9";
        QString sidebarSel = isDarkMode ? "#334155" : "#E0F2FE";
        
        QString baseStyle = QString(R"(
            QMainWindow { background-color: %1; }
            QLabel { color: %3; font-size: 14px; font-family: 'Segoe UI', system-ui; }
            QLabel b { color: %4; font-weight: 700; }
            QLineEdit, QSpinBox, QComboBox, QTextEdit { 
                padding: 10px 12px; border: 1px solid %5; border-radius: 8px; 
                background: %6; color: %3; font-size: 14px; font-family: 'Segoe UI';
            }
            QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 2px solid #3B82F6; background: %2; }
            QLineEdit:disabled, QSpinBox:disabled { background: %5; color: #94A3B8; }
            QPushButton { font-family: 'Segoe UI'; font-size: 14px; border-radius: 8px; border: none; font-weight: 600; cursor: pointer; }
            QPushButton:disabled { background: #CBD5E1; color: #64748B; }
            
            QListWidget#sidebar { background-color: %2; color: #64748B; border-right: 1px solid %5; padding: 20px 10px; font-size: 15px; font-weight: bold; font-family: 'Segoe UI'; outline: none; }
            QListWidget#sidebar::item { height: 45px; padding-left: 15px; border-radius: 8px; margin-bottom: 5px; }
            QListWidget#sidebar::item:hover { background-color: %7; color: #0F172A; }
            QListWidget#sidebar::item:selected { background-color: %8; color: #0284C7; font-weight: 800; }
            
            QScrollBar:vertical { border: none; background: transparent; width: 8px; margin: 0px; }
            QScrollBar::handle:vertical { background: #CBD5E1; min-height: 20px; border-radius: 4px; }
            QScrollBar::handle:vertical:hover { background: #94A3B8; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
            
            /* Custom Tooltip */
            QToolTip { 
                background-color: #1E293B; color: #F8FAFC; border: 1px solid #334155; 
                border-radius: 6px; padding: 5px; font-family: 'Segoe UI'; font-size: 13px; 
            }
        )").arg(bgMain).arg(bgCard).arg(textMain).arg(textBold).arg(borderCol).arg(inputBg).arg(sidebarHover).arg(sidebarSel);
        
        setStyleSheet(baseStyle);
        stack->setStyleSheet(QString("QStackedWidget { background-color: %1; }").arg(bgMain));
    }

    RasFocusApp() {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
        resize(1150, 750); 
        
        QWidget* central = new QWidget(); setCentralWidget(central);
        QVBoxLayout* rootLayout = new QVBoxLayout(central); rootLayout->setContentsMargins(0, 0, 0, 0); rootLayout->setSpacing(0);
        
        // --- CUSTOM TITLE BAR ---
        QWidget* titleBar = new QWidget(); titleBar->setFixedHeight(45); titleBar->setObjectName("titleBar");
        QHBoxLayout* tbLayout = new QHBoxLayout(titleBar); tbLayout->setContentsMargins(20, 0, 15, 0);
        QLabel* appTitle = new QLabel("🚀 <b>RasFocus Pro</b>"); appTitle->setStyleSheet("font-size: 15px; border: none;"); tbLayout->addWidget(appTitle); tbLayout->addStretch();
        
        // Minimize Button
        QPushButton* btnMin = new QPushButton("—"); btnMin->setFixedSize(35, 30); 
        btnMin->setStyleSheet("QPushButton { background: transparent; color: #64748B; } QPushButton:hover { background: #E2E8F0; color: #0F172A; }");
        connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized); tbLayout->addWidget(btnMin);
        
        // Maximize/Restore Button
        QPushButton* btnMax = new QPushButton("⬜"); btnMax->setFixedSize(35, 30); 
        btnMax->setStyleSheet("QPushButton { background: transparent; color: #64748B; font-size: 14px; } QPushButton:hover { background: #E2E8F0; color: #0F172A; }");
        connect(btnMax, &QPushButton::clicked, this, [=]() {
            if (this->isMaximized()) { this->showNormal(); btnMax->setText("⬜"); } 
            else { this->showMaximized(); btnMax->setText("🗗"); }
        });
        tbLayout->addWidget(btnMax);
        
        // Close Button
        QPushButton* btnClose = new QPushButton("✕"); btnClose->setFixedSize(35, 30); 
        btnClose->setStyleSheet("QPushButton { background: transparent; color: #64748B; font-size: 16px; font-weight: bold; } QPushButton:hover { background: #EF4444; color: white; }");
        connect(btnClose, &QPushButton::clicked, this, &QWidget::hide); tbLayout->addWidget(btnClose);
        
        rootLayout->addWidget(titleBar);
        
        // MAIN CONTENT
        QWidget* mainContent = new QWidget(); QHBoxLayout* mainLayout = new QHBoxLayout(mainContent); mainLayout->setContentsMargins(0, 0, 0, 0); mainLayout->setSpacing(0);
        
        // Using Unicode text icons instead of emojis
        sidebar = new QListWidget(); sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(260);
        sidebar->addItem("  🗠  Overview"); sidebar->addItem("  ⛨  Blocks & Allows"); sidebar->addItem("  🗓  Schedule"); sidebar->addItem("  ⚙  Advanced Features");
        sidebar->addItem("  ⏱  Tools & Pomodoro"); sidebar->addItem("  🎴  Settings & Theme"); sidebar->addItem("  💬  Live Chat"); sidebar->addItem("  ★  Premium Upgrade");
        
        stack = new QStackedWidget();
        setupOverviewPage(); setupListsPage(); setupSchedulePage(); setupAdvancedPage(); setupToolsPage(); setupSettingsPage(); setupChatPage(); setupUpgradePage();
        
        mainLayout->addWidget(sidebar); mainLayout->addWidget(stack); rootLayout->addWidget(mainContent);
        
        applyTheme(); 
        connect(sidebar, &QListWidget::currentRowChanged, [=](int idx){
            QWidget* nextPage = stack->widget(idx); QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(nextPage); nextPage->setGraphicsEffect(eff);
            stack->setCurrentIndex(idx); QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity"); a->setDuration(250); a->setStartValue(0); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped);
        });
        sidebar->setCurrentRow(0);
        
        // --- SIZE GRIP (For frameless resizing) ---
        QSizeGrip *sizeGrip = new QSizeGrip(this);
        sizeGrip->setStyleSheet("width: 15px; height: 15px; background: transparent;");
        rootLayout->addWidget(sizeGrip, 0, Qt::AlignBottom | Qt::AlignRight);

        setupTray(); LoadAllData(); ApplyEyeFilters();
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }
    
    void mousePressEvent(QMouseEvent *event) override { if (event->button() == Qt::LeftButton && event->pos().y() <= 45) { isDragging = true; dragPosition = event->globalPos() - frameGeometry().topLeft(); event->accept(); } }
    void mouseMoveEvent(QMouseEvent *event) override { if (isDragging && event->buttons() & Qt::LeftButton) { move(event->globalPos() - dragPosition); event->accept(); } }
    void mouseReleaseEvent(QMouseEvent *event) override { isDragging = false; event->accept(); }

private:
    void setupOverviewPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 40, 50, 40); l->setSpacing(25);
        
        QLabel* title = new QLabel("Dashboard Overview"); title->setStyleSheet("font-size: 28px; font-weight: bold; color: #0F172A;"); l->addWidget(title);
        
        QFrame* profileCard = createCard(); QHBoxLayout* h1 = new QHBoxLayout(profileCard); h1->setContentsMargins(30, 20, 30, 20);
        h1->addWidget(new QLabel("<b>Profile Name:</b>")); editName = new QLineEdit(); editName->setPlaceholderText("Enter your name"); editName->setFixedWidth(250); h1->addWidget(editName);
        SmoothButton* btnSave = new SmoothButton("Save"); btnSave->setStyleSheet("background: #F1F5F9; color: #0F172A; padding: 10px 20px; border: 1px solid #CBD5E1;"); h1->addWidget(btnSave);
        lblLicense = new QLabel("Checking..."); lblLicense->setStyleSheet("font-weight: bold; font-size: 14px; margin-left: 20px;"); h1->addWidget(lblLicense); h1->addStretch();
        l->addWidget(profileCard);
        connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); SaveAllData(); SyncProfileNameToFirebase(userProfileName); new ToastNotification("✅ Profile Saved!", this); });
        
        QFrame* focusCard = createCard(); QVBoxLayout* fcLayout = new QVBoxLayout(focusCard); fcLayout->setContentsMargins(30, 30, 30, 30); fcLayout->setSpacing(25);
        QLabel* st = new QLabel("Start a Focus Session"); st->setStyleSheet("font-size: 18px; font-weight: bold;"); fcLayout->addWidget(st);
        
        QGridLayout* g = new QGridLayout(); g->setSpacing(20);
        g->addWidget(new QLabel("<b>Lock via Password:</b>"), 0, 0); 
        editPass = new QLineEdit(); editPass->setEchoMode(QLineEdit::Password); editPass->setPlaceholderText("Friend sets this"); 
        // Note: Styling based on lock state is handled in updateUIStates()
        g->addWidget(editPass, 1, 0);
        
        g->addWidget(new QLabel("<b>Lock via Timer:</b>"), 0, 1); 
        QHBoxLayout* th = new QHBoxLayout(); spinHr = new QSpinBox(); spinHr->setSuffix(" Hr"); spinHr->setRange(0, 24); spinMin = new QSpinBox(); spinMin->setSuffix(" Min"); spinMin->setMaximum(59); th->addWidget(spinHr); th->addWidget(spinMin); g->addLayout(th, 1, 1);
        fcLayout->addLayout(g);
        
        QHBoxLayout* h2 = new QHBoxLayout();
        btnStart = new SmoothButton("▶ START FOCUS"); btnStart->setStyleSheet("background-color: #10B981; color: white; padding: 15px 35px; font-size: 15px;");
        btnStop = new SmoothButton("⏹ STOP FOCUS"); btnStop->setStyleSheet("background-color: #EF4444; color: white; padding: 15px 35px; font-size: 15px;");
        h2->addWidget(btnStart); h2->addWidget(btnStop); h2->addStretch(); fcLayout->addLayout(h2);
        l->addWidget(focusCard);
        
        dashProgress = new CircularProgress(this); l->addWidget(dashProgress, 0, Qt::AlignCenter);
        lblStatus = new QLabel(""); lblStatus->setAlignment(Qt::AlignCenter); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold;"); l->addWidget(lblStatus);
        lblAdminMsg = new QLabel(""); lblAdminMsg->setAlignment(Qt::AlignCenter); lblAdminMsg->setStyleSheet("color: #8B5CF6; font-weight: bold;"); l->addWidget(lblAdminMsg);

        l->addStretch(); stack->addWidget(page);
        connect(btnStart, &QPushButton::clicked, this, &RasFocusApp::onStartFocus); connect(btnStop, &QPushButton::clicked, this, &RasFocusApp::onStopFocus);
    }

    void updateEmptyState(QListWidget* lw, const QString& msg) {
        if(lw->count() == 0) {
            QListWidgetItem* item = new QListWidgetItem(lw);
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled); // unselectable
            QLabel* emptyLbl = new QLabel(msg);
            emptyLbl->setStyleSheet("color: #94A3B8; font-style: italic; padding: 10px;");
            emptyLbl->setAlignment(Qt::AlignCenter);
            lw->setItemWidget(item, emptyLbl);
        } else {
            // Remove empty state item if it exists
            if(lw->count() > 0 && lw->item(0)->flags() == (lw->item(0)->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled)) {
                delete lw->takeItem(0);
            }
        }
    }

    void addInlineItem(QListWidget* listWidget, const QString& text, const QString& emptyMsg) {
        if (text.isEmpty()) return;
        
        // Remove empty state if present
        if(listWidget->count() > 0 && listWidget->item(0)->flags() == (listWidget->item(0)->flags() & ~Qt::ItemIsSelectable & ~Qt::ItemIsEnabled)) {
            delete listWidget->takeItem(0);
        }

        for (int i = 0; i < listWidget->count(); ++i) { QWidget* widget = listWidget->itemWidget(listWidget->item(i)); if (widget) { QLabel* label = widget->findChild<QLabel*>(); if (label && label->text() == text) return; } }
        
        QListWidgetItem* item = new QListWidgetItem(listWidget); item->setSizeHint(QSize(0, 40)); 
        QWidget* customWidget = new QWidget(); QHBoxLayout* layout = new QHBoxLayout(customWidget); layout->setContentsMargins(15, 0, 10, 0);
        QLabel* label = new QLabel(text); label->setStyleSheet("font-size: 14px; font-weight: 500; color: #334155;");
        SmoothButton* btnRemove = new SmoothButton("✖"); btnRemove->setFixedSize(24, 24); btnRemove->setStyleSheet("QPushButton { background: #F1F5F9; color: #94A3B8; border-radius: 12px; font-size: 10px; } QPushButton:hover { background: #FEE2E2; color: #EF4444; }");
        layout->addWidget(label); layout->addStretch(); layout->addWidget(btnRemove); listWidget->setItemWidget(item, customWidget);
        
        connect(btnRemove, &QPushButton::clicked, [=]() { 
            int row = listWidget->row(item); delete listWidget->takeItem(row); 
            updateEmptyState(listWidget, emptyMsg);
            SyncListsFromUI(); 
        });
    }

    void setupListsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40); l->setSpacing(20);
        QLabel* pageTitle = new QLabel("App & Website Filtering"); pageTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: #0F172A; margin-bottom: 10px;"); l->addWidget(pageTitle);
        
        QFrame* modeCard = createCard(); QHBoxLayout* radL = new QHBoxLayout(modeCard); radL->setContentsMargins(25, 20, 25, 20);
        rbBlock = new QRadioButton("<b>Block List Mode</b> (Blocks specific)"); rbAllow = new QRadioButton("<b>Allow List Mode</b> (Blocks EVERYTHING except allowed)");
        rbBlock->setStyleSheet("font-size: 15px;"); rbAllow->setStyleSheet("font-size: 15px;"); rbBlock->setChecked(true); radL->addWidget(rbBlock); radL->addWidget(rbAllow); l->addWidget(modeCard);
        connect(rbBlock, &QRadioButton::toggled, [=](){ useAllowMode = rbAllow->isChecked(); SaveAllData(); });
        
        QFrame* listCard = createCard(); QGridLayout* g = new QGridLayout(listCard); g->setContentsMargins(30, 30, 30, 30); g->setSpacing(30);
        QString btnStyle = "background-color: #3B82F6; color: white; padding: 10px 15px;";
        QString listStyle = "QListWidget { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px; padding: 5px; } QListWidget::item { background: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; margin-bottom: 5px; } QListWidget::item:hover { border-color: #CBD5E1; }";

        auto makeList = [&](QString title, QLineEdit*& inA, QComboBox*& inW, QListWidget*& lA, QListWidget*& lW, int col) {
            QLabel* lblApp = new QLabel("<b>" + title + " Apps (.exe):</b>"); lblApp->setStyleSheet("font-size: 16px; margin-bottom: 5px;"); g->addWidget(lblApp, 0, col);
            QHBoxLayout* h = new QHBoxLayout(); inA = new QLineEdit(); inA->setPlaceholderText("E.g., chrome.exe"); SmoothButton* bA = new SmoothButton("Add"); bA->setStyleSheet(btnStyle); h->addWidget(inA); h->addWidget(bA); g->addLayout(h, 1, col);
            lA = new QListWidget(); lA->setStyleSheet(listStyle); lA->setSelectionMode(QAbstractItemView::NoSelection); g->addWidget(lA, 2, col);
            updateEmptyState(lA, "No apps added yet.");

            QLabel* lblWeb = new QLabel("<b>" + title + " Websites:</b>"); lblWeb->setStyleSheet("font-size: 16px; margin-top: 15px; margin-bottom: 5px;"); g->addWidget(lblWeb, 3, col);
            QHBoxLayout* h2 = new QHBoxLayout(); inW = new QComboBox(); inW->setEditable(true); SmoothButton* bW = new SmoothButton("Add"); bW->setStyleSheet(btnStyle); h2->addWidget(inW); h2->addWidget(bW); g->addLayout(h2, 4, col);
            lW = new QListWidget(); lW->setStyleSheet(listStyle); lW->setSelectionMode(QAbstractItemView::NoSelection); g->addWidget(lW, 5, col);
            updateEmptyState(lW, "No websites added yet.");
            
            connect(bA, &QPushButton::clicked, [=](){ QString t = inA->text().trimmed().toLower(); if(!t.isEmpty()){ if(!t.endsWith(".exe")) t += ".exe"; addInlineItem(lA, t, "No apps added yet."); inA->clear(); SyncListsFromUI(); } });
            connect(bW, &QPushButton::clicked, [=](){ QString t = inW->currentText().trimmed().toLower(); if(!t.isEmpty()){ addInlineItem(lW, t, "No websites added yet."); inW->setCurrentText(""); SyncListsFromUI(); } });
        };
        
        makeList("🚫 Block", inBlockApp, inBlockWeb, listBlockApp, listBlockWeb, 0);
        
        QVBoxLayout* midLay = new QVBoxLayout();
        QLabel* lblRun = new QLabel("<b>🏃 Running Apps:</b>"); lblRun->setStyleSheet("font-size: 16px; margin-bottom: 5px;"); midLay->addWidget(lblRun);
        listRunning = new QListWidget(); listRunning->setStyleSheet("QListWidget { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px; padding: 5px; } QListWidget::item { padding: 10px; border-bottom: 1px solid #F1F5F9; } QListWidget::item:selected { background: #EFF6FF; color: #3B82F6; font-weight: bold; border-radius: 6px; }"); midLay->addWidget(listRunning);
        SmoothButton* bRun = new SmoothButton("Add Selected to Active List"); bRun->setStyleSheet("background-color: #10B981; color: white; padding: 12px; margin-top: 10px;"); midLay->addWidget(bRun);
        g->addLayout(midLay, 0, 1, 6, 1);
        
        connect(bRun, &QPushButton::clicked, [=](){ if(!listRunning->currentItem()) return; QString app = listRunning->currentItem()->text().trimmed().toLower(); if(!app.endsWith(".exe")) app += ".exe"; if(useAllowMode) { addInlineItem(listAllowApp, app, "No apps added yet."); } else { addInlineItem(listBlockApp, app, "No apps added yet."); } SyncListsFromUI(); });
        
        makeList("✅ Allow", inAllowApp, inAllowWeb, listAllowApp, listAllowWeb, 2);
        l->addWidget(listCard);
        
        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com", "reddit.com", "netflix.com"};
        inBlockWeb->addItems(popSites); inAllowWeb->addItems(popSites); inBlockWeb->setCurrentText(""); inAllowWeb->setCurrentText("");
        stack->addWidget(page);
    }

    void setupSchedulePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QFrame* card = createCard(); QVBoxLayout* cl = new QVBoxLayout(card); cl->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("📅 Schedule (Coming Soon)"); title->setStyleSheet("font-size: 26px; font-weight: bold; color: #3B82F6;"); cl->addWidget(title, 0, Qt::AlignCenter);
        QLabel* sub = new QLabel("Plan your daily focus routines automatically in future updates."); sub->setAlignment(Qt::AlignCenter); cl->addWidget(sub);
        l->addWidget(card); l->addStretch(); stack->addWidget(page);
    }

    void setupAdvancedPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 40, 50, 40); l->setSpacing(25);
        QLabel* title = new QLabel("Advanced Restrictions"); title->setStyleSheet("font-size: 28px; font-weight: bold;"); l->addWidget(title);
        
        QFrame* advCard = createCard(); QVBoxLayout* advLayout = new QVBoxLayout(advCard); advLayout->setContentsMargins(40, 40, 40, 40); advLayout->setSpacing(20);
        QString tStyle = "QCheckBox { font-size: 16px; padding: 15px 20px; border-radius: 8px; font-weight: 600; background: #F8FAFC; border: 1px solid #E2E8F0; } QCheckBox::indicator { width: 45px; height: 24px; border-radius: 12px; } QCheckBox::indicator:unchecked { background-color: #CBD5E1; } QCheckBox::indicator:checked { background-color: #10B981; }";
        
        chkReels = new QCheckBox("   Block Facebook Reels & Watch (Strict Mode)"); chkReels->setStyleSheet(tStyle);
        chkShorts = new QCheckBox("   Block YouTube Shorts"); chkShorts->setStyleSheet(tStyle);
        chkAdblock = new QCheckBox("   System-wide AD BLOCKER (Silent Install)"); chkAdblock->setStyleSheet(tStyle);
        
        advLayout->addWidget(chkReels); advLayout->addWidget(chkShorts); advLayout->addWidget(chkAdblock); l->addWidget(advCard);
        
        connect(chkReels, &QCheckBox::clicked, [=](bool c){ blockReels = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkShorts, &QCheckBox::clicked, [=](bool c){ blockShorts = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkAdblock, &QCheckBox::clicked, [=](bool c){ isAdblockActive = c; ToggleAdBlock(c); SaveAllData(); SyncTogglesToFirebase(); });
        
        l->addStretch(); stack->addWidget(page);
    }

    void setupToolsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 30, 40, 30); l->setSpacing(20);
        QLabel* title = new QLabel("Productivity Tools"); title->setStyleSheet("font-size: 28px; font-weight: bold; margin-bottom: 5px;"); l->addWidget(title);
        
        // ROW 1: POMODORO & STOPWATCH
        QHBoxLayout* row1 = new QHBoxLayout(); row1->setSpacing(20);
        
        QFrame* pomoCard = createCard(); QVBoxLayout* pl = new QVBoxLayout(pomoCard); pl->setContentsMargins(30, 30, 30, 30);
        QLabel* pt = new QLabel("🍅 Pomodoro Timer"); pt->setStyleSheet("font-size: 20px; font-weight: bold; color: #EF4444; margin-bottom: 15px;"); pl->addWidget(pt);
        QHBoxLayout* ph1 = new QHBoxLayout(); ph1->addWidget(new QLabel("<b>Focus (Min):</b>")); pomoMin = new QSpinBox(); pomoMin->setValue(25); ph1->addWidget(pomoMin); ph1->addStretch(); pl->addLayout(ph1);
        QHBoxLayout* ph2 = new QHBoxLayout(); ph2->addWidget(new QLabel("<b>Sessions:</b>")); pomoSes = new QSpinBox(); pomoSes->setValue(4); ph2->addWidget(pomoSes); ph2->addStretch(); pl->addLayout(ph2);
        QHBoxLayout* pBtnH = new QHBoxLayout(); pBtnH->setContentsMargins(0, 15, 0, 0);
        bPStart = new SmoothButton("Start"); bPStart->setStyleSheet("background: #10B981; color: white; padding: 12px;");
        bPStop = new SmoothButton("Stop"); bPStop->setStyleSheet("background: #EF4444; color: white; padding: 12px;");
        pBtnH->addWidget(bPStart); pBtnH->addWidget(bPStop); pl->addLayout(pBtnH);
        lblPomoStatus = new QLabel("Status: Ready"); lblPomoStatus->setStyleSheet("color: #64748B; font-weight: bold; margin-top: 10px;"); pl->addWidget(lblPomoStatus);
        row1->addWidget(pomoCard);
        
        QFrame* swCard = createCard(); QVBoxLayout* sl = new QVBoxLayout(swCard); sl->setContentsMargins(30, 30, 30, 30);
        QLabel* st = new QLabel("⏱️ Pro Stopwatch"); st->setStyleSheet("font-size: 20px; font-weight: bold; color: #3B82F6; margin-bottom: 15px;"); sl->addWidget(st);
        QLabel* sDesc = new QLabel("A floating, always-on-top stopwatch\nfor precise time tracking."); sDesc->setStyleSheet("color: #64748B;"); sl->addWidget(sDesc); sl->addStretch();
        SmoothButton* bOpenSw = new SmoothButton("Open Widget"); bOpenSw->setStyleSheet("background: #3B82F6; color: white; padding: 12px;"); sl->addWidget(bOpenSw);
        row1->addWidget(swCard);
        l->addLayout(row1);
        
        // ROW 2: EYE CURE CARD 
        QFrame* eyeCard = createCard(); QVBoxLayout* el = new QVBoxLayout(eyeCard); el->setContentsMargins(30, 30, 30, 30); el->setSpacing(20);
        QLabel* et = new QLabel("👁️ Eye Cure Filters"); et->setStyleSheet("font-size: 20px; font-weight: bold; color: #8B5CF6;"); el->addWidget(et);
        
        QString sStyle = "QSlider::groove:horizontal { background: #E2E8F0; height: 6px; border-radius: 3px; } QSlider::sub-page:horizontal { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FDE68A, stop:1 #F59E0B); border-radius: 3px; } QSlider::handle:horizontal { background: #FFFFFF; border: 3px solid #F59E0B; width: 16px; height: 16px; margin: -6px 0; border-radius: 11px; }";
        QString dStyle = "QSlider::groove:horizontal { background: #E2E8F0; height: 6px; border-radius: 3px; } QSlider::sub-page:horizontal { background: #38BDF8; border-radius: 3px; } QSlider::handle:horizontal { background: #FFFFFF; border: 3px solid #38BDF8; width: 16px; height: 16px; margin: -6px 0; border-radius: 11px; }";

        QHBoxLayout* sRow = new QHBoxLayout();
        // Warmth
        QVBoxLayout* warmLay = new QVBoxLayout();
        QLabel* lblWarmVal = new QLabel("3650K"); lblWarmVal->setAlignment(Qt::AlignCenter); lblWarmVal->setStyleSheet("background: #F59E0B; color: white; border-radius: 4px; padding: 3px 8px; font-weight: bold; font-size: 12px;"); warmLay->addWidget(lblWarmVal, 0, Qt::AlignCenter);
        sliderWarm = new QSlider(Qt::Horizontal); sliderWarm->setRange(0, 100); sliderWarm->setValue(0); sliderWarm->setStyleSheet(sStyle); warmLay->addWidget(sliderWarm);
        QHBoxLayout* wt = new QHBoxLayout(); wt->addWidget(new QLabel("Warm")); wt->addStretch(); wt->addWidget(new QLabel("Cool")); warmLay->addLayout(wt); sRow->addLayout(warmLay);
        sRow->addSpacing(30);
        // Brightness
        QVBoxLayout* brightLay = new QVBoxLayout();
        QLabel* lblBrightVal = new QLabel("100%"); lblBrightVal->setAlignment(Qt::AlignCenter); lblBrightVal->setStyleSheet("background: #38BDF8; color: white; border-radius: 4px; padding: 3px 8px; font-weight: bold; font-size: 12px;"); brightLay->addWidget(lblBrightVal, 0, Qt::AlignLeft);
        sliderBright = new QSlider(Qt::Horizontal); sliderBright->setRange(10, 100); sliderBright->setValue(100); sliderBright->setStyleSheet(dStyle); brightLay->addWidget(sliderBright);
        QHBoxLayout* bt = new QHBoxLayout(); bt->addWidget(new QLabel("Dim")); bt->addStretch(); bt->addWidget(new QLabel("Bright")); brightLay->addLayout(bt); sRow->addLayout(brightLay);
        el->addLayout(sRow);

        // PRESETS
        QGridLayout* pg = new QGridLayout(); pg->setSpacing(12);
        QString bN = "QPushButton { background: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; padding: 10px; } QPushButton:hover { background: #E2E8F0; }";
        QString bA = "QPushButton { background: #0EA5E9; color: white; border: none; padding: 10px; }";
        
        SmoothButton* btnPause = new SmoothButton("Pause"); btnPause->setStyleSheet(bN); SmoothButton* btnHealth = new SmoothButton("Health"); btnHealth->setStyleSheet(bA);
        SmoothButton* btnGame = new SmoothButton("Game"); btnGame->setStyleSheet(bN); SmoothButton* btnMovie = new SmoothButton("Movie"); btnMovie->setStyleSheet(bN);
        SmoothButton* btnOffice = new SmoothButton("Office"); btnOffice->setStyleSheet(bN); SmoothButton* btnReading = new SmoothButton("Reading"); btnReading->setStyleSheet(bN);
        
        pg->addWidget(btnPause, 0, 0); pg->addWidget(btnHealth, 0, 1); pg->addWidget(btnGame, 0, 2); 
        pg->addWidget(btnMovie, 1, 0); pg->addWidget(btnOffice, 1, 1); pg->addWidget(btnReading, 1, 2);
        el->addLayout(pg);
        l->addWidget(eyeCard);

        // LOGIC
        connect(bPStart, &QPushButton::clicked, [=](){ if(!isSessionActive && !isTrialExpired) { pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value(); isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1; SaveAllData(); updateUIStates(); new ToastNotification("🍅 Pomodoro Started!", this); } });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { ClearSessionData(); updateUIStates(); new ToastNotification("🛑 Stopped!", this); } });
        connect(bOpenSw, &QPushButton::clicked, [=](){ if(!swWindow) swWindow = new StopwatchWindow(); swWindow->showNormal(); swWindow->activateWindow(); });
        
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; lblBrightVal->setText(QString::number(v) + "%"); ApplyEyeFilters(); SaveAllData(); });
        connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; int k = 6500 - (v * 30); lblWarmVal->setText(QString::number(k) + "K"); ApplyEyeFilters(); SaveAllData(); });

        auto rB = [=]() { btnPause->setStyleSheet(bN); btnHealth->setStyleSheet(bN); btnGame->setStyleSheet(bN); btnMovie->setStyleSheet(bN); btnOffice->setStyleSheet(bN); btnReading->setStyleSheet(bN); };
        connect(btnOffice, &QPushButton::clicked, [=](){ rB(); btnOffice->setStyleSheet(bA); sliderBright->setValue(100); sliderWarm->setValue(0); });
        connect(btnReading, &QPushButton::clicked, [=](){ rB(); btnReading->setStyleSheet(bA); sliderBright->setValue(85); sliderWarm->setValue(30); });
        connect(btnHealth, &QPushButton::clicked, [=](){ rB(); btnHealth->setStyleSheet(bA); sliderBright->setValue(60); sliderWarm->setValue(75); });
        connect(btnGame, &QPushButton::clicked, [=](){ rB(); btnGame->setStyleSheet(bA); sliderBright->setValue(90); sliderWarm->setValue(10); });
        connect(btnMovie, &QPushButton::clicked, [=](){ rB(); btnMovie->setStyleSheet(bA); sliderBright->setValue(70); sliderWarm->setValue(40); });

        l->addStretch(); stack->addWidget(page);
    }

    void setupSettingsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QLabel* title = new QLabel("Settings & Personalization"); title->setStyleSheet("font-size: 28px; font-weight: bold; margin-bottom: 20px;"); l->addWidget(title);
        
        QFrame* card = createCard(); QVBoxLayout* cl = new QVBoxLayout(card); cl->setContentsMargins(40, 40, 40, 40);
        cl->addWidget(new QLabel("<b>App Theme:</b>"));
        QCheckBox* chkDark = new QCheckBox(" Enable Dark Mode UI"); chkDark->setStyleSheet("font-size: 16px; padding: 15px; margin-top: 10px; background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px;");
        chkDark->setChecked(isDarkMode); connect(chkDark, &QCheckBox::clicked, [=](bool c){ isDarkMode = c; applyTheme(); });
        cl->addWidget(chkDark); l->addWidget(card); l->addStretch(); stack->addWidget(page);
    }

    void setupChatPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 30, 40, 30);
        QLabel* title = new QLabel("💬 Live Support Chat"); title->setStyleSheet("font-size: 28px; font-weight: bold; color: #EC4899; margin-bottom: 15px;"); l->addWidget(title);
        
        QFrame* card = createCard(); QVBoxLayout* cl = new QVBoxLayout(card); cl->setContentsMargins(25, 25, 25, 25); cl->setSpacing(15);
        chatLog = new QTextEdit(); chatLog->setReadOnly(true); chatLog->setStyleSheet("border: 1px solid #E2E8F0; border-radius: 8px; background: #F8FAFC; padding: 15px; font-size: 15px;"); cl->addWidget(chatLog);
        
        QHBoxLayout* ch = new QHBoxLayout();
        chatIn = new QLineEdit(); chatIn->setPlaceholderText("Type your message to Admin..."); ch->addWidget(chatIn);
        SmoothButton* bSend = new SmoothButton("Send Message"); bSend->setStyleSheet("background: #EC4899; color: white; padding: 12px 25px;"); ch->addWidget(bSend);
        cl->addLayout(ch); l->addWidget(card);
        
        connect(bSend, &QPushButton::clicked, [=](){
            QString msg = chatIn->text().trimmed(); if(msg.isEmpty()) return;
            chatLog->append("<span style='color:#3B82F6;'><b>You:</b></span> " + msg); chatIn->clear();
            QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            runPowerShell("$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + msg + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
        });
        stack->addWidget(page);
    }
    
    void setupUpgradePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 40, 50, 40);
        QLabel* title = new QLabel("★ Premium Upgrade"); title->setStyleSheet("font-size: 30px; font-weight: bold; color: #F59E0B;"); l->addWidget(title);
        QLabel* sub = new QLabel("Unlock all features. Send payment via Nagad/bKash to <b>01566054963</b>"); sub->setStyleSheet("font-size: 16px; margin-bottom: 20px;"); l->addWidget(sub);
        
        QFrame* card = createCard(); QVBoxLayout* fl = new QVBoxLayout(card); fl->setContentsMargins(40, 40, 40, 40); fl->setSpacing(20);
        fl->addWidget(new QLabel("<b>Email / Name:</b>")); upgEmail = new QLineEdit(); fl->addWidget(upgEmail);
        fl->addWidget(new QLabel("<b>Sender Number:</b>")); upgPhone = new QLineEdit(); fl->addWidget(upgPhone);
        fl->addWidget(new QLabel("<b>Transaction ID (TrxID):</b>")); upgTrx = new QLineEdit(); fl->addWidget(upgTrx);
        fl->addWidget(new QLabel("<b>Select Package:</b>")); upgPkg = new QComboBox(); upgPkg->addItems({"7 Days Trial", "6 Months (50 BDT)", "1 Year (100 BDT)"}); fl->addWidget(upgPkg);
        
        SmoothButton* bSub = new SmoothButton("SUBMIT REQUEST"); bSub->setStyleSheet("background: #10B981; color: white; padding: 15px; margin-top: 15px; font-size: 16px;"); fl->addWidget(bSub);
        l->addWidget(card); l->addStretch(); stack->addWidget(page);
        
        connect(bSub, &QPushButton::clicked, [=](){
            if(upgEmail->text().isEmpty() || upgPhone->text().isEmpty() || upgTrx->text().isEmpty()) { new ToastNotification("⚠️ Fill all fields!", this); return; }
            QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            runPowerShell("$body = @{ fields = @{ deviceID = @{ stringValue = '" + dId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + upgPkg->currentText() + "' }; userEmail = @{ stringValue = '" + upgEmail->text() + "' }; senderPhone = @{ stringValue = '" + upgPhone->text() + "' }; trxId = @{ stringValue = '" + upgTrx->text() + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
            new ToastNotification("✅ Sent to Admin!", this);
        });
    }

    void setupTray() {
        QString iconPath = QCoreApplication::applicationDirPath() + "/icon.ico"; QIcon icon;
        if(QFile::exists(iconPath)) { icon = QIcon(iconPath); } else {
            QPixmap pix(32, 32); pix.fill(Qt::transparent); QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing); p.setBrush(QColor("#3B82F6")); p.drawEllipse(2, 2, 28, 28); p.setPen(Qt::white); p.setFont(QFont("Segoe UI", 12, QFont::Bold)); p.drawText(pix.rect(), Qt::AlignCenter, "RF"); icon = QIcon(pix);
        }
        trayIcon = new QSystemTrayIcon(icon, this);
        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason r){ if(r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) { showNormal(); raise(); activateWindow(); } });
        trayIcon->show();
    }

    // ==========================================
    // LOGIC & SYNC FUNCTIONS
    // ==========================================
    void onStartFocus() {
        if(isSessionActive) return;
        QString p = editPass->text(); int tSec = (spinHr->value() * 3600) + (spinMin->value() * 60);
        if(p.isEmpty() && tSec == 0) { new ToastNotification("⚠️ Set Password or Time!", this); return; }
        useAllowMode = rbAllow->isChecked();
        if(!p.isEmpty()) { isPassMode = true; currentSessionPass = p; SyncPasswordToFirebase(p, true); } else { isTimeMode = true; focusTimeTotalSeconds = tSec; timerTicks = 0; }
        isSessionActive = true; editPass->clear(); SaveAllData(); updateUIStates();
        new ToastNotification("🔒 Focus Mode Active. Hiding to tray.", this); QTimer::singleShot(1500, this, &QWidget::hide);
    }
    
    void onStopFocus() {
        if(!isSessionActive) return;
        if(isTimeMode) { new ToastNotification("⚠️ Locked: Time mode is active!", this); return; }
        if(isPassMode && editPass->text() == currentSessionPass) { ClearSessionData(); SyncPasswordToFirebase("", false); new ToastNotification("✅ Session Stopped.", this); } 
        else { new ToastNotification("❌ Wrong Password!", this); }
    }

    QStringList GetRunningAppsUI() {
        QStringList p; HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)};
        if(Process32FirstW(h, &pe)) { do { QString n = QString::fromWCharArray(pe.szExeFile).toLower(); if(!systemApps.contains(n)) p << n; } while(Process32NextW(h, &pe)); }
        CloseHandle(h); p.removeDuplicates(); p.sort(Qt::CaseInsensitive); return p;
    }

    void SyncListsFromUI() {
        auto getList = [](QListWidget* lw) { QStringList l; for(int i=0; i<lw->count(); ++i) { QWidget* widget = lw->itemWidget(lw->item(i)); if(widget) { QLabel* label = widget->findChild<QLabel*>(); if(label && label->text() != "No apps added yet." && label->text() != "No websites added yet.") l << label->text(); } } return l; };
        blockedApps = getList(listBlockApp); blockedWebs = getList(listBlockWeb); allowedApps = getList(listAllowApp); allowedWebs = getList(listAllowWeb); SaveAllData();
    }

    void LoadAllData() {
        auto lF = [&](QString fn, QStringList& l, QListWidget* lw, QString emptyMsg) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream in(&f); while(!in.atEnd()) { QString v=in.readLine().trimmed(); if(!v.isEmpty()){ l<<v; addInlineItem(lw, v, emptyMsg); } } f.close(); } updateEmptyState(lw, emptyMsg); };
        lF("bl_app.dat", blockedApps, listBlockApp, "No apps added yet."); lF("bl_web.dat", blockedWebs, listBlockWeb, "No websites added yet."); lF("al_app.dat", allowedApps, listAllowApp, "No apps added yet."); lF("al_web.dat", allowedWebs, listAllowWeb, "No websites added yet.");
        listRunning->addItems(GetRunningAppsUI());
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
            QTextStream out(&f); out << (isSessionActive?1:0) << " " << (isTimeMode?1:0) << " " << (isPassMode?1:0) << " " << currentSessionPass << " " << focusTimeTotalSeconds << " " << timerTicks << " " << (useAllowMode?1:0) << " " << (isPomodoroMode?1:0) << " " << (isPomodoroBreak?1:0) << " " << pomoTicks << " " << eyeBrightness << " " << eyeWarmth << " " << (blockReels?1:0) << " " << (blockShorts?1:0) << " " << (isAdblockActive?1:0) << " " << pomoCurrentSession << "\n" << userProfileName << "\n"; f.close();
        }
    }

    void ClearSessionData() {
        isSessionActive = isTimeMode = isPassMode = isPomodoroMode = isPomodoroBreak = false; currentSessionPass = ""; focusTimeTotalSeconds = timerTicks = pomoTicks = 0; pomoCurrentSession = 1;
        dashProgress->updateProgress(0, "Ready"); lblStatus->setText(""); lblPomoStatus->setText("Status: Ready"); SaveAllData(); updateUIStates();
    }

    void updateUIStates() {
        btnStart->setEnabled(!isSessionActive); btnStop->setEnabled(isSessionActive); spinHr->setEnabled(!isSessionActive); spinMin->setEnabled(!isSessionActive); pomoMin->setEnabled(!isSessionActive); pomoSes->setEnabled(!isSessionActive); bPStart->setEnabled(!isSessionActive); bPStop->setEnabled(isSessionActive); rbBlock->setEnabled(!isSessionActive); rbAllow->setEnabled(!isSessionActive); inBlockApp->setEnabled(!isSessionActive); inBlockWeb->setEnabled(!isSessionActive); inAllowApp->setEnabled(!isSessionActive); inAllowWeb->setEnabled(!isSessionActive); listBlockApp->setEnabled(!isSessionActive); listBlockWeb->setEnabled(!isSessionActive); listAllowApp->setEnabled(!isSessionActive); listAllowWeb->setEnabled(!isSessionActive);
        
        if(isSessionActive) {
            lblStatus->setText("🔒 Focus is Active. Controls locked.");
            if(isPassMode) {
                editPass->setEnabled(true);
                editPass->setStyleSheet("background: #FEF2F2; border: 2px solid #EF4444; color: #7F1D1D; font-weight: bold;");
                editPass->setPlaceholderText("Enter password to unlock");
            } else {
                editPass->setEnabled(false);
            }
        } else {
            lblStatus->setText("");
            editPass->setEnabled(true);
            editPass->setStyleSheet(isDarkMode ? "background: #0F172A; border: 1px solid #334155; color: #F8FAFC;" : "background: #F1F5F9; border: 1px solid #E2E8F0; color: #1E293B;");
            editPass->setPlaceholderText("Friend sets this");
        }
    }

    void SyncProfileNameToFirebase(QString name) { QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncPasswordToFirebase(QString pass, bool isLocking) { QString dId = GetDeviceID(); QString val = isLocking ? pass : ""; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=livePassword&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ livePassword = @{ stringValue = '" + val + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncTogglesToFirebase() { QString dId = GetDeviceID(); QString bR = blockReels ? "$true" : "$false", bS = blockShorts ? "$true" : "$false", bA = isAdblockActive ? "$true" : "$false"; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncLiveTrackerToFirebase() {
        QString dId = GetDeviceID(); QString mode = "None"; QString timeL = "00:00"; QString activeStr = isSessionActive ? "$true" : "$false";
        if (isSessionActive) { if(isPomodoroMode) { mode = "Pomodoro"; int l = (pomoLengthMin*60) - pomoTicks; if(isPomodoroBreak) l = (2*60) - pomoTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0')); } else if(isTimeMode) { mode = "Timer"; int l = focusTimeTotalSeconds - timerTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0')); } else if(isPassMode) { mode = "Password"; timeL = "Manual Lock"; } }
        QString usageStr = ""; for(auto i = usageStats.constBegin(); i != usageStats.constEnd(); ++i) { if(i.value() > 60) { usageStr += QString("%1: %2m | ").arg(i.key()).arg(i.value()/60); } } if(usageStr.isEmpty()) usageStr = "No app usage yet.";
        QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=isSelfControlActive&updateMask.fieldPaths=activeModeType&updateMask.fieldPaths=timeRemaining&updateMask.fieldPaths=appUsageSummary&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
        runPowerShell("$body = @{ fields = @{ isSelfControlActive = @{ booleanValue = " + activeStr + " }; activeModeType = @{ stringValue = '" + mode + "' }; timeRemaining = @{ stringValue = '" + timeL + "' }; appUsageSummary = @{ stringValue = '" + usageStr + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
    }

    void ValidateLicenseAndTrial() {
        QString dId = GetDeviceID(); HINTERNET hInternet = InternetOpenA("RasFocus", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet) {
            DWORD timeout = 4000; InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout)); InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
            QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            HINTERNET hConnect = InternetOpenUrlA(hInternet, url.toStdString().c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hConnect) {
                char buffer[1024]; DWORD bytesRead; QString response = ""; while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; } InternetCloseHandle(hConnect);
                QString fbPackage = "7 Days Trial"; int pkgPos = response.indexOf("\"package\""); if (pkgPos != -1) { int valPos = response.indexOf("\"stringValue\": \"", pkgPos); if (valPos != -1) { valPos += 16; int endPos = response.indexOf("\"", valPos); if (endPos != -1) fbPackage = response.mid(valPos, endPos - valPos); } }
                QString trialFile = GetSecretDir() + "sys_lic.dat"; QFile in(trialFile); time_t activationTime = 0; QString savedPackage = "7 Days Trial";
                if (in.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream inStream(&in); inStream >> activationTime; savedPackage = inStream.readLine().trimmed(); in.close(); } else { activationTime = time(0); savedPackage = fbPackage; QFile out(trialFile); if(out.open(QIODevice::WriteOnly|QIODevice::Text)){ QTextStream outStream(&out); outStream << activationTime << " " << savedPackage; out.close(); } }
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
        HWND hActive = GetForegroundWindow(); if(!hActive || (overlayWidget && hActive == (HWND)overlayWidget->winId())) return; WCHAR title[512];
        if(GetWindowTextW(hActive, title, 512) > 0) { QString sTitle = QString::fromWCharArray(title).toLower(); if(sTitle.contains("facebook")) usageStats["Facebook"]++; else if(sTitle.contains("youtube")) usageStats["YouTube"]++; else if(sTitle.contains("instagram")) usageStats["Instagram"]++; else { DWORD activePid; GetWindowThreadProcessId(hActive, &activePid); HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, activePid); if(ph){ WCHAR ex[MAX_PATH]; DWORD sz = MAX_PATH; if(QueryFullProcessImageNameW(ph, 0, ex, &sz)){ QString p = QString::fromWCharArray(ex); QString exe = p.mid(p.lastIndexOf('\\')+1); usageStats[exe]++; } CloseHandle(ph); } } }
    }

    void fastLoop() { 
        if(!blockAdult && !blockReels && !blockShorts && !isSessionActive) return; if(isOverlayVisible) return; HWND hActive = GetForegroundWindow();
        if(hActive && (!overlayWidget || hActive != (HWND)overlayWidget->winId())) { WCHAR title[512]; if(GetWindowTextW(hActive, title, 512) > 0) { QString sTitle = QString::fromWCharArray(title).toLower();
            if(blockAdult) { for(const QString& kw : explicitKeywords) { if(sTitle.contains(kw)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(1); return; } } }
            if(blockReels && sTitle.contains("facebook") && (sTitle.contains("reels") || sTitle.contains("video") || sTitle.contains("watch"))) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
            if(blockShorts && sTitle.contains("youtube") && sTitle.contains("shorts")) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
            if(isSessionActive) { bool isBrowser = sTitle.contains("chrome") || sTitle.contains("edge") || sTitle.contains("firefox") || sTitle.contains("brave") || sTitle.contains("opera") || sTitle.contains("vivaldi") || sTitle.contains("yandex") || sTitle.contains("safari");
                if(isBrowser) { if(useAllowMode) { bool ok = false; for(const QString& w : allowedWebs) { if(CheckMatch(w, sTitle)) { ok=true; break; } }
                    if(!ok && !sTitle.contains("allowed websites")) { CloseActiveTabAndMinimize(hActive); QString p = GetSecretDir() + "allowed_sites.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><head><title>Allowed Websites</title></head><body style='text-align:center; font-family:sans-serif; margin-top:50px; background-color:#F8FAFC;'><h2>Focus Mode is Active!</h2><p>You can only access the following websites:</p>"; for(auto x:allowedWebs) out<<"<a href='https://"<<x<<"' style='display:inline-block; margin:10px; padding:15px 25px; background:#10B981; color:white; font-weight:bold; text-decoration:none; border-radius:8px;'>" << x << "</a><br>"; out<<"</body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                } else { for(const QString& w : blockedWebs) { if(CheckMatch(w, sTitle)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; } } } } } } }
    }

    void slowLoop() { 
        TrackUsage();
        if(isSessionActive) {
            if(isTrialExpired) { ClearSessionData(); QMessageBox::critical(this, "Expired", "License Expired or Revoked. App Locked!"); return; }
            if(isPomodoroMode) { pomoTicks++; if(pomoTicks%5==0) SaveAllData();
                if(!isPomodoroBreak && pomoTicks >= pomoLengthMin*60) { isPomodoroBreak=true; pomoTicks=0; QString p = GetSecretDir() + "pomodoro_break.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><body style='background:#1e3c72; color:white; text-align:center; padding-top:100px; font-family:sans-serif;'><h1>Time to Relax & Drink Water!</h1><p>Break Started.</p></body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                else if(isPomodoroBreak && pomoTicks >= 2*60) { isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); new ToastNotification("✅ Pomodoro Complete!", this); } }
                int totalMins = isPomodoroBreak ? 2 : pomoLengthMin; int l = (totalMins*60)-pomoTicks; if(l<0) l=0; int prog = 100 - ((l * 100) / (totalMins * 60)); QString st = isPomodoroBreak ? "Break" : QString("Focus %1/%2").arg(pomoCurrentSession).arg(pomoTotalSessions); QString tt = QString("%1: %2:%3").arg(st).arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0'));
                dashProgress->updateProgress(prog, QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0'))); lblPomoStatus->setText(tt); trayIcon->setToolTip(tt);
            } else if(isTimeMode) { timerTicks++; int left = focusTimeTotalSeconds - timerTicks; if(left <= 0) { ClearSessionData(); new ToastNotification("✅ Focus Time Over!", this); return; } int prog = 100 - ((left * 100) / focusTimeTotalSeconds); QString tt = QString("%1:%2:%3").arg(left/3600, 2, 10, QChar('0')).arg((left%3600)/60, 2, 10, QChar('0')).arg(left%60, 2, 10, QChar('0')); dashProgress->updateProgress(prog, tt); trayIcon->setToolTip("Time Left: " + tt);
            } else { dashProgress->updateProgress(100, "Locked"); trayIcon->setToolTip("Focus Active (Password)"); }
            
            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)}; DWORD myPid = GetCurrentProcessId();
            if(Process32FirstW(h, &pe)) { do { if(pe.th32ProcessID == myPid) continue; QString n = QString::fromWCharArray(pe.szExeFile).toLower(); if(n == "taskmgr.exe") { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } continue; }
                if(useAllowMode) { if(!systemApps.contains(n) && !allowedApps.contains(n, Qt::CaseInsensitive) && !commonThirdPartyApps.contains(n)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } } 
                else { if(blockedApps.contains(n, Qt::CaseInsensitive)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } }
            } while(Process32NextW(h, &pe)); } CloseHandle(h);
        }
    }

    void syncLoop() { 
        ValidateLicenseAndTrial(); SyncLiveTrackerToFirebase(); 
        if (isTrialExpired) { lblLicense->setText("LICENSE EXPIRED"); lblLicense->setStyleSheet("color: red; font-weight: bold; margin-left: 20px;"); stack->setEnabled(false); if(!isSessionActive) { QMessageBox::critical(this, "Expired", "License Expired! Please upgrade to continue.", QMessageBox::Ok); stack->setEnabled(true); sidebar->setCurrentRow(7); } }
        else if (isLicenseValid) { lblLicense->setText(QString("PREMIUM: %1 DAYS").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: green; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); if(sidebar->count() > 7) sidebar->item(7)->setHidden(true); }
        else { lblLicense->setText(QString("TRIAL: %1 DAYS").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: orange; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); }
        if(!safeAdminMsg.isEmpty()) lblAdminMsg->setText("Admin Notice: " + safeAdminMsg); else lblAdminMsg->setText("");
        if(!pendingAdminChatStr.isEmpty()) { chatLog->append("<span style='color:#EC4899;'><b>Admin:</b></span> " + pendingAdminChatStr); pendingAdminChatStr = ""; }
        if(!pendingBroadcastMsg.isEmpty() && pendingBroadcastMsg != "ACK") { currentBroadcastMsg = pendingBroadcastMsg; pendingBroadcastMsg = ""; QMessageBox::information(this, "Admin Broadcast", currentBroadcastMsg); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=broadcastMsg&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ broadcastMsg = @{ stringValue = 'ACK' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        if(pendingAdminCmd == 1 && !isSessionActive) { pendingAdminCmd = 0; currentSessionPass = "12345"; isPassMode = true; isTimeMode = false; isPomodoroMode = false; isSessionActive = true; SaveAllData(); updateUIStates(); hide(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_START' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        else if(pendingAdminCmd == 2 && isSessionActive) { pendingAdminCmd = 0; ClearSessionData(); updateUIStates(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_STOP' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    }
    void closeEvent(QCloseEvent *event) override { event->ignore(); hide(); }
};

int main(int argc, char *argv[]) {
    if (!IsRunAsAdmin()) { QString args = ""; for(int i=1; i<argc; i++) args += QString(argv[i]) + " "; RequestAdminAndRestart(args); }
    HWND hExisting = FindWindowW(NULL, L"RasFocus Pro - Dashboard"); if (hExisting) { ShowWindow(hExisting, SW_RESTORE); SetForegroundWindow(hExisting); return 0; }
    SetupAutoStart(); hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    QApplication app(argc, argv); QApplication::setQuitOnLastWindowClosed(false); app.setFont(QFont("Segoe UI", 12)); 
    RasFocusApp window; QString cmdStr = ""; for(int i=1; i<argc; i++) cmdStr += argv[i];
    if(cmdStr.contains("-autostart")) { window.hide(); } else { window.showNormal(); window.raise(); window.activateWindow(); }
    int ret = app.exec(); UnhookWindowsHookEx(hKeyboardHook); return ret;
}
