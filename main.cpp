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
#include <QPainterPath>
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
#include <QSharedMemory> 

// Windows API
#include <windows.h>
#include <tlhelp32.h>
#include <wininet.h>
#include <shlobj.h>
#include <objbase.h>
#include <mmsystem.h> 

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "winmm.lib") 

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

QStringList islamicQuotes = { "\"মুমিনদের বলুন, তারা যেন তাদের দৃষ্টি নত রাখে এবং গঠনমূলক কাজ করুন।\" - (সূরা আন-নূর: ৩০)", "\"লজ্জাশীলতা কল্যাণ ছাড়া আর কিছুই বয়ে আনে না।\" - (সহীহ বুখারী)" };
QStringList timeQuotes = { "\"যারা সময়কে মূল্যায়ন করে না, সময়ও তাদেরকে মূল্যায়ন করে না।\" - এ.পি.জে. আবদুল কালাম" };

bool isSessionActive = false, isTimeMode = false, isPassMode = false, useAllowMode = false, isOverlayVisible = false;
bool blockReels = false, blockShorts = false, isAdblockActive = false, isDarkMode = false;
bool blockAdult = true; 
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

QMap<QString, int> usageStats;
DWORD lastUsageUpdate = 0;

const QString MUTEX_NAME = "RasFocusPro_SingleInstance_Mutex";
const UINT WM_WAKEUP = RegisterWindowMessageA("RasFocusPro_Wakeup");

// ==========================================
// AUDIO PLAYBACK UTILITY
// ==========================================
void ManageFocusSound(bool start) {
    if (start) {
        QString audioPath = QCoreApplication::applicationDirPath() + "/focus_noise.mp3";
        if (QFile::exists(audioPath)) {
            QString cmdOpen = "open \"" + audioPath + "\" type mpegvideo alias focusSound";
            mciSendStringA(cmdOpen.toStdString().c_str(), NULL, 0, NULL);
            mciSendStringA("play focusSound repeat", NULL, 0, NULL); 
        }
    } else {
        mciSendStringA("stop focusSound", NULL, 0, NULL);
        mciSendStringA("close focusSound", NULL, 0, NULL);
    }
}

// ==========================================
// UTILITY FUNCTIONS
// ==========================================

void runPowerShell(QString cmdBody) {
    QProcess::startDetached("powershell.exe", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << cmdBody);
}

QString GetDeviceID() {
    char compName[MAX_COMPUTERNAME_LENGTH + 1]; DWORD size = sizeof(compName); GetComputerNameA(compName, &size);
    DWORD volSerial = 0; GetVolumeInformationA("C:\\", NULL, 0, &volSerial, NULL, NULL, NULL, 0);
    char id[256]; sprintf(id, "%s-%X", compName, volSerial); return QString(id);
}

QString GetSecretDir() {
    QString dir = QDir::homePath() + "/AppData/Local/RasFocusPro"; 
    QDir().mkpath(dir);
    SetFileAttributesA(dir.toStdString().c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return dir + "/";
}

bool CheckMatch(QString url, QString sTitle) { 
    QString t = sTitle.remove(' '); QString exact = url.toLower().remove(' '); 
    if (!exact.isEmpty() && t.contains(exact)) return true; 
    QString s = url.toLower(); s.replace('.', ' ').replace('/', ' ').replace(':', ' ').replace('-', ' '); 
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
        QWidget* bg = new QWidget(); bg->setObjectName("bg");
        QVBoxLayout* bl = new QVBoxLayout(bg); bl->setContentsMargins(40, 40, 40, 40); 
        
        overlayLabel = new QLabel(); overlayLabel->setAlignment(Qt::AlignCenter); overlayLabel->setWordWrap(true);
        overlayLabel->setFont(QFont("Segoe UI", 28, QFont::Bold)); overlayLabel->setStyleSheet("color: white;");
        bl->addWidget(overlayLabel); l->addWidget(bg);
        
        overlayHideTimer = new QTimer();
        QObject::connect(overlayHideTimer, &QTimer::timeout, [](){ overlayWidget->hide(); isOverlayVisible = false; overlayHideTimer->stop(); });
    }
    SelectRandomQuote(type); overlayLabel->setText(currentDisplayQuote);
    if(type == 1) overlayWidget->setStyleSheet("#bg { background-color: #093d1f; border: 6px solid #f1c40f; border-radius: 20px; }");
    else overlayWidget->setStyleSheet("#bg { background-color: #1a252f; border: 6px solid #3498db; border-radius: 20px; }");
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
                    QMetaObject::invokeMethod(qApp, [=](){ ShowCustomOverlay(1); }, Qt::QueuedConnection); break;
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void ApplyEyeFilters() {
    if(!dimFilterWidget) {
        dimFilterWidget = new QLabel(); dimFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
        dimFilterWidget->setAttribute(Qt::WA_TranslucentBackground); dimFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); dimFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    if(!warmFilterWidget) {
        warmFilterWidget = new QLabel(); warmFilterWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
        warmFilterWidget->setAttribute(Qt::WA_TranslucentBackground); warmFilterWidget->setAttribute(Qt::WA_TransparentForMouseEvents); warmFilterWidget->setGeometry(QGuiApplication::primaryScreen()->virtualGeometry());
    }
    int dimAlpha = (100 - eyeBrightness) * 2.55; if(dimAlpha < 0) dimAlpha = 0; if(dimAlpha > 240) dimAlpha = 240;
    if(dimAlpha > 0) { dimFilterWidget->setStyleSheet(QString("background-color: rgba(0, 0, 0, %1);").arg(dimAlpha)); dimFilterWidget->show(); } else { dimFilterWidget->hide(); }
    int warmAlpha = eyeWarmth * 1.5; if(warmAlpha < 0) warmAlpha = 0; if(warmAlpha > 180) warmAlpha = 180;
    if(warmAlpha > 0) { warmFilterWidget->setStyleSheet(QString("background-color: rgba(255, 130, 0, %1);").arg(warmAlpha)); warmFilterWidget->show(); } else { warmFilterWidget->hide(); }
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

void CreateDesktopShortcut() {
    char exePath[MAX_PATH]; GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        QString linkPath = QString(desktopPath) + "\\RasFocus Pro.lnk";
        if (!QFile::exists(linkPath)) {
            CoInitialize(NULL);
            IShellLinkA* psl;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID*)&psl))) {
                psl->SetPath(exePath);
                psl->SetWorkingDirectory(QFileInfo(exePath).absolutePath().toStdString().c_str());
                IPersistFile* ppf;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf))) {
                    WCHAR wsz[MAX_PATH]; MultiByteToWideChar(CP_ACP, 0, linkPath.toStdString().c_str(), -1, wsz, MAX_PATH);
                    ppf->Save(wsz, TRUE); ppf->Release();
                }
                psl->Release();
            }
            CoUninitialize();
        }
    }
}

// ==========================================
// CUSTOM UI COMPONENTS
// ==========================================

class ToastNotification : public QWidget {
public:
    ToastNotification(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        QLabel* label = new QLabel(text, this);
        label->setStyleSheet("background-color: #10B981; color: white; padding: 18px 35px; border-radius: 10px; font-weight: bold; font-family: 'Segoe UI'; font-size: 18px; border: 2px solid #059669;");
        QVBoxLayout* layout = new QVBoxLayout(this); layout->addWidget(label); layout->setContentsMargins(0,0,0,0);
        adjustSize();
        if (parent) { move(parent->geometry().center().x() - width() / 2, parent->geometry().bottom() - 150); }
        show();
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(this); this->setGraphicsEffect(eff);
        QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity"); a->setDuration(300); a->setStartValue(0); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped);
        QTimer::singleShot(3000, this, [=]() {
            QPropertyAnimation *a2 = new QPropertyAnimation(eff, "opacity"); a2->setDuration(300); a2->setStartValue(1); a2->setEndValue(0);
            connect(a2, &QPropertyAnimation::finished, this, &QWidget::deleteLater); a2->start(QPropertyAnimation::DeleteWhenStopped);
        });
    }
};

class ToggleSwitch : public QCheckBox {
public:
    ToggleSwitch(const QString& text, QWidget* parent = nullptr) : QCheckBox(text, parent) {
        setCursor(Qt::PointingHandCursor);
        // Added sufficient minimum width and padding so text doesn't overlap with custom drawn pill
        setStyleSheet("QCheckBox { font-size: 15px; color: inherit; padding-left: 70px; font-weight: bold; min-height: 40px; min-width: 170px; } QCheckBox::indicator { width: 0px; height: 0px; }");
    }
protected:
    void paintEvent(QPaintEvent *e) override {
        QCheckBox::paintEvent(e);
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        int w = 60, h = 32; QRect rect(0, (height()-h)/2, w, h);
        QPainterPath path; path.addRoundedRect(rect, h/2, h/2);
        p.fillPath(path, isChecked() ? QColor("#15AABF") : QColor("#CBD5E1"));
        p.setBrush(QColor("#FFFFFF")); p.setPen(Qt::NoPen);
        int handleSize = 24;
        if(isChecked()) p.drawEllipse(w - handleSize - 4, rect.y() + 4, handleSize, handleSize);
        else p.drawEllipse(4, rect.y() + 4, handleSize, handleSize);
    }
};

class CircularProgress : public QWidget {
public:
    int progress = 0; QString centerText = "Ready";
    CircularProgress(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(320, 320); }
    void updateProgress(int p, QString text) { progress = p; centerText = text; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        int size = qMin(width(), height()); QRect rect((width()-size)/2 + 10, (height()-size)/2 + 10, size - 20, size - 20);
        QPen bgPen(QColor("#E2E8F0"), 20); p.setPen(bgPen); p.drawArc(rect, 0, 360 * 16);
        QPen progPen(QColor("#15AABF"), 20); progPen.setCapStyle(Qt::RoundCap); p.setPen(progPen);
        int spanAngle = -(progress * 360 * 16) / 100; p.drawArc(rect, 90 * 16, spanAngle);
        p.setPen(QColor(isDarkMode ? "#F8FAFC" : "#0F172A")); p.setFont(QFont("Segoe UI", 32, QFont::Bold)); p.drawText(rect, Qt::AlignCenter, centerText);
    }
};

class StopwatchWindow : public QWidget {
public:
    QLabel *lblSw; QWidget *controlPanel; QElapsedTimer timer; QTimer *updateTimer;
    bool isRunning = false; qint64 pausedTime = 0; QPoint dragPos;

    StopwatchWindow() {
        setWindowTitle("Pro Stopwatch"); setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground); resize(450, 160);

        QFrame* bgFrame = new QFrame(this);
        bgFrame->setStyleSheet("background-color: rgba(30, 41, 59, 0.90); border-radius: 12px; border: 3px solid #15AABF;");
        bgFrame->setGeometry(0, 0, 450, 160);

        QVBoxLayout* l = new QVBoxLayout(bgFrame);
        lblSw = new QLabel("00:00:00.00"); lblSw->setAlignment(Qt::AlignCenter);
        lblSw->setStyleSheet("font-size: 55px; font-family: 'Consolas'; font-weight: bold; color: #15AABF; background: transparent; border: none;");
        l->addWidget(lblSw);

        controlPanel = new QWidget(); QHBoxLayout* h = new QHBoxLayout(controlPanel); h->setContentsMargins(0,0,0,0);
        QPushButton* btnStart = new QPushButton("Start/Pause"); QPushButton* btnReset = new QPushButton("Reset"); QPushButton* btnClose = new QPushButton("Close");
        QString bStyle = "background: #15AABF; color: white; padding: 10px 15px; font-weight: bold; border-radius: 6px; font-size: 16px; border: none;";
        btnStart->setStyleSheet(bStyle); btnReset->setStyleSheet(bStyle); btnClose->setStyleSheet("background: #FF5C5C; color: white; padding: 10px 15px; font-weight: bold; border-radius: 6px; font-size: 16px; border: none;");
        h->addWidget(btnStart); h->addWidget(btnReset); h->addWidget(btnClose); l->addWidget(controlPanel);
        
        QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(controlPanel); eff->setOpacity(0.0); controlPanel->setGraphicsEffect(eff);

        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, [=](){
            if(isRunning) {
                qint64 el = timer.elapsed() + pausedTime;
                int ms = (el % 1000) / 10; int s = (el / 1000) % 60; int m = (el / 60000) % 60; int hr = (el / 3600000);
                lblSw->setText(QString("%1:%2:%3.%4").arg(hr, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')).arg(ms, 2, 10, QChar('0')));
            }
        });

        connect(btnStart, &QPushButton::clicked, [=](){ if(isRunning) { isRunning = false; pausedTime += timer.elapsed(); updateTimer->stop(); } else { isRunning = true; timer.start(); updateTimer->start(30); } });
        connect(btnReset, &QPushButton::clicked, [=](){ isRunning = false; pausedTime = 0; updateTimer->stop(); lblSw->setText("00:00:00.00"); });
        connect(btnClose, &QPushButton::clicked, this, &QWidget::hide);
    }
    
    void enterEvent(QEvent *e) override { QPropertyAnimation *a = new QPropertyAnimation(controlPanel->graphicsEffect(), "opacity"); a->setDuration(200); a->setStartValue(0); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped); QWidget::enterEvent(e); }
    void leaveEvent(QEvent *e) override { QPropertyAnimation *a = new QPropertyAnimation(controlPanel->graphicsEffect(), "opacity"); a->setDuration(200); a->setStartValue(1); a->setEndValue(0); a->start(QPropertyAnimation::DeleteWhenStopped); QWidget::leaveEvent(e); }
    void mousePressEvent(QMouseEvent *e) override { dragPos = e->globalPos() - frameGeometry().topLeft(); e->accept(); }
    void mouseMoveEvent(QMouseEvent *e) override { move(e->globalPos() - dragPos); e->accept(); }
};

StopwatchWindow* swWindow = nullptr;

// ==========================================
// MAIN GUI CLASS
// ==========================================
class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack = nullptr; 
    QListWidget* sidebar = nullptr; 
    QTimer *fastTimer = nullptr, *slowTimer = nullptr, *syncTimer = nullptr; 
    QSystemTrayIcon* trayIcon = nullptr;
    QLineEdit *editName = nullptr, *editPass = nullptr; 
    QSpinBox *spinHr = nullptr, *spinMin = nullptr; 
    QPushButton *btnStart = nullptr, *btnStop = nullptr;
    QLabel *lblStatus = nullptr, *lblLicense = nullptr, *lblAdminMsg = nullptr; 
    CircularProgress *dashProgress = nullptr;
    QRadioButton *rbBlock = nullptr, *rbAllow = nullptr; 
    QListWidget *listBlockApp = nullptr, *listBlockWeb = nullptr, *listAllowApp = nullptr, *listAllowWeb = nullptr;
    QComboBox *cbBlockApp = nullptr, *cbAllowApp = nullptr; 
    QComboBox *cbBlockWeb = nullptr, *cbAllowWeb = nullptr; 
    QListWidget *listRunning = nullptr; 
    ToggleSwitch *chkReels = nullptr, *chkShorts = nullptr, *chkAdblock = nullptr; 
    QSpinBox *pomoMin = nullptr, *pomoSes = nullptr;
    QPushButton *bPStart = nullptr, *bPStop = nullptr; 
    QLabel *lblPomoTime = nullptr, *lblPomoStatus = nullptr;
    ToggleSwitch *chkFocusSound = nullptr; 
    QSlider *sliderBright = nullptr, *sliderWarm = nullptr; 
    QTextEdit *chatLog = nullptr; 
    QLineEdit *chatIn = nullptr;
    QLineEdit *upgEmail = nullptr, *upgPhone = nullptr, *upgTrx = nullptr; 
    QComboBox *upgPkg = nullptr;
    
    QPoint dragPosition; bool isDragging = false;

    QFrame* createCard() {
        QFrame* card = new QFrame();
        card->setStyleSheet(isDarkMode ? "QFrame { background-color: #1E293B; border: none; border-radius: 12px; }" : "QFrame { background-color: #ffffff; border: 1px solid #E2E8F0; border-radius: 12px; }");
        return card;
    }

    void applyTheme() {
        QString bgMain = isDarkMode ? "#0F172A" : "#F8FAFC"; // Light gray gap to separate sidebar
        QString bgCard = isDarkMode ? "#1E293B" : "#FFFFFF";
        QString textMain = isDarkMode ? "#F8FAFC" : "#1E293B";
        QString borderCol = isDarkMode ? "#334155" : "#E2E8F0";
        
        // FIX: Added placeholder styling so they contrast beautifully with the pure white background
        QString baseStyle = QString(R"(
            QMainWindow { background-color: %1; }
            QLabel, QRadioButton { color: %3; font-size: 15px; font-family: 'Segoe UI', sans-serif; }
            QCheckBox { color: %3; font-family: 'Segoe UI'; font-size: 15px; }
            
            /* Inputs Styling */
            QLineEdit, QSpinBox, QTextEdit { 
                padding: 10px 15px; 
                border: 2px solid #15AABF; 
                border-radius: 6px; 
                background-color: #FFFFFF !important; 
                color: #000000 !important; 
                font-size: 15px; 
                font-weight: bold !important; 
                min-height: 20px; 
            }
            
            /* PLACEHOLDER VISIBILITY FIX */
            QLineEdit::placeholder, QTextEdit::placeholder { color: #94A3B8 !important; font-weight: normal; }
            
            QComboBox { 
                padding: 10px 15px; 
                border: 2px solid #15AABF; 
                border-radius: 6px; 
                background-color: #FFFFFF !important; 
                color: #000000 !important; 
                font-size: 15px; 
                font-weight: bold !important; 
                min-height: 20px; 
            }
            
            QComboBox::drop-down { border: none; width: 35px; }
            QComboBox::down-arrow { 
                image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; 
                border-top: 6px solid #15AABF; margin-right: 10px; 
            }
            
            QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTextEdit:focus { border: 2px solid #1298AB; background-color: #FFFFFF !important; }
            QLineEdit:disabled, QSpinBox:disabled { background-color: #E2E8F0 !important; color: #94A3B8 !important; font-weight: bold !important; border: 1px solid #CBD5E1; }
            
            QComboBox QAbstractItemView { 
                background-color: #FFFFFF !important; 
                color: #000000 !important; 
                border: 1px solid #15AABF; 
                selection-background-color: #15AABF; 
                selection-color: #FFFFFF !important; 
                font-weight: bold !important; 
                font-size: 15px; 
                outline: none; 
            }
            
            QScrollBar:vertical { border: none; background: transparent; width: 10px; margin: 0px; }
            QScrollBar::handle:vertical { background: #CBD5E1; min-height: 40px; border-radius: 5px; }
            QScrollBar::handle:vertical:hover { background: #15AABF; }
        )").arg(bgMain, bgCard, textMain, borderCol); 
        
        setStyleSheet(baseStyle);
        stack->setStyleSheet(QString("QStackedWidget { background-color: %1; }").arg(bgMain));
    }

    protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override {
        MSG *msg = static_cast<MSG *>(message);
        // This makes sure double clicking the exe wakes up the existing application smoothly
        if (msg->message == WM_WAKEUP) {
            if(this->isHidden() || this->isMinimized()) {
                this->showNormal();
            }
            this->raise();
            this->activateWindow();
            return true;
        }
        return QMainWindow::nativeEvent(eventType, message, result);
    }
    
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->pos().y() <= 50) { 
            if(this->isMaximized()) this->showNormal(); 
            else this->showMaximized(); 
        }
    }
    public:

    RasFocusApp() {
        setWindowTitle("RasFocus Pro");
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
        
        // INCREASING WINDOW SIZE FOR BEAUTIFUL SPACIOUS LAYOUT
        resize(1400, 850); 
        setMinimumSize(1250, 750);
        
        if(QFile::exists("icon.ico")) setWindowIcon(QIcon("icon.ico"));

        QWidget* central = new QWidget(); setCentralWidget(central);
        QVBoxLayout* rootLayout = new QVBoxLayout(central); rootLayout->setContentsMargins(0, 0, 0, 0); rootLayout->setSpacing(0);
        
        // --- TITLE BAR ---
        QWidget* titleBar = new QWidget(); titleBar->setFixedHeight(45);
        titleBar->setStyleSheet("background-color: #ffffff; border-bottom: 1px solid #E2E8F0;");
        QHBoxLayout* tbLayout = new QHBoxLayout(titleBar); tbLayout->setContentsMargins(15, 0, 0, 0); tbLayout->setSpacing(5);
        
        QLabel* appIcon = new QLabel();
        if(QFile::exists("icon.ico")) appIcon->setPixmap(QPixmap("icon.ico").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else { appIcon->setText("👁️"); appIcon->setStyleSheet("font-size: 20px; border: none;"); }
        QLabel* appTitle = new QLabel(" RasFocus Pro"); appTitle->setStyleSheet("font-weight: bold; color: #000000; font-size: 15px; border: none;");
        
        tbLayout->addWidget(appIcon); tbLayout->addWidget(appTitle); tbLayout->addStretch();
        
        QPushButton* btnHelp = new QPushButton("❔"); btnHelp->setFixedSize(45, 45);
        btnHelp->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { color: #15AABF; }");
        connect(btnHelp, &QPushButton::clicked, [=](){ QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/user_guide.html")); });
        tbLayout->addWidget(btnHelp);

        QPushButton* btnMail = new QPushButton("✉️"); btnMail->setFixedSize(45, 45);
        btnMail->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { color: #15AABF; }");
        connect(btnMail, &QPushButton::clicked, [=](){ QDesktopServices::openUrl(QUrl("mailto:admin@rasfocus.com?subject=RasFocus%20Feedback")); });
        tbLayout->addWidget(btnMail);

        QFrame* vLine = new QFrame(); vLine->setFrameShape(QFrame::VLine); vLine->setFrameShadow(QFrame::Sunken); vLine->setStyleSheet("color: #E2E8F0; margin-top: 10px; margin-bottom: 10px;");
        tbLayout->addWidget(vLine);

        QPushButton* btnMin = new QPushButton("—"); btnMin->setFixedSize(45, 45);
        btnMin->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 16px; } QPushButton:hover { background: #E2E8F0; color: #000000; }");
        connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized); tbLayout->addWidget(btnMin);
        
        QPushButton* btnMax = new QPushButton("◻"); btnMax->setFixedSize(45, 45);
        btnMax->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { background: #E2E8F0; color: #000000; }");
        connect(btnMax, &QPushButton::clicked, [=](){ if(this->isMaximized()) this->showNormal(); else this->showMaximized(); }); 
        tbLayout->addWidget(btnMax);

        QPushButton* btnClose = new QPushButton("X"); btnClose->setFixedSize(45, 45);
        btnClose->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { background: #FF5C5C; color: white; }");
        connect(btnClose, &QPushButton::clicked, this, &QWidget::hide); tbLayout->addWidget(btnClose);
        
        rootLayout->addWidget(titleBar);
        
        QWidget* mainContent = new QWidget(); QHBoxLayout* mainLayout = new QHBoxLayout(mainContent);
        mainLayout->setContentsMargins(0, 0, 0, 0); mainLayout->setSpacing(0);
        
        // --- SIDEBAR ---
        sidebar = new QListWidget(); sidebar->setFixedWidth(280); 
        sidebar->setStyleSheet(R"(
            QListWidget { background-color: #15AABF; border: none; padding-top: 15px; outline: 0; }
            QListWidget::item { border: none; margin: 0px; padding: 15px 25px; color: #FFFFFF; font-size: 18px; font-weight: bold; font-family: 'Segoe UI'; border-left: 6px solid transparent; }
            QListWidget::item:hover { background-color: #19B5CA; }
            QListWidget::item:selected { background-color: #F8FAFC; color: #15AABF; border-left: 6px solid #108595; }
        )");
        
        sidebar->addItem("  🛡️   Focus Mode"); 
        sidebar->addItem("  📅   Schedule");
        sidebar->addItem("  ⚙️   MagicX Options");
        sidebar->addItem("  ☕   Pomodoro Break");
        sidebar->addItem("  🔧   Settings");
        sidebar->addItem("  💬   Live Chat");
        sidebar->addItem("  ⭐   Activate Pro");
        
        for(int i=0; i<sidebar->count(); i++) sidebar->item(i)->setSizeHint(QSize(280, 65));

        stack = new QStackedWidget();
        
        setupFocusModePage(); 
        
        setupSchedulePage(); setupAdvancedPage();
        setupToolsPage(); setupSettingsPage(); setupChatPage(); setupUpgradePage();
        
        mainLayout->addWidget(sidebar); mainLayout->addWidget(stack); rootLayout->addWidget(mainContent);
        applyTheme(); 
        
        connect(sidebar, &QListWidget::currentRowChanged, [=](int idx){
            QWidget* nextPage = stack->widget(idx); QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(nextPage);
            nextPage->setGraphicsEffect(eff); stack->setCurrentIndex(idx);
            QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity"); a->setDuration(200); a->setStartValue(0.3); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped);
        });
        
        sidebar->setCurrentRow(0); setupTray(); LoadAllData(); ApplyEyeFilters();
        
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }

private:
    void addListItemWithX(QListWidget* listWidget, QString text) {
        for(int i=0; i<listWidget->count(); ++i) {
            QWidget* w = listWidget->itemWidget(listWidget->item(i));
            if(w) {
                QLabel* lbl = w->findChild<QLabel*>("itemLabel");
                if(lbl && lbl->text() == text) return; 
            } else if (listWidget->item(i)->text() == text) {
                return;
            }
        }
        QListWidgetItem* item = new QListWidgetItem(listWidget);
        item->setSizeHint(QSize(0, 45));
        QWidget* w = new QWidget();
        QHBoxLayout* hL = new QHBoxLayout(w);
        hL->setContentsMargins(15, 0, 10, 0);
        
        QLabel* lbl = new QLabel(text);
        lbl->setObjectName("itemLabel");
        lbl->setStyleSheet("font-size: 15px; font-weight: bold; color: #334155; background: transparent; border: none;");
        
        QPushButton* btnX = new QPushButton("✕");
        btnX->setFixedSize(26, 26);
        btnX->setCursor(Qt::PointingHandCursor);
        btnX->setStyleSheet("QPushButton { background-color: transparent; color: #EF4444; font-size: 18px; font-weight: bold; border-radius: 13px; border: none; padding-bottom: 2px; } QPushButton:hover { background-color: #FEE2E2; }");
        
        hL->addWidget(lbl); hL->addStretch(); hL->addWidget(btnX);
        listWidget->setItemWidget(item, w);
        connect(btnX, &QPushButton::clicked, [=](){
            delete listWidget->takeItem(listWidget->row(item));
            this->SyncListsFromUI();
        });
    }

    // ========================================================
    // ALL-IN-ONE FOCUS MODE PAGE - REDESIGNED FOR NO CLIPPING
    // ========================================================
    void setupFocusModePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); 
        l->setContentsMargins(40, 40, 40, 40); 
        l->setSpacing(25); // Better vertical spacing
        
        // Buttons CSS: Minimum width defined so text never clips!
        QString btnTeal = "QPushButton { background-color: #15AABF; color: white; padding: 10px 15px; min-width: 120px; font-size: 15px; font-weight: bold; border-radius: 6px; border: none; } QPushButton:hover { background-color: #1298AB; }";
        QString btnCoral = "QPushButton { background-color: #FF5C5C; color: white; padding: 10px 15px; min-width: 120px; font-size: 15px; font-weight: bold; border-radius: 6px; border: none; } QPushButton:hover { background-color: #E64A4A; } QPushButton:disabled { background-color: #FCA5A5; }";
        QString btnStopSt = "QPushButton { background-color: #64748B; color: white; padding: 10px 15px; min-width: 120px; font-size: 15px; font-weight: bold; border-radius: 6px; border: none; } QPushButton:hover { background-color: #475569; }";
        QString btnSecondary = "QPushButton { background-color: #D1F0F4; color: #0D7E8F; padding: 10px 15px; min-width: 140px; border-radius: 6px; font-weight: bold; border: none; font-size: 15px; } QPushButton:hover { background-color: #B5E6EC; }";
        
        // --- ROW 1: Profile & Trial ---
        QHBoxLayout* topH = new QHBoxLayout();
        topH->addWidget(new QLabel("<b>Profile Name:</b>")); 
        editName = new QLineEdit(); editName->setPlaceholderText("Enter Name"); editName->setMinimumWidth(200); topH->addWidget(editName);
        QPushButton* btnSave = new QPushButton("SAVE"); btnSave->setStyleSheet(btnTeal); topH->addWidget(btnSave);
        lblLicense = new QLabel("TRIAL: 7 DAYS LEFT"); lblLicense->setStyleSheet("font-weight: bold; font-size: 14px; margin-left: 30px; color: #F59E0B;"); topH->addWidget(lblLicense);
        
        lblAdminMsg = new QLabel(""); lblAdminMsg->setStyleSheet("color: #D97706; font-weight: bold; margin-left: 20px;");
        topH->addWidget(lblAdminMsg);
        topH->addStretch();
        
        QPushButton* btnChat = new QPushButton("LIVE CHAT"); btnChat->setStyleSheet("QPushButton { background-color: #EC4899; color: white; padding: 10px 15px; min-width: 120px; border-radius: 6px; font-weight: bold; border: none; } QPushButton:hover { background-color: #DB2777; }"); 
        QPushButton* btnUpg = new QPushButton("UPGRADE"); btnUpg->setStyleSheet("QPushButton { background-color: #F59E0B; color: white; padding: 10px 15px; min-width: 120px; border-radius: 6px; font-weight: bold; border: none; } QPushButton:hover { background-color: #D97706; }");
        topH->addWidget(btnChat); topH->addWidget(btnUpg);
        l->addLayout(topH);
        
        connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); SaveAllData(); SyncProfileNameToFirebase(userProfileName); new ToastNotification("✅ Profile Saved!", this); });
        connect(btnChat, &QPushButton::clicked, [=](){ sidebar->setCurrentRow(5); }); 
        connect(btnUpg, &QPushButton::clicked, [=](){ sidebar->setCurrentRow(6); });

        // --- ROW 2: Core Focus Controls ---
        QHBoxLayout* ctrlH = new QHBoxLayout();
        ctrlH->addWidget(new QLabel("<b>Friend Control (Pass):</b>")); 
        editPass = new QLineEdit(); editPass->setEchoMode(QLineEdit::Password); editPass->setPlaceholderText("Password"); editPass->setMinimumWidth(150); ctrlH->addWidget(editPass);
        
        ctrlH->addSpacing(20);
        ctrlH->addWidget(new QLabel("<b>Self Control (Time):</b>")); 
        spinHr = new QSpinBox(); spinHr->setSuffix(" Hr"); spinHr->setMinimumWidth(90);
        spinMin = new QSpinBox(); spinMin->setSuffix(" Min"); spinMin->setMaximum(59); spinMin->setMinimumWidth(90);
        ctrlH->addWidget(spinHr); ctrlH->addWidget(spinMin);
        
        ctrlH->addSpacing(30);
        btnStart = new QPushButton("START"); btnStart->setStyleSheet(btnCoral); ctrlH->addWidget(btnStart);
        ctrlH->addSpacing(10);
        btnStop = new QPushButton("STOP"); btnStop->setStyleSheet(btnStopSt); ctrlH->addWidget(btnStop);
        
        lblStatus = new QLabel("Ready"); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold; margin-left: 20px;"); ctrlH->addWidget(lblStatus);
        ctrlH->addStretch();
        l->addLayout(ctrlH);
        
        // --- ROW 3: Secondary Tools ---
        QHBoxLayout* toolsH = new QHBoxLayout();
        toolsH->addWidget(new QLabel("<b>Productivity Tools:</b>"));
        QPushButton* btnSW = new QPushButton("STOP WATCH"); btnSW->setStyleSheet(btnSecondary);
        QPushButton* btnPomo = new QPushButton("POMODORO"); btnPomo->setStyleSheet(btnSecondary);
        QPushButton* btnEye = new QPushButton("EYE CURE"); btnEye->setStyleSheet(btnSecondary);
        
        toolsH->addWidget(btnSW); toolsH->addWidget(btnPomo); toolsH->addWidget(btnEye);
        toolsH->addStretch();
        l->addLayout(toolsH);

        connect(btnStart, &QPushButton::clicked, this, &RasFocusApp::onStartFocus); connect(btnStop, &QPushButton::clicked, this, &RasFocusApp::onStopFocus);
        connect(btnSW, &QPushButton::clicked, [=](){ sidebar->setCurrentRow(3); if(!swWindow) swWindow = new StopwatchWindow(); swWindow->showNormal(); swWindow->activateWindow(); });
        connect(btnPomo, &QPushButton::clicked, [=](){ sidebar->setCurrentRow(3); }); connect(btnEye, &QPushButton::clicked, [=](){ sidebar->setCurrentRow(3); });

        // --- ROW 4: Radios & Toggles Spaced Properly ---
        QHBoxLayout* optH = new QHBoxLayout();
        rbBlock = new QRadioButton("Block List"); rbAllow = new QRadioButton("Allow List (Only Allow runs in Pomo)");
        rbBlock->setStyleSheet("font-weight: bold; color: #1E293B; margin-right: 15px;"); rbAllow->setStyleSheet("font-weight: bold; color: #1E293B; margin-right: 30px;");
        rbBlock->setChecked(!useAllowMode); rbAllow->setChecked(useAllowMode);
        optH->addWidget(rbBlock); optH->addWidget(rbAllow); 
        
        chkAdblock = new ToggleSwitch("AD BLOCKER (Silent)"); 
        chkReels = new ToggleSwitch("Block FB Reels"); 
        chkShorts = new ToggleSwitch("Block YT Shorts");
        chkFocusSound = new ToggleSwitch("Focus Sound"); 
        
        optH->addWidget(chkAdblock); optH->addWidget(chkReels); optH->addWidget(chkShorts); optH->addWidget(chkFocusSound);
        
        optH->addStretch(); l->addLayout(optH);

        // --- ROW 5: The Three Lists Grid ---
        QGridLayout* gl = new QGridLayout(); 
        gl->setSpacing(25); 
        gl->setColumnStretch(0, 1); gl->setColumnStretch(1, 1); gl->setColumnStretch(2, 1); // Equal width columns
        
        QString lsSt = "QListWidget { background: #FFFFFF !important; border: 1px solid #15AABF; font-size: 15px; border-radius: 6px; outline: none; } QListWidget::item { color: #000000 !important; font-weight: bold !important; border-bottom: 1px solid #F1F5F9;}";
        QString lsStRun = "QListWidget { background: #FFFFFF !important; border: 1px solid #15AABF; font-size: 15px; border-radius: 6px; padding: 5px; outline: none; } QListWidget::item { padding: 8px; color: #000000 !important; font-weight: bold !important; border-bottom: 1px solid #F1F5F9; } QListWidget::item:selected { background-color: #E0F2FE !important; color: #0369A1 !important; border-radius: 4px; }";
        
        auto makeBox = [&](QString title, QComboBox*& cbA, QListWidget*& lA, QComboBox*& cbW, QListWidget*& lW, int col) {
            gl->addWidget(new QLabel("<b>" + title + " Apps (e.g., vlc.exe):</b>"), 0, col);
            QHBoxLayout* h1x = new QHBoxLayout(); 
            cbA = new QComboBox(); cbA->setEditable(true); cbA->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            QPushButton* bAddA = new QPushButton("ADD"); bAddA->setStyleSheet(btnTeal); bAddA->setMinimumWidth(80);
            h1x->addWidget(cbA); h1x->addWidget(bAddA); gl->addLayout(h1x, 1, col);
            lA = new QListWidget(); lA->setStyleSheet(lsSt); gl->addWidget(lA, 2, col);
            
            gl->addWidget(new QLabel("<b>" + title + " Websites:</b>"), 3, col);
            QHBoxLayout* h2x = new QHBoxLayout(); 
            cbW = new QComboBox(); cbW->setEditable(true); cbW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); 
            QPushButton* bAddW = new QPushButton("ADD"); bAddW->setStyleSheet(btnTeal); bAddW->setMinimumWidth(80);
            h2x->addWidget(cbW); h2x->addWidget(bAddW); gl->addLayout(h2x, 4, col);
            lW = new QListWidget(); lW->setStyleSheet(lsSt); gl->addWidget(lW, 5, col);
            
            connect(bAddA, &QPushButton::clicked, [=](){ 
                QString t = cbA->currentText().trimmed().toLower(); 
                if(!t.isEmpty()){ 
                    if(!t.endsWith(".exe")) t += ".exe"; 
                    addListItemWithX(lA, t); 
                    cbA->setCurrentText(""); 
                    this->SyncListsFromUI(); 
                } 
            });
            connect(bAddW, &QPushButton::clicked, [=](){ 
                QString t = cbW->currentText().trimmed().toLower(); 
                if(!t.isEmpty()){ 
                    addListItemWithX(lW, t); 
                    cbW->setCurrentText(""); 
                    this->SyncListsFromUI(); 
                } 
            });
        };
        
        makeBox("Block", cbBlockApp, listBlockApp, cbBlockWeb, listBlockWeb, 0); 
        
        QVBoxLayout* midL = new QVBoxLayout();
        midL->addWidget(new QLabel("<b>Running Apps (Auto-Detected):</b>"));
        QPushButton* bRun = new QPushButton("Add Selected App to List"); bRun->setStyleSheet(btnSecondary); midL->addWidget(bRun);
        listRunning = new QListWidget(); listRunning->setStyleSheet(lsStRun); midL->addWidget(listRunning);
        gl->addLayout(midL, 0, 1, 6, 1);
        connect(bRun, &QPushButton::clicked, [=](){ 
            if(!listRunning->currentItem()) return; 
            QString app = listRunning->currentItem()->text().trimmed().toLower(); 
            if(!app.endsWith(".exe")) app += ".exe"; 
            if(useAllowMode) { addListItemWithX(listAllowApp, app); } 
            else { addListItemWithX(listBlockApp, app); } 
            this->SyncListsFromUI(); 
        });
        
        makeBox("Allow", cbAllowApp, listAllowApp, cbAllowWeb, listAllowWeb, 2); 
        
        l->addLayout(gl);
        stack->addWidget(page);

        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com"};
        cbBlockWeb->addItems(popSites); cbBlockWeb->setCurrentText(""); 
        cbAllowWeb->addItems(popSites); cbAllowWeb->setCurrentText(""); 
    }

    void RefreshAppDropdowns() {
        QStringList apps = GetRunningAppsUI();
        if(cbBlockApp) { cbBlockApp->clear(); cbBlockApp->addItem("Select.."); cbBlockApp->addItems(apps); cbBlockApp->setCurrentIndex(0); }
        if(cbAllowApp) { cbAllowApp->clear(); cbAllowApp->addItem("Select.."); cbAllowApp->addItems(apps); cbAllowApp->setCurrentIndex(0); }
        if(listRunning) { listRunning->clear(); listRunning->addItems(apps); }
    }

    void setupSchedulePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(60, 60, 60, 60);
        QLabel* title = new QLabel("Schedule (Coming Soon)"); title->setStyleSheet("font-size: 28px; font-weight: bold; color: #15AABF;");
        l->addWidget(title); l->addStretch(); stack->addWidget(page);
    }

    void setupAdvancedPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QLabel* title = new QLabel("Advanced Restrictions"); title->setStyleSheet("font-size: 26px; font-weight: bold; margin-bottom: 10px; color: #0F172A;"); l->addWidget(title);
        l->addStretch(); stack->addWidget(page);
    }

    void setupToolsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); 
        l->setContentsMargins(60, 40, 60, 40); 
        
        QFrame* pomoContainer = new QFrame();
        pomoContainer->setFixedWidth(750);
        pomoContainer->setStyleSheet("QFrame { background-color: transparent; }");
        QVBoxLayout* pomoLayout = new QVBoxLayout(pomoContainer);
        pomoLayout->setContentsMargins(0, 0, 0, 0);
        pomoLayout->setSpacing(0);
        
        QFrame* topFrame = new QFrame(); 
        topFrame->setStyleSheet("QFrame { background-color: #15AABF; border-top-left-radius: 12px; border-top-right-radius: 12px; border: none; }"); 
        QVBoxLayout* tL = new QVBoxLayout(topFrame); 
        tL->setAlignment(Qt::AlignCenter); 
        tL->setSpacing(10);
        tL->setContentsMargins(30, 40, 30, 40);
        
        QLabel* title = new QLabel("Next break in:"); 
        title->setStyleSheet("color: white; font-weight: bold; font-size: 18px; background: transparent; border: none;"); title->setAlignment(Qt::AlignCenter);
        
        dashProgress = new CircularProgress(); 
        dashProgress->hide(); 
        
        lblPomoTime = new QLabel("00:00:00"); 
        lblPomoTime->setStyleSheet("color: white; font-size: 110px; font-family: 'Segoe UI', Arial; font-weight: 300; background: transparent; border: none; margin: 10px 0;"); 
        lblPomoTime->setAlignment(Qt::AlignCenter);
        
        bPStart = new QPushButton("START"); 
        bPStart->setFixedSize(200, 50); 
        bPStart->setStyleSheet("QPushButton { background-color: #FF5C5C; color: white; font-weight: bold; font-size: 20px; border-radius: 8px; border: none; letter-spacing: 1px; } QPushButton:hover { background-color: #E64A4A; } QPushButton:disabled { background-color: #FCA5A5; }");
        
        tL->addWidget(title); 
        tL->addWidget(lblPomoTime); 
        tL->addWidget(bPStart, 0, Qt::AlignCenter);
        pomoLayout->addWidget(topFrame);
        
        QFrame* botFrame = new QFrame(); 
        botFrame->setStyleSheet(QString("QFrame { background-color: %1; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; border: 1px solid #E2E8F0; border-top: none; }").arg(isDarkMode ? "#1E293B" : "#FFFFFF"));
        QVBoxLayout* bL = new QVBoxLayout(botFrame); 
        bL->setContentsMargins(50, 40, 50, 40); 
        bL->setSpacing(25);
        
        QHBoxLayout* h1 = new QHBoxLayout(); h1->addWidget(new QLabel("Pomodoro Focus Length")); pomoMin = new QSpinBox(); pomoMin->setValue(25); pomoMin->setSuffix(" Minutes"); h1->addWidget(pomoMin); bL->addLayout(h1);
        QHBoxLayout* h2 = new QHBoxLayout(); h2->addWidget(new QLabel("Total Sessions")); pomoSes = new QSpinBox(); pomoSes->setValue(4); h2->addWidget(pomoSes); bL->addLayout(h2);
        
        bPStop = new QPushButton("Stop Timer"); bPStop->setStyleSheet("QPushButton { background: transparent; color: #EF4444; text-decoration: underline; font-weight: bold; font-size: 16px; border: none; }"); bL->addWidget(bPStop, 0, Qt::AlignCenter);
        lblPomoStatus = new QLabel("Ready"); lblPomoStatus->setAlignment(Qt::AlignCenter); lblPomoStatus->setStyleSheet("font-size: 16px; font-weight:bold; color: #64748B; border: none; background: transparent;"); bL->addWidget(lblPomoStatus);
        
        QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #E2E8F0; margin: 10px 0;"); bL->addWidget(line);
        bL->addWidget(new QLabel("<b>Eye Focus Filters</b>"));
        sliderBright = new QSlider(Qt::Horizontal); sliderBright->setRange(10, 100); sliderBright->setValue(100); sliderBright->setStyleSheet("QSlider::handle:horizontal { background: #15AABF; width: 20px; border-radius: 10px; margin: -8px 0; } QSlider::groove:horizontal { background: #E2E8F0; height: 5px; border-radius: 2px; }");
        sliderWarm = new QSlider(Qt::Horizontal); sliderWarm->setRange(0, 100); sliderWarm->setValue(0); sliderWarm->setStyleSheet("QSlider::handle:horizontal { background: #F59E0B; width: 20px; border-radius: 10px; margin: -8px 0; } QSlider::groove:horizontal { background: #E2E8F0; height: 5px; border-radius: 2px; }");
        bL->addWidget(new QLabel("Brightness")); bL->addWidget(sliderBright); bL->addWidget(new QLabel("Warmth")); bL->addWidget(sliderWarm);
        
        pomoLayout->addWidget(botFrame);
        l->addWidget(pomoContainer, 0, Qt::AlignHCenter | Qt::AlignTop);
        
        connect(bPStart, &QPushButton::clicked, [=](){ 
            if(!isSessionActive && !isTrialExpired) { 
                pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value(); isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1; SaveAllData(); updateUIStates(); new ToastNotification("🍅 Pomodoro Started!", this); 
                if (chkFocusSound && chkFocusSound->isChecked()) ManageFocusSound(true); 
            } 
        });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { ClearSessionData(); updateUIStates(); new ToastNotification("🛑 Pomodoro Stopped!", this); } });
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; ApplyEyeFilters(); SaveAllData(); }); connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; ApplyEyeFilters(); SaveAllData(); });
    }

    void setupSettingsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QLabel* title = new QLabel("Settings & Theme"); title->setStyleSheet("font-size: 26px; font-weight: bold; margin-bottom: 25px; color: #15AABF;"); l->addWidget(title);
        
        QFrame* themeCard = createCard(); QVBoxLayout* tl = new QVBoxLayout(themeCard); tl->setContentsMargins(40, 40, 40, 40);
        ToggleSwitch* chkDark = new ToggleSwitch(" Enable Dark Mode"); chkDark->setChecked(isDarkMode);
        connect(chkDark, &QCheckBox::clicked, [=](bool c){ isDarkMode = c; applyTheme(); });
        tl->addWidget(chkDark); l->addWidget(themeCard);
        
        l->addSpacing(30);
        QPushButton* bOpenSw = new QPushButton("Open Floating Stopwatch"); bOpenSw->setStyleSheet("QPushButton { background: #15AABF; color: white; padding: 15px 30px; font-weight: bold; border-radius: 8px; font-size: 16px; border: none; min-width: 250px; } QPushButton:hover { background-color: #1298AB; }");
        l->addWidget(bOpenSw, 0, Qt::AlignLeft);
        connect(bOpenSw, &QPushButton::clicked, [=](){ if(!swWindow) swWindow = new StopwatchWindow(); swWindow->showNormal(); swWindow->activateWindow(); });
        
        l->addStretch(); stack->addWidget(page);
    }

    void setupChatPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QLabel* title = new QLabel("Live Chat Support"); title->setStyleSheet("font-size: 26px; font-weight: bold; color: #15AABF; margin-bottom: 20px;"); l->addWidget(title);
        QFrame* chatCard = createCard(); QVBoxLayout* cl = new QVBoxLayout(chatCard); cl->setContentsMargins(30, 30, 30, 30);
        chatLog = new QTextEdit(); chatLog->setReadOnly(true); chatLog->setStyleSheet("border: none; background: transparent; font-size: 16px;"); cl->addWidget(chatLog);
        QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #E2E8F0;"); cl->addWidget(line);
        QHBoxLayout* ch = new QHBoxLayout(); chatIn = new QLineEdit(); chatIn->setPlaceholderText("Type message..."); chatIn->setStyleSheet("border: none; background: transparent; font-size: 16px;"); ch->addWidget(chatIn);
        QPushButton* bSend = new QPushButton("Send"); bSend->setStyleSheet("QPushButton { background: #15AABF; color: white; font-weight: bold; padding: 10px 30px; font-size: 16px; border-radius: 6px; border: none; min-width: 100px; } QPushButton:hover { background-color: #1298AB; }"); ch->addWidget(bSend);
        cl->addLayout(ch); l->addWidget(chatCard);
        connect(bSend, &QPushButton::clicked, [=](){ QString msg = chatIn->text().trimmed(); if(!msg.isEmpty()) { chatLog->append("<b style='color:#15AABF;'>You:</b> " + msg); chatIn->clear(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + msg + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); } });
        stack->addWidget(page);
    }
    
    void setupUpgradePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(50, 50, 50, 50);
        QLabel* title = new QLabel("Premium Upgrade"); title->setStyleSheet("font-size: 28px; font-weight: bold; color: #F59E0B; margin-bottom: 10px;"); l->addWidget(title);
        l->addWidget(new QLabel("Send payment via Nagad/bKash to <b>01566054963</b> and submit the form below.")); l->addSpacing(30);
        QFrame* formCard = createCard(); QVBoxLayout* fl = new QVBoxLayout(formCard); fl->setContentsMargins(40, 40, 40, 40); fl->setSpacing(20);
        fl->addWidget(new QLabel("Email / Name:")); upgEmail = new QLineEdit(); fl->addWidget(upgEmail);
        fl->addWidget(new QLabel("Sender Number:")); upgPhone = new QLineEdit(); fl->addWidget(upgPhone);
        fl->addWidget(new QLabel("TrxID:")); upgTrx = new QLineEdit(); fl->addWidget(upgTrx);
        fl->addWidget(new QLabel("Package:")); upgPkg = new QComboBox(); upgPkg->addItems({"7 Days Trial", "6 Months (50 BDT)", "1 Year (100 BDT)"}); fl->addWidget(upgPkg);
        QPushButton* bSub = new QPushButton("SUBMIT"); bSub->setStyleSheet("QPushButton { background: #10B981; color: white; padding: 15px; min-width: 150px; font-weight: bold; font-size: 16px; border-radius: 6px; border: none; } QPushButton:hover { background-color: #059669; }"); fl->addWidget(bSub);
        l->addWidget(formCard);
        connect(bSub, &QPushButton::clicked, [=](){ if(upgEmail->text().isEmpty() || upgPhone->text().isEmpty() || upgTrx->text().isEmpty()) { new ToastNotification("⚠️ Fill all fields!", this); return; } QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ deviceID = @{ stringValue = '" + dId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + upgPkg->currentText() + "' }; userEmail = @{ stringValue = '" + upgEmail->text() + "' }; senderPhone = @{ stringValue = '" + upgPhone->text() + "' }; trxId = @{ stringValue = '" + upgTrx->text() + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); new ToastNotification("✅ Request Sent!", this); });
        l->addStretch(); stack->addWidget(page);
    }

    void setupTray() {
        QString iconPath = QCoreApplication::applicationDirPath() + "/icon.ico"; QIcon icon;
        if(QFile::exists(iconPath)) { icon = QIcon(iconPath); } else { QPixmap pix(32, 32); pix.fill(Qt::transparent); QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing); p.setBrush(QColor("#15AABF")); p.drawEllipse(2, 2, 28, 28); p.setPen(Qt::white); p.setFont(QFont("Segoe UI", 12, QFont::Bold)); p.drawText(pix.rect(), Qt::AlignCenter, "RF"); icon = QIcon(pix); }
        trayIcon = new QSystemTrayIcon(icon, this);
        connect(trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason r){ if(r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) { showNormal(); raise(); activateWindow(); } });
        trayIcon->show();
    }

    void onStartFocus() {
        if(isSessionActive) return; QString p = editPass->text(); int tSec = (spinHr->value() * 3600) + (spinMin->value() * 60);
        if(p.isEmpty() && tSec == 0) { new ToastNotification("⚠️ Set Password or Time!", this); return; }
        useAllowMode = rbAllow->isChecked();
        if(!p.isEmpty()) { isPassMode = true; currentSessionPass = p; SyncPasswordToFirebase(p, true); } else { isTimeMode = true; focusTimeTotalSeconds = tSec; timerTicks = 0; }
        
        isSessionActive = true; editPass->clear(); SaveAllData(); updateUIStates(); 
        
        if(chkFocusSound && chkFocusSound->isChecked()) ManageFocusSound(true);

        new ToastNotification("🔒 Focus Mode Active.", this); QTimer::singleShot(1500, this, &QWidget::hide);
    }
    
    void onStopFocus() {
        if(!isSessionActive) return;
        if(isTimeMode) { new ToastNotification("⚠️ Time mode active!", this); return; }
        if(isPassMode && editPass->text() == currentSessionPass) { 
            ClearSessionData(); SyncPasswordToFirebase("", false); 
            new ToastNotification("✅ Session Stopped.", this); 
        } 
        else { new ToastNotification("❌ Wrong Password!", this); }
    }

    QStringList GetRunningAppsUI() {
        QStringList p; HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)};
        if(Process32FirstW(h, &pe)) { do { QString n = QString::fromWCharArray(pe.szExeFile).toLower(); if(!systemApps.contains(n)) p << n; } while(Process32NextW(h, &pe)); } CloseHandle(h); p.removeDuplicates(); p.sort(Qt::CaseInsensitive); return p;
    }

    void SyncListsFromUI() { 
        QStringList bA, bW, aA, aW;
        auto extract = [](QListWidget* lw, QStringList& list) {
            for(int i=0; i<lw->count(); ++i) {
                QWidget* w = lw->itemWidget(lw->item(i));
                if(w) {
                    QLabel* lbl = w->findChild<QLabel*>("itemLabel");
                    if(lbl) list << lbl->text();
                } else {
                    if(!lw->item(i)->text().isEmpty()) list << lw->item(i)->text();
                }
            }
        };
        extract(listBlockApp, bA); extract(listBlockWeb, bW);
        extract(listAllowApp, aA); extract(listAllowWeb, aW);
        blockedApps = bA; blockedWebs = bW; allowedApps = aA; allowedWebs = aW; 
        SaveAllData(); 
    }

    void LoadAllData() {
        auto lF = [&](QString fn, QStringList& l, QListWidget* lw) { 
            QFile f(GetSecretDir() + fn); 
            if(f.open(QIODevice::ReadOnly|QIODevice::Text)) { 
                QTextStream in(&f); 
                while(!in.atEnd()) { 
                    QString v=in.readLine().trimmed(); 
                    if(!v.isEmpty()){ l<<v; addListItemWithX(lw, v); } 
                } f.close(); 
            } 
        };
        lF("bl_app.dat", blockedApps, listBlockApp); lF("bl_web.dat", blockedWebs, listBlockWeb); lF("al_app.dat", allowedApps, listAllowApp); lF("al_web.dat", allowedWebs, listAllowWeb); 
        
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QTextStream in(&f); int a=0, tm=0, pm=0, ua=0, po=0, pb=0, br=0, bs=0, ad=0, pc=1;
            in >> a >> tm >> pm >> currentSessionPass >> focusTimeTotalSeconds >> timerTicks >> ua >> po >> pb >> pomoTicks >> eyeBrightness >> eyeWarmth >> br >> bs >> ad >> pc;
            userProfileName = in.readLine().trimmed(); if(userProfileName.isEmpty()) userProfileName = in.readLine().trimmed(); 
            isSessionActive=(a==1); isTimeMode=(tm==1); isPassMode=(pm==1); useAllowMode=(ua==1); isPomodoroMode=(po==1); isPomodoroBreak=(pb==1); pomoCurrentSession=pc; blockReels=(br==1); blockShorts=(bs==1); isAdblockActive=(ad==1);
            rbAllow->setChecked(useAllowMode); chkReels->setChecked(blockReels); chkShorts->setChecked(blockShorts); chkAdblock->setChecked(isAdblockActive); sliderBright->setValue(eyeBrightness); sliderWarm->setValue(eyeWarmth); editName->setText(userProfileName); f.close();
        } updateUIStates();
        RefreshAppDropdowns(); 
    }

    void SaveAllData() {
        auto sF = [](QString fn, const QStringList& l) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::WriteOnly|QIODevice::Text)) { QTextStream out(&f); for(auto i:l) out<<i<<"\n"; f.close(); } };
        sF("bl_app.dat", blockedApps); sF("bl_web.dat", blockedWebs); sF("al_app.dat", allowedApps); sF("al_web.dat", allowedWebs);
        QFile f(GetSecretDir() + "session.dat");
        if(f.open(QIODevice::WriteOnly|QIODevice::Text)) { QTextStream out(&f); out << (isSessionActive?1:0) << " " << (isTimeMode?1:0) << " " << (isPassMode?1:0) << " " << currentSessionPass << " " << focusTimeTotalSeconds << " " << timerTicks << " " << (useAllowMode?1:0) << " " << (isPomodoroMode?1:0) << " " << (isPomodoroBreak?1:0) << " " << pomoTicks << " " << eyeBrightness << " " << eyeWarmth << " " << (blockReels?1:0) << " " << (blockShorts?1:0) << " " << (isAdblockActive?1:0) << " " << pomoCurrentSession << "\n" << userProfileName << "\n"; f.close(); }
    }

    void ClearSessionData() {
        isSessionActive = isTimeMode = isPassMode = isPomodoroMode = isPomodoroBreak = false; currentSessionPass = ""; focusTimeTotalSeconds = timerTicks = pomoTicks = 0; pomoCurrentSession = 1;
        lblStatus->setText(""); SaveAllData(); updateUIStates(); ManageFocusSound(false); 
    }

    void updateUIStates() {
        btnStart->setEnabled(!isSessionActive); btnStop->setEnabled(isSessionActive);
        editPass->setEnabled(!isSessionActive); spinHr->setEnabled(!isSessionActive); spinMin->setEnabled(!isSessionActive);
        pomoMin->setEnabled(!isSessionActive); pomoSes->setEnabled(!isSessionActive); bPStart->setEnabled(!isSessionActive); bPStop->setEnabled(isSessionActive);
        rbBlock->setEnabled(!isSessionActive); rbAllow->setEnabled(!isSessionActive); 
        if(chkFocusSound) chkFocusSound->setEnabled(!isSessionActive);
        if(cbBlockApp) cbBlockApp->setEnabled(!isSessionActive); if(cbBlockWeb) cbBlockWeb->setEnabled(!isSessionActive); 
        if(cbAllowApp) cbAllowApp->setEnabled(!isSessionActive); if(cbAllowWeb) cbAllowWeb->setEnabled(!isSessionActive); 
        if(listBlockApp) listBlockApp->setEnabled(!isSessionActive); if(listBlockWeb) listBlockWeb->setEnabled(!isSessionActive);
        if(listAllowApp) listAllowApp->setEnabled(!isSessionActive); if(listAllowWeb) listAllowWeb->setEnabled(!isSessionActive);
        if(isSessionActive) lblStatus->setText("🔒 Focus Active."); else lblStatus->setText("");
    }

    void SyncProfileNameToFirebase(QString name) { QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=profileName&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ profileName = @{ stringValue = '" + name + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncPasswordToFirebase(QString pass, bool isLocking) { QString dId = GetDeviceID(); QString val = isLocking ? pass : ""; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=livePassword&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ livePassword = @{ stringValue = '" + val + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncTogglesToFirebase() { QString dId = GetDeviceID(); QString bR = blockReels ? "$true" : "$false", bS = blockShorts ? "$true" : "$false", bA = isAdblockActive ? "$true" : "$false"; QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=fbReelsBlock&updateMask.fieldPaths=ytShortsBlock&updateMask.fieldPaths=adBlock&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ fbReelsBlock = @{ booleanValue = " + bR + " }; ytShortsBlock = @{ booleanValue = " + bS + " }; adBlock = @{ booleanValue = " + bA + " } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    void SyncLiveTrackerToFirebase() {
        QString dId = GetDeviceID(); QString mode = "None"; QString timeL = "00:00"; QString activeStr = isSessionActive ? "$true" : "$false";
        if (isSessionActive) {
            if(isPomodoroMode) { mode = "Pomodoro"; int l = (pomoLengthMin*60) - pomoTicks; if(isPomodoroBreak) l = (2*60) - pomoTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0')); }
            else if(isTimeMode) { mode = "Timer"; int l = focusTimeTotalSeconds - timerTicks; if(l<0) l=0; timeL = QString("%1:%2").arg(l/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0')); }
            else if(isPassMode) { mode = "Password"; timeL = "Manual Lock"; }
        }
        QString usageStr = ""; for(auto i = usageStats.constBegin(); i != usageStats.constEnd(); ++i) { if(i.value() > 60) { usageStr += QString("%1: %2m | ").arg(i.key()).arg(i.value()/60); } } if(usageStr.isEmpty()) usageStr = "No app usage yet.";
        QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=isSelfControlActive&updateMask.fieldPaths=activeModeType&updateMask.fieldPaths=timeRemaining&updateMask.fieldPaths=appUsageSummary&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ isSelfControlActive = @{ booleanValue = " + activeStr + " }; activeModeType = @{ stringValue = '" + mode + "' }; timeRemaining = @{ stringValue = '" + timeL + "' }; appUsageSummary = @{ stringValue = '" + usageStr + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'");
    }

    void ValidateLicenseAndTrial() {
        QString dId = GetDeviceID(); HINTERNET hInternet = InternetOpenA("RasFocus", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (hInternet) {
            DWORD timeout = 4000; InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout)); InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
            QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY";
            HINTERNET hConnect = InternetOpenUrlA(hInternet, url.toStdString().c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (hConnect) {
                char buffer[1024]; DWORD bytesRead; QString response = "";
                while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) { buffer[bytesRead] = '\0'; response += buffer; } InternetCloseHandle(hConnect);
                QString fbPackage = "7 Days Trial"; int pkgPos = response.indexOf("\"package\""); if (pkgPos != -1) { int valPos = response.indexOf("\"stringValue\": \"", pkgPos); if (valPos != -1) { valPos += 16; int endPos = response.indexOf("\"", valPos); if (endPos != -1) fbPackage = response.mid(valPos, endPos - valPos); } }
                QString trialFile = GetSecretDir() + "sys_lic.dat"; QFile in(trialFile); time_t activationTime = 0; QString savedPackage = "7 Days Trial";
                if (in.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream inStream(&in); inStream >> activationTime; savedPackage = inStream.readLine().trimmed(); in.close(); } else { activationTime = time(0); savedPackage = fbPackage; QFile out(trialFile); if(out.open(QIODevice::WriteOnly|QIODevice::Text)){ QTextStream outStream(&out); outStream << activationTime << " " << savedPackage; out.close(); } }
                int totalDays = (savedPackage.contains("1 Year")) ? 365 : ((savedPackage.contains("6 Months")) ? 180 : 7); double daysPassed = difftime(time(0), activationTime) / 86400.0; trialDaysLeft = totalDays - (int)daysPassed;
                bool explicitlyRevoked = response.contains("\"stringValue\": \"REVOKED\""); bool explicitlyApproved = response.contains("\"stringValue\": \"APPROVED\"");
                if (explicitlyRevoked) { isLicenseValid = false; isTrialExpired = true; trialDaysLeft = 0; } else { if (trialDaysLeft <= 0) { isTrialExpired = true; trialDaysLeft = 0; isLicenseValid = false; } else { isTrialExpired = false; isLicenseValid = explicitlyApproved; } }
                auto parseBool = [&](QString fName, bool defaultVal) { int pos = response.indexOf("\"" + fName + "\""); if(pos != -1) { int vPos = response.indexOf("\"booleanValue\":", pos); if(vPos != -1) { if(response.indexOf("true", vPos) < response.indexOf("}", vPos)) return true; if(response.indexOf("false", vPos) < response.indexOf("}", vPos)) return false; } } return defaultVal; };
                
                blockAdult = parseBool("adultBlock", true);
                
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
        if(!blockAdult && !blockReels && !blockShorts && !isSessionActive) return; if(isOverlayVisible) return;
        HWND hActive = GetForegroundWindow();
        if(hActive && (!overlayWidget || hActive != (HWND)overlayWidget->winId())) {
            WCHAR title[512];
            if(GetWindowTextW(hActive, title, 512) > 0) {
                QString sTitle = QString::fromWCharArray(title).toLower();
                if(blockAdult) { for(const QString& kw : explicitKeywords) { if(sTitle.contains(kw)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(1); return; } } }
                if(blockReels && sTitle.contains("facebook") && (sTitle.contains("reels") || sTitle.contains("video") || sTitle.contains("watch"))) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                if(blockShorts && sTitle.contains("youtube") && sTitle.contains("shorts")) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; }
                if(isSessionActive) {
                    bool isBrowser = sTitle.contains("chrome") || sTitle.contains("edge") || sTitle.contains("firefox") || sTitle.contains("brave") || sTitle.contains("opera") || sTitle.contains("vivaldi") || sTitle.contains("yandex") || sTitle.contains("safari");
                    if(isBrowser) {
                        if(useAllowMode) {
                            bool ok = false; for(const QString& w : allowedWebs) { if(CheckMatch(w, sTitle)) { ok=true; break; } }
                            if(!ok && !sTitle.contains("allowed websites")) { 
                                CloseActiveTabAndMinimize(hActive); QString p = GetSecretDir() + "allowed_sites.html"; QFile f(p); 
                                if(f.open(QIODevice::WriteOnly)){ 
                                    QTextStream out(&f); 
                                    out<<"<html><head><title>Allowed Websites</title></head><body style='text-align:center; font-family:sans-serif; margin-top:50px; background-color:#F8FAFC;'><h2>Focus Mode is Active!</h2><p>You can only access the following websites:</p>"; 
                                    for(auto x:allowedWebs) out<<"<a href='https://"<<x<<"' style='display:inline-block; margin:10px; padding:15px 25px; background:#1CB8C9; color:white; font-weight:bold; text-decoration:none; border-radius:8px;'>" << x << "</a><br>"; 
                                    out<<"</body></html>"; f.close(); 
                                } 
                                QDesktopServices::openUrl(QUrl::fromLocalFile(p)); 
                            }
                        } else { for(const QString& w : blockedWebs) { if(CheckMatch(w, sTitle)) { CloseActiveTabAndMinimize(hActive); ShowCustomOverlay(2); return; } } }
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
                if(!isPomodoroBreak && pomoTicks >= pomoLengthMin*60) { isPomodoroBreak=true; pomoTicks=0; QString p = GetSecretDir() + "pomodoro_break.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><body style='background:#15AABF; color:white; text-align:center; padding-top:100px; font-family:sans-serif;'><h1>Time to Relax & Drink Water!</h1><p>Break Started.</p></body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                else if(isPomodoroBreak && pomoTicks >= 2*60) { isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); new ToastNotification("✅ Pomodoro Complete!", this); } }
                
                int totalMins = isPomodoroBreak ? 2 : pomoLengthMin;
                int l = (totalMins*60)-pomoTicks; if(l<0) l=0;
                
                // For exact CareUEyes look, progress circle is hidden and we just update text.
                QString st = isPomodoroBreak ? "Break" : QString("Focus %1/%2").arg(pomoCurrentSession).arg(pomoTotalSessions);
                QString tt = QString("%1:%2:%3").arg(l/3600, 2, 10, QChar('0')).arg((l%3600)/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0'));
                
                if(lblPomoTime) lblPomoTime->setText(tt);
                if(lblPomoStatus) lblPomoStatus->setText(st); 
                if(trayIcon) trayIcon->setToolTip(st + " - " + tt);
            }
            else if(isTimeMode) {
                timerTicks++; int left = focusTimeTotalSeconds - timerTicks;
                if(left <= 0) { ClearSessionData(); new ToastNotification("✅ Focus Time Over!", this); return; }
                QString tt = QString("%1:%2:%3").arg(left/3600, 2, 10, QChar('0')).arg((left%3600)/60, 2, 10, QChar('0')).arg(left%60, 2, 10, QChar('0'));
                if(trayIcon) trayIcon->setToolTip("Time Left: " + tt);
            } else { if(trayIcon) trayIcon->setToolTip("Focus Active (Password)"); }
            
            HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); PROCESSENTRY32W pe = {sizeof(pe)}; DWORD myPid = GetCurrentProcessId();
            if(Process32FirstW(h, &pe)) {
                do {
                    if(pe.th32ProcessID == myPid) continue;
                    QString n = QString::fromWCharArray(pe.szExeFile).toLower();
                    if(n == "taskmgr.exe") { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } continue; }
                    if(useAllowMode) { if(!systemApps.contains(n) && !allowedApps.contains(n, Qt::CaseInsensitive) && !commonThirdPartyApps.contains(n)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } } 
                    else { if(blockedApps.contains(n, Qt::CaseInsensitive)) { HANDLE ph = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID); if(ph) { TerminateProcess(ph, 1); CloseHandle(ph); } } }
                } while(Process32NextW(h, &pe));
            } CloseHandle(h);
        }
    }

    void syncLoop() { 
        ValidateLicenseAndTrial(); SyncLiveTrackerToFirebase(); 
        if (isTrialExpired) { lblLicense->setText("LICENSE EXPIRED"); lblLicense->setStyleSheet("color: red; font-weight: bold; margin-left: 30px;"); stack->setEnabled(false); if(!isSessionActive) { QMessageBox::critical(this, "Expired", "License Expired! Please upgrade from the premium tab.", QMessageBox::Ok); stack->setEnabled(true); sidebar->setCurrentRow(7); } }
        else if (isLicenseValid) { lblLicense->setText(QString("PREMIUM: %1 DAYS LEFT").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: #10B981; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); if(sidebar->count() > 7) sidebar->item(7)->setHidden(true); }
        else { lblLicense->setText(QString("TRIAL: %1 DAYS LEFT").arg(trialDaysLeft)); lblLicense->setStyleSheet("color: #F59E0B; font-weight: bold; margin-left: 20px;"); stack->setEnabled(true); }
        
        if(!safeAdminMsg.isEmpty()) lblAdminMsg->setText("Admin Notice: " + safeAdminMsg); else lblAdminMsg->setText("");
        if(!pendingAdminChatStr.isEmpty()) { chatLog->append("<span style='color:#EC4899;'><b>Admin:</b></span> " + pendingAdminChatStr); pendingAdminChatStr = ""; }
        if(!pendingBroadcastMsg.isEmpty() && pendingBroadcastMsg != "ACK") { currentBroadcastMsg = pendingBroadcastMsg; pendingBroadcastMsg = ""; QMessageBox::information(this, "Admin Broadcast", currentBroadcastMsg); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=broadcastMsg&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ broadcastMsg = @{ stringValue = 'ACK' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        if(pendingAdminCmd == 1 && !isSessionActive) { pendingAdminCmd = 0; currentSessionPass = "12345"; isPassMode = true; isTimeMode = false; isPomodoroMode = false; isSessionActive = true; SaveAllData(); updateUIStates(); hide(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_START' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
        else if(pendingAdminCmd == 2 && isSessionActive) { pendingAdminCmd = 0; ClearSessionData(); updateUIStates(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=adminCmd&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ adminCmd = @{ stringValue = 'ACK_STOP' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); }
    }
    void closeEvent(QCloseEvent *event) override { event->ignore(); hide(); }
};

int main(int argc, char *argv[]) {
    // 1. Create Desktop Shortcut First
    CreateDesktopShortcut();

    // 2. Broadcast & Mutex (Wakes up old instance instantly if clicked twice)
    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME.toStdString().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SendMessageA(HWND_BROADCAST, WM_WAKEUP, 0, 0); // BroadCasting to old app
        return 0; // Instance already exists, close silently.
    }

    // 3. Registry Auto-Start (Silently runs at logon without Admin Prompt)
    SetupAutoStart(); 

    QString cmdArgs = ""; for(int i=1; i<argc; i++) cmdArgs += QString(argv[i]) + " ";
    
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false); 
    app.setFont(QFont("Segoe UI", 12)); 
    
    RasFocusApp window;
    
    // Auto-start silently to tray if argument is provided
    if(cmdArgs.contains("-autostart")) { 
        window.hide(); 
    } else { 
        window.showNormal(); window.raise(); window.activateWindow(); 
    }
    
    int ret = app.exec();
    UnhookWindowsHookEx(hKeyboardHook);
    ReleaseMutex(hMutex);
    return ret;
}
