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

QStringList blockedApps;
QStringList blockedWebs;
QStringList allowedApps;
QStringList allowedWebs;
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

class ToastNotification : public QWidget {
public:
    ToastNotification(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        QLabel* label = new QLabel(text, this);
        label->setStyleSheet("background-color: #10B981; color: white; padding: 15px 30px; border-radius: 8px; font-weight: bold; font-family: 'Segoe UI'; font-size: 16px; border: 1px solid #059669;");
        QVBoxLayout* layout = new QVBoxLayout(this); layout->addWidget(label); layout->setContentsMargins(0,0,0,0);
        adjustSize();
        if (parent) { move(parent->geometry().center().x() - width() / 2, parent->geometry().bottom() - 120); }
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
        setStyleSheet("QCheckBox { font-size: 16px; color: inherit; spacing: 15px; font-weight: bold; } QCheckBox::indicator { width: 0px; height: 0px; }");
    }
protected:
    void paintEvent(QPaintEvent *e) override {
        QCheckBox::paintEvent(e);
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        int w = 50, h = 26; QRect rect(0, (height()-h)/2, w, h);
        QPainterPath path; path.addRoundedRect(rect, h/2, h/2);
        p.fillPath(path, isChecked() ? QColor("#1CB8C9") : QColor("#CBD5E1")); 
        p.setBrush(QColor("#FFFFFF")); p.setPen(Qt::NoPen);
        int handleSize = 20;
        if(isChecked()) p.drawEllipse(w - handleSize - 3, rect.y() + 3, handleSize, handleSize);
        else p.drawEllipse(3, rect.y() + 3, handleSize, handleSize);
    }
};

class CircularProgress : public QWidget {
public:
    int progress = 0; QString centerText = "Ready";
    CircularProgress(QWidget *parent = nullptr) : QWidget(parent) { setMinimumSize(250, 250); }
    void updateProgress(int p, QString text) { progress = p; centerText = text; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        int size = qMin(width(), height()); QRect rect((width()-size)/2 + 10, (height()-size)/2 + 10, size - 20, size - 20);
        QPen bgPen(QColor("#E2E8F0"), 15); p.setPen(bgPen); p.drawArc(rect, 0, 360 * 16);
        QPen progPen(QColor("#1CB8C9"), 15); progPen.setCapStyle(Qt::RoundCap); p.setPen(progPen);
        int spanAngle = -(progress * 360 * 16) / 100; p.drawArc(rect, 90 * 16, spanAngle);
        p.setPen(QColor(isDarkMode ? "#F8FAFC" : "#0F172A")); p.setFont(QFont("Segoe UI", 24, QFont::Bold)); p.drawText(rect, Qt::AlignCenter, centerText);
    }
};

class StopwatchWindow : public QWidget {
public:
    QLabel *lblSw; QWidget *controlPanel; QElapsedTimer timer; QTimer *updateTimer;
    bool isRunning = false; qint64 pausedTime = 0; QPoint dragPos;

    StopwatchWindow() {
        setWindowTitle("Pro Stopwatch"); setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_TranslucentBackground); resize(400, 140);

        QFrame* bgFrame = new QFrame(this);
        bgFrame->setStyleSheet("background-color: rgba(30, 41, 59, 0.90); border-radius: 10px; border: 2px solid #1CB8C9;");
        bgFrame->setGeometry(0, 0, 400, 140);

        QVBoxLayout* l = new QVBoxLayout(bgFrame);
        lblSw = new QLabel("00:00:00.00"); lblSw->setAlignment(Qt::AlignCenter);
        lblSw->setStyleSheet("font-size: 45px; font-family: 'Consolas'; font-weight: bold; color: #1CB8C9; background: transparent; border: none;");
        l->addWidget(lblSw);

        controlPanel = new QWidget(); QHBoxLayout* h = new QHBoxLayout(controlPanel); h->setContentsMargins(0,0,0,0);
        QPushButton* btnStart = new QPushButton("Start/Pause"); QPushButton* btnReset = new QPushButton("Reset"); QPushButton* btnClose = new QPushButton("Close");
        QString bStyle = "background: #1CB8C9; color: white; padding: 8px 12px; font-weight: bold; border-radius: 5px; font-size: 14px; border: none;";
        btnStart->setStyleSheet(bStyle); btnReset->setStyleSheet(bStyle); btnClose->setStyleSheet("background: #EF4444; color: white; padding: 8px 12px; font-weight: bold; border-radius: 5px; font-size: 14px; border: none;");
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

class RasFocusApp : public QMainWindow {
public:
    QStackedWidget* stack; QListWidget* sidebar; QTimer *fastTimer, *slowTimer, *syncTimer; QSystemTrayIcon* trayIcon;
    QLineEdit *editName, *editPass; QSpinBox *spinHr, *spinMin; QPushButton *btnStart, *btnStop;
    QLabel *lblStatus, *lblLicense, *lblAdminMsg; CircularProgress *dashProgress;
    QRadioButton *rbBlock, *rbAllow; QListWidget *listBlockApp, *listBlockWeb, *listAllowApp, *listAllowWeb;
    QComboBox *cbBlockApp, *inBlockWeb, *cbAllowApp, *inAllowWeb; 
    ToggleSwitch *chkReels, *chkShorts, *chkAdblock; QSpinBox *pomoMin, *pomoSes;
    QPushButton *bPStart, *bPStop; QLabel *lblPomoTime, *lblPomoStatus;
    QCheckBox *chkFocusSound; 
    QSlider *sliderBright, *sliderWarm; QTextEdit *chatLog; QLineEdit *chatIn;
    QLineEdit *upgEmail, *upgPhone, *upgTrx; QComboBox *upgPkg;
    QPoint dragPosition; bool isDragging = false;

    QFrame* createCard() {
        QFrame* card = new QFrame();
        card->setObjectName("RasCard");
        card->setStyleSheet(isDarkMode ? "#RasCard { background-color: #1E293B; border-radius: 10px; }" : "#RasCard { background-color: #ffffff; border: 1px solid #CBD5E1; border-radius: 10px; }");
        return card;
    }

    void applyTheme() {
        QString bgMain = isDarkMode ? "#0F172A" : "#F1F5F9";
        QString textMain = isDarkMode ? "#F8FAFC" : "#1E293B";
        QString borderCol = isDarkMode ? "#334155" : "#CBD5E1";
        QString inputBg = isDarkMode ? "#1E293B" : "#FFFFFF";
        QString inputText = isDarkMode ? "#F8FAFC" : "#0F172A";
        
        QString baseStyle = QString(R"(
            QMainWindow { background-color: %1; }
            QLabel { color: %2; font-size: 15px; font-family: 'Segoe UI'; }
            QRadioButton, QCheckBox { color: %2; font-size: 15px; font-family: 'Segoe UI'; }
            QLineEdit, QSpinBox, QComboBox, QTextEdit { padding: 8px 12px; border: 1px solid %3; border-radius: 6px; background-color: %4; color: %5; font-size: 15px; min-height: 35px; }
            QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #1CB8C9; }
            QLineEdit:disabled, QSpinBox:disabled { background-color: #E2E8F0; color: #94A3B8; }
            QComboBox QAbstractItemView { background-color: %4; color: %5; border: 1px solid %3; selection-background-color: #E0F2FE; selection-color: #0369A1; }
            QPushButton { font-family: 'Segoe UI'; font-size: 15px; font-weight: bold; border-radius: 6px; border: none; min-height: 40px; padding: 0 15px; }
            QPushButton:hover { opacity: 0.85; }
            QPushButton:disabled { background-color: #CBD5E1; color: #64748B; }
            QScrollBar:vertical { border: none; background: transparent; width: 12px; margin: 0px; }
            QScrollBar::handle:vertical { background: %3; min-height: 30px; border-radius: 6px; }
            QScrollBar::handle:vertical:hover { background: #1CB8C9; }
        )").arg(bgMain).arg(textMain).arg(borderCol).arg(inputBg).arg(inputText);
        
        setStyleSheet(baseStyle);
        stack->setStyleSheet(QString("QStackedWidget { background-color: %1; }").arg(bgMain));
    }

    protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_WAKEUP) {
            if(this->isHidden() || this->isMinimized()) { this->showNormal(); }
            this->raise(); this->activateWindow();
            return true;
        }
        return QMainWindow::nativeEvent(eventType, message, result);
    }
    
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->pos().y() <= 40) { 
            if(this->isMaximized()) this->showNormal(); 
            else this->showMaximized(); 
        }
    }
    
    public:
    RasFocusApp() {
        setWindowTitle("RasFocus Pro");
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
        resize(1100, 750); 
        
        QWidget* central = new QWidget(); setCentralWidget(central);
        QVBoxLayout* rootLayout = new QVBoxLayout(central); rootLayout->setContentsMargins(0, 0, 0, 0); rootLayout->setSpacing(0);
        
        QWidget* titleBar = new QWidget(); titleBar->setFixedHeight(40);
        titleBar->setStyleSheet("background-color: #ffffff; border-bottom: 1px solid #E2E8F0;");
        QHBoxLayout* tbLayout = new QHBoxLayout(titleBar); tbLayout->setContentsMargins(15, 0, 0, 0); tbLayout->setSpacing(5);
        
        QLabel* appIcon = new QLabel();
        if(QFile::exists("icon.ico")) { appIcon->setPixmap(QPixmap("icon.ico").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation)); } 
        else { appIcon->setText("👁️"); appIcon->setStyleSheet("font-size: 18px; border: none;"); }
        QLabel* appTitle = new QLabel(" RasFocus Pro"); appTitle->setStyleSheet("font-weight: bold; color: #0F172A; font-size: 14px; border: none;");
        
        tbLayout->addWidget(appIcon); tbLayout->addWidget(appTitle); tbLayout->addStretch();
        
        QPushButton* btnHelp = new QPushButton("❔"); btnHelp->setFixedSize(40, 40);
        btnHelp->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { color: #1CB8C9; }");
        connect(btnHelp, &QPushButton::clicked, [=](){ QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/user_guide.html")); });
        tbLayout->addWidget(btnHelp);

        QPushButton* btnMail = new QPushButton("✉️"); btnMail->setFixedSize(40, 40);
        btnMail->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 18px; } QPushButton:hover { color: #1CB8C9; }");
        connect(btnMail, &QPushButton::clicked, [=](){ QDesktopServices::openUrl(QUrl("mailto:admin@rasfocus.com?subject=RasFocus%20Feedback")); });
        tbLayout->addWidget(btnMail);

        QFrame* vLine = new QFrame(); vLine->setFrameShape(QFrame::VLine); vLine->setFrameShadow(QFrame::Sunken); vLine->setStyleSheet("color: #E2E8F0; margin-top: 8px; margin-bottom: 8px;");
        tbLayout->addWidget(vLine);

        QPushButton* btnMin = new QPushButton("—"); btnMin->setFixedSize(40, 40);
        btnMin->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 14px; } QPushButton:hover { background: #E2E8F0; color: #0F172A; }");
        connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized); tbLayout->addWidget(btnMin);
        
        QPushButton* btnMax = new QPushButton("◻"); btnMax->setFixedSize(40, 40);
        btnMax->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 16px; } QPushButton:hover { background: #E2E8F0; color: #0F172A; }");
        connect(btnMax, &QPushButton::clicked, [=](){ if(this->isMaximized()) this->showNormal(); else this->showMaximized(); }); 
        tbLayout->addWidget(btnMax);

        QPushButton* btnClose = new QPushButton("X"); btnClose->setFixedSize(40, 40);
        btnClose->setStyleSheet("QPushButton { border: none; background: transparent; color: #64748B; font-weight: bold; border-radius:0px; font-size: 16px; } QPushButton:hover { background: #EF4444; color: white; }");
        connect(btnClose, &QPushButton::clicked, this, &QWidget::hide); tbLayout->addWidget(btnClose);
        
        rootLayout->addWidget(titleBar);
        
        QWidget* mainContent = new QWidget(); QHBoxLayout* mainLayout = new QHBoxLayout(mainContent);
        mainLayout->setContentsMargins(0, 0, 0, 0); mainLayout->setSpacing(0);
        
        sidebar = new QListWidget(); sidebar->setFixedWidth(240); 
        sidebar->setStyleSheet(R"(
            QListWidget { background-color: #1CB8C9; border: none; outline: 0; padding-top: 15px; }
            QListWidget::item { color: #FFFFFF; padding: 15px 20px; font-size: 16px; font-weight: bold; border-left: 5px solid transparent; }
            QListWidget::item:hover { background-color: rgba(0,0,0,0.1); }
            QListWidget::item:selected { background-color: #FFFFFF; color: #0F172A; border-left: 5px solid #0F172A; }
        )");
        
        sidebar->addItem("  📊   Overview");
        sidebar->addItem("  🛡️   Focus Blocks");
        sidebar->addItem("  📅   Schedule");
        sidebar->addItem("  ⚙️   MagicX Options");
        sidebar->addItem("  ☕   Pomodoro Break");
        sidebar->addItem("  🔧   Settings");
        sidebar->addItem("  💬   Live Chat");
        sidebar->addItem("  ⭐   Activate Pro");
        
        for(int i=0; i<sidebar->count(); i++){ sidebar->item(i)->setSizeHint(QSize(240, 60)); }

        stack = new QStackedWidget();
        setupOverviewPage(); setupListsPage(); setupSchedulePage(); setupAdvancedPage();
        setupToolsPage(); setupSettingsPage(); setupChatPage(); setupUpgradePage();
        
        mainLayout->addWidget(sidebar); mainLayout->addWidget(stack); rootLayout->addWidget(mainContent);
        applyTheme(); 
        
        connect(sidebar, &QListWidget::currentRowChanged, [=](int idx){
            QWidget* nextPage = stack->widget(idx); QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(nextPage);
            nextPage->setGraphicsEffect(eff); stack->setCurrentIndex(idx);
            QPropertyAnimation *a = new QPropertyAnimation(eff, "opacity"); a->setDuration(250); a->setStartValue(0.3); a->setEndValue(1); a->start(QPropertyAnimation::DeleteWhenStopped);
        });
        
        sidebar->setCurrentRow(0); setupTray(); LoadAllData(); ApplyEyeFilters();
        
        fastTimer = new QTimer(this); connect(fastTimer, &QTimer::timeout, this, &RasFocusApp::fastLoop); fastTimer->start(200);
        slowTimer = new QTimer(this); connect(slowTimer, &QTimer::timeout, this, &RasFocusApp::slowLoop); slowTimer->start(1000);
        syncTimer = new QTimer(this); connect(syncTimer, &QTimer::timeout, this, &RasFocusApp::syncLoop); syncTimer->start(4000);
    }
    
    void mousePressEvent(QMouseEvent *event) override { if (event->button() == Qt::LeftButton && event->pos().y() <= 40) { isDragging = true; dragPosition = event->globalPos() - frameGeometry().topLeft(); event->accept(); } }
    void mouseMoveEvent(QMouseEvent *event) override { if (isDragging && event->buttons() & Qt::LeftButton) { move(event->globalPos() - dragPosition); event->accept(); } }
    void mouseReleaseEvent(QMouseEvent *event) override { isDragging = false; event->accept(); }

private:
    void setupOverviewPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        
        QFrame* profileCard = createCard(); QHBoxLayout* h1 = new QHBoxLayout(profileCard); h1->setContentsMargins(30, 20, 30, 20);
        QLabel* lblP = new QLabel("Profile Name:"); lblP->setStyleSheet("font-weight: bold;"); h1->addWidget(lblP);
        editName = new QLineEdit(); editName->setMinimumWidth(250); h1->addWidget(editName);
        QPushButton* btnSave = new QPushButton("Save"); btnSave->setStyleSheet("background-color: #1CB8C9; color: white;"); btnSave->setFixedWidth(100); h1->addWidget(btnSave);
        lblLicense = new QLabel("Checking License..."); lblLicense->setStyleSheet("font-weight: bold; color: #10B981; margin-left: 20px;"); h1->addWidget(lblLicense);
        l->addWidget(profileCard);
        connect(btnSave, &QPushButton::clicked, [=](){ userProfileName = editName->text(); SaveAllData(); SyncProfileNameToFirebase(userProfileName); new ToastNotification("✅ Profile Saved!", this); });
        
        l->addSpacing(20);
        QFrame* focusCard = createCard(); QVBoxLayout* fcLayout = new QVBoxLayout(focusCard); fcLayout->setContentsMargins(30, 30, 30, 30);
        
        QGridLayout* g = new QGridLayout(); g->setSpacing(20);
        QLabel* lblF = new QLabel("Friend Control (Password):"); lblF->setStyleSheet("font-weight: bold;"); g->addWidget(lblF, 0, 0);
        editPass = new QLineEdit(); editPass->setEchoMode(QLineEdit::Password); g->addWidget(editPass, 1, 0);
        
        QLabel* lblS = new QLabel("Self Control (Timer):"); lblS->setStyleSheet("font-weight: bold;"); g->addWidget(lblS, 0, 1);
        QHBoxLayout* th = new QHBoxLayout(); spinHr = new QSpinBox(); spinHr->setSuffix(" Hr"); spinHr->setRange(0, 24); spinMin = new QSpinBox(); spinMin->setSuffix(" Min"); spinMin->setMaximum(59); 
        th->addWidget(spinHr); th->addWidget(spinMin); g->addLayout(th, 1, 1); fcLayout->addLayout(g);
        
        QHBoxLayout* h2 = new QHBoxLayout(); h2->setContentsMargins(0, 20, 0, 0);
        chkFocusSound = new QCheckBox("🔊 Play Brown Noise"); chkFocusSound->setStyleSheet("font-weight: bold; color: #64748B;");
        
        btnStart = new QPushButton("▶ START FOCUS"); btnStart->setStyleSheet("background-color: #10B981; color: white; font-size: 16px;"); btnStart->setFixedSize(180, 45);
        btnStop = new QPushButton("⏹ STOP FOCUS"); btnStop->setStyleSheet("background-color: #EF4444; color: white; font-size: 16px;"); btnStop->setFixedSize(180, 45);
        h2->addWidget(chkFocusSound); h2->addStretch(); h2->addWidget(btnStart); h2->addWidget(btnStop); fcLayout->addLayout(h2);
        l->addWidget(focusCard);
        
        dashProgress = new CircularProgress(this); l->addWidget(dashProgress, 0, Qt::AlignCenter);
        lblStatus = new QLabel(""); lblStatus->setAlignment(Qt::AlignCenter); lblStatus->setStyleSheet("color: #EF4444; font-weight: bold; font-size: 16px;"); l->addWidget(lblStatus);
        lblAdminMsg = new QLabel(""); lblAdminMsg->setAlignment(Qt::AlignCenter); lblAdminMsg->setStyleSheet("color: #8B5CF6; font-weight: bold; font-size: 16px;"); l->addWidget(lblAdminMsg);
        
        l->addStretch(); stack->addWidget(page);
        connect(btnStart, &QPushButton::clicked, this, &RasFocusApp::onStartFocus); connect(btnStop, &QPushButton::clicked, this, &RasFocusApp::onStopFocus);
    }

    void RefreshAppDropdowns() {
        QStringList apps = GetRunningAppsUI();
        if(cbBlockApp) { cbBlockApp->clear(); cbBlockApp->addItems(apps); cbBlockApp->setCurrentText(""); }
        if(cbAllowApp) { cbAllowApp->clear(); cbAllowApp->addItems(apps); cbAllowApp->setCurrentText(""); }
    }

    void setupListsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(30, 30, 30, 30); l->setSpacing(20);
        
        QFrame* modeCard = createCard(); QHBoxLayout* radL = new QHBoxLayout(modeCard); radL->setContentsMargins(20, 15, 20, 15);
        rbBlock = new QRadioButton("🚫 Block List Mode"); rbAllow = new QRadioButton("✅ Allow List Mode");
        rbBlock->setStyleSheet("font-weight:bold; color: #EF4444;"); rbAllow->setStyleSheet("font-weight:bold; color: #10B981;");
        rbBlock->setChecked(!useAllowMode); radL->addWidget(rbBlock);
        rbAllow->setChecked(useAllowMode); radL->addWidget(rbAllow);
        l->addWidget(modeCard);
        connect(rbBlock, &QRadioButton::toggled, [=](){ useAllowMode = rbAllow->isChecked(); SaveAllData(); });
        
        QHBoxLayout* mainH = new QHBoxLayout(); mainH->setSpacing(20);
        
        auto createListSection = [&](QString title, QComboBox*& cbA, QComboBox*& cbW, QListWidget*& lA, QListWidget*& lW, bool isBlock) {
            QFrame* card = createCard(); QVBoxLayout* cL = new QVBoxLayout(card); cL->setContentsMargins(20, 20, 20, 20); cL->setSpacing(10);
            QLabel* tLabel = new QLabel(title); tLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;").arg(isBlock ? "#EF4444" : "#10B981")); cL->addWidget(tLabel);

            QLabel* appLbl = new QLabel("Applications (.exe):"); appLbl->setStyleSheet("font-weight: bold;"); cL->addWidget(appLbl);
            QHBoxLayout* aH = new QHBoxLayout();
            cbA = new QComboBox(); cbA->setEditable(true); cbA->setPlaceholderText("Select or type app.exe..."); cbA->setFixedHeight(40);
            QPushButton* btnRef = new QPushButton("🔄"); btnRef->setStyleSheet("background: #64748B; color: white;"); btnRef->setFixedSize(40, 40);
            QPushButton* btnAddA = new QPushButton("Add"); btnAddA->setStyleSheet(QString("background: %1; color: white;").arg(isBlock?"#EF4444":"#10B981")); btnAddA->setFixedSize(70, 40);
            aH->addWidget(cbA, 1); aH->addWidget(btnRef); aH->addWidget(btnAddA); cL->addLayout(aH);

            lA = new QListWidget(); lA->setStyleSheet("QListWidget { border: 1px solid #CBD5E1; border-radius: 6px; padding: 5px; background: transparent; } QListWidget::item { padding: 5px; border-bottom: 1px solid #E2E8F0; }"); cL->addWidget(lA);

            QLabel* webLbl = new QLabel("Websites (URL):"); webLbl->setStyleSheet("font-weight: bold;"); cL->addWidget(webLbl);
            QHBoxLayout* wH = new QHBoxLayout();
            cbW = new QComboBox(); cbW->setEditable(true); cbW->setPlaceholderText("Type website (facebook.com)"); cbW->setFixedHeight(40);
            QPushButton* btnAddW = new QPushButton("Add"); btnAddW->setStyleSheet(QString("background: %1; color: white;").arg(isBlock?"#EF4444":"#10B981")); btnAddW->setFixedSize(70, 40);
            wH->addWidget(cbW, 1); wH->addWidget(btnAddW); cL->addLayout(wH);

            lW = new QListWidget(); lW->setStyleSheet("QListWidget { border: 1px solid #CBD5E1; border-radius: 6px; padding: 5px; background: transparent; } QListWidget::item { padding: 5px; border-bottom: 1px solid #E2E8F0; }"); cL->addWidget(lW);

            QPushButton* btnRem = new QPushButton("Remove Selected"); btnRem->setStyleSheet("background: #64748B; color: white;"); btnRem->setFixedHeight(40); cL->addWidget(btnRem);

            connect(btnRef, &QPushButton::clicked, this, &RasFocusApp::RefreshAppDropdowns);
            connect(btnAddA, &QPushButton::clicked, [=](){ QString text = cbA->currentText().trimmed().toLower(); if(!text.isEmpty()){ if(!text.endsWith(".exe")) text += ".exe"; lA->addItem(text); cbA->setCurrentText(""); SyncListsFromUI(); } });
            connect(btnAddW, &QPushButton::clicked, [=](){ QString text = cbW->currentText().trimmed().toLower(); if(!text.isEmpty()){ lW->addItem(text); cbW->setCurrentText(""); SyncListsFromUI(); } });
            connect(btnRem, &QPushButton::clicked, [=](){ if(lA->currentItem()) delete lA->takeItem(lA->currentRow()); if(lW->currentItem()) delete lW->takeItem(lW->currentRow()); SyncListsFromUI(); });

            return card;
        };

        mainH->addWidget(createListSection("🚫 Block List", cbBlockApp, inBlockWeb, listBlockApp, listBlockWeb, true));
        mainH->addWidget(createListSection("✅ Allow List", cbAllowApp, inAllowWeb, listAllowApp, listAllowWeb, false));
        
        l->addLayout(mainH); stack->addWidget(page);

        QStringList popSites = {"facebook.com", "youtube.com", "instagram.com", "tiktok.com"};
        inBlockWeb->addItems(popSites); inBlockWeb->setCurrentText("");
        inAllowWeb->addItems(popSites); inAllowWeb->setCurrentText("");
    }

    void setupSchedulePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("Schedule (Coming Soon)"); title->setStyleSheet("font-size: 24px; font-weight: bold; color: #1CB8C9;");
        l->addWidget(title); l->addStretch(); stack->addWidget(page);
    }

    void setupAdvancedPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("Advanced MagicX Features"); title->setStyleSheet("font-size: 24px; font-weight: bold; color: #0F172A; margin-bottom: 5px;"); l->addWidget(title);
        QLabel* sub = new QLabel("Enable these strict settings to prevent distractions."); sub->setStyleSheet("color: #64748B; margin-bottom: 20px;"); l->addWidget(sub);
        
        QFrame* advCard = createCard(); QVBoxLayout* advLayout = new QVBoxLayout(advCard); advLayout->setContentsMargins(30, 30, 30, 30); advLayout->setSpacing(25);
        
        chkReels = new ToggleSwitch("Block Facebook Reels"); chkShorts = new ToggleSwitch("Block YouTube Shorts"); chkAdblock = new ToggleSwitch("System AD BLOCKER");
        advLayout->addWidget(chkReels); advLayout->addWidget(chkShorts); advLayout->addWidget(chkAdblock);
        l->addWidget(advCard);
        
        connect(chkReels, &QCheckBox::clicked, [=](bool c){ blockReels = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkShorts, &QCheckBox::clicked, [=](bool c){ blockShorts = c; SaveAllData(); SyncTogglesToFirebase(); });
        connect(chkAdblock, &QCheckBox::clicked, [=](bool c){ isAdblockActive = c; ToggleAdBlock(c); SaveAllData(); SyncTogglesToFirebase(); });
        l->addStretch(); stack->addWidget(page);
    }

    void setupToolsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(0,0,0,0); l->setSpacing(0);
        
        QFrame* topFrame = new QFrame(); topFrame->setStyleSheet("background-color: #1CB8C9; border: none; border-radius: 0px;"); topFrame->setFixedHeight(250);
        QVBoxLayout* tL = new QVBoxLayout(topFrame); tL->setAlignment(Qt::AlignCenter); tL->setSpacing(5);
        QLabel* title = new QLabel("Next break in:"); title->setStyleSheet("color: white; font-weight: bold; font-size: 18px; background: transparent;"); title->setAlignment(Qt::AlignCenter);
        lblPomoTime = new QLabel("00:00:00"); lblPomoTime->setStyleSheet("color: white; font-size: 80px; font-family: 'Segoe UI Light'; background: transparent;"); lblPomoTime->setAlignment(Qt::AlignCenter);
        bPStart = new QPushButton("START"); bPStart->setFixedSize(180, 45); bPStart->setStyleSheet("background-color: #FF4D4D; color: white; font-weight: bold; font-size: 18px; border-radius: 22px;");
        tL->addWidget(title); tL->addWidget(lblPomoTime); tL->addWidget(bPStart, 0, Qt::AlignCenter);
        l->addWidget(topFrame);
        
        QWidget* botWidget = new QWidget(); QVBoxLayout* bL = new QVBoxLayout(botWidget); bL->setContentsMargins(60, 30, 60, 30); bL->setSpacing(20);
        
        QHBoxLayout* h1 = new QHBoxLayout(); QLabel* l1 = new QLabel("Pomodoro Length"); l1->setStyleSheet("font-weight:bold;"); h1->addWidget(l1); pomoMin = new QSpinBox(); pomoMin->setValue(25); pomoMin->setSuffix(" Min"); h1->addWidget(pomoMin); bL->addLayout(h1);
        QHBoxLayout* h2 = new QHBoxLayout(); QLabel* l2 = new QLabel("Total Sessions"); l2->setStyleSheet("font-weight:bold;"); h2->addWidget(l2); pomoSes = new QSpinBox(); pomoSes->setValue(4); h2->addWidget(pomoSes); bL->addLayout(h2);
        
        bPStop = new QPushButton("Stop Timer"); bPStop->setStyleSheet("background: transparent; color: #EF4444; text-decoration: underline; font-weight: bold;"); bL->addWidget(bPStop, 0, Qt::AlignCenter);
        lblPomoStatus = new QLabel("Ready"); lblPomoStatus->setAlignment(Qt::AlignCenter); lblPomoStatus->setStyleSheet("font-weight:bold; color: #64748B;"); bL->addWidget(lblPomoStatus);
        
        QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #CBD5E1;"); bL->addWidget(line);
        QLabel* l3 = new QLabel("Eye Focus Filters"); l3->setStyleSheet("font-weight:bold;"); bL->addWidget(l3);
        sliderBright = new QSlider(Qt::Horizontal); sliderBright->setRange(10, 100); sliderBright->setValue(100); sliderBright->setStyleSheet("QSlider::handle:horizontal { background: #1CB8C9; width: 18px; border-radius: 9px; margin: -6px 0; } QSlider::groove:horizontal { background: #CBD5E1; height: 6px; border-radius: 3px; }");
        sliderWarm = new QSlider(Qt::Horizontal); sliderWarm->setRange(0, 100); sliderWarm->setValue(0); sliderWarm->setStyleSheet("QSlider::handle:horizontal { background: #F59E0B; width: 18px; border-radius: 9px; margin: -6px 0; } QSlider::groove:horizontal { background: #CBD5E1; height: 6px; border-radius: 3px; }");
        bL->addWidget(new QLabel("Brightness")); bL->addWidget(sliderBright); bL->addWidget(new QLabel("Warmth")); bL->addWidget(sliderWarm);
        
        l->addWidget(botWidget); l->addStretch(); stack->addWidget(page);
        
        connect(bPStart, &QPushButton::clicked, [=](){ 
            if(!isSessionActive && !isTrialExpired) { 
                pomoLengthMin = pomoMin->value(); pomoTotalSessions = pomoSes->value(); isPomodoroMode = true; isSessionActive = true; pomoTicks = 0; pomoCurrentSession = 1; SaveAllData(); updateUIStates(); new ToastNotification("🍅 Pomodoro Started!", this); 
                if (chkFocusSound->isChecked()) ManageFocusSound(true); 
            } 
        });
        connect(bPStop, &QPushButton::clicked, [=](){ if(isPomodoroMode) { ClearSessionData(); updateUIStates(); new ToastNotification("🛑 Pomodoro Stopped!", this); } });
        connect(sliderBright, &QSlider::valueChanged, [=](int v){ eyeBrightness = v; ApplyEyeFilters(); SaveAllData(); }); connect(sliderWarm, &QSlider::valueChanged, [=](int v){ eyeWarmth = v; ApplyEyeFilters(); SaveAllData(); });
    }

    void setupSettingsPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("Settings & Theme"); title->setStyleSheet("font-size: 24px; font-weight: bold; color: #1CB8C9; margin-bottom: 20px;"); l->addWidget(title);
        
        QFrame* themeCard = createCard(); QVBoxLayout* tl = new QVBoxLayout(themeCard); tl->setContentsMargins(30, 30, 30, 30);
        ToggleSwitch* chkDark = new ToggleSwitch(" Enable Dark Mode"); chkDark->setChecked(isDarkMode);
        connect(chkDark, &QCheckBox::clicked, [=](bool c){ isDarkMode = c; applyTheme(); });
        tl->addWidget(chkDark); l->addWidget(themeCard);
        
        l->addSpacing(30);
        QPushButton* bOpenSw = new QPushButton("Open Floating Stopwatch"); bOpenSw->setStyleSheet("background: #1CB8C9; color: white;"); bOpenSw->setFixedSize(250, 45);
        l->addWidget(bOpenSw, 0, Qt::AlignLeft);
        connect(bOpenSw, &QPushButton::clicked, [=](){ if(!swWindow) swWindow = new StopwatchWindow(); swWindow->showNormal(); swWindow->activateWindow(); });
        
        l->addStretch(); stack->addWidget(page);
    }

    void setupChatPage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("Live Chat Support"); title->setStyleSheet("font-size: 24px; font-weight: bold; color: #1CB8C9; margin-bottom: 15px;"); l->addWidget(title);
        QFrame* chatCard = createCard(); QVBoxLayout* cl = new QVBoxLayout(chatCard); cl->setContentsMargins(20, 20, 20, 20);
        chatLog = new QTextEdit(); chatLog->setReadOnly(true); chatLog->setStyleSheet("border: none; background: transparent;"); cl->addWidget(chatLog);
        QFrame* line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("color: #CBD5E1;"); cl->addWidget(line);
        QHBoxLayout* ch = new QHBoxLayout(); chatIn = new QLineEdit(); chatIn->setPlaceholderText("Type message..."); chatIn->setStyleSheet("border: none; background: transparent;"); ch->addWidget(chatIn);
        QPushButton* bSend = new QPushButton("Send"); bSend->setStyleSheet("background: #1CB8C9; color: white;"); bSend->setFixedSize(100, 40); ch->addWidget(bSend);
        cl->addLayout(ch); l->addWidget(chatCard);
        connect(bSend, &QPushButton::clicked, [=](){ QString msg = chatIn->text().trimmed(); if(!msg.isEmpty()) { chatLog->append("<b style='color:#1CB8C9;'>You:</b> " + msg); chatIn->clear(); QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?updateMask.fieldPaths=liveChatUser&key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ liveChatUser = @{ stringValue = '" + msg + "' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); } });
        stack->addWidget(page);
    }
    
    void setupUpgradePage() {
        QWidget* page = new QWidget(); QVBoxLayout* l = new QVBoxLayout(page); l->setContentsMargins(40, 40, 40, 40);
        QLabel* title = new QLabel("Premium Upgrade"); title->setStyleSheet("font-size: 28px; font-weight: bold; color: #F59E0B; margin-bottom: 10px;"); l->addWidget(title);
        l->addWidget(new QLabel("Send payment via Nagad/bKash to <b>01566054963</b> and submit the form.")); l->addSpacing(20);
        QFrame* formCard = createCard(); QVBoxLayout* fl = new QVBoxLayout(formCard); fl->setContentsMargins(30, 30, 30, 30); fl->setSpacing(20);
        QLabel* l1 = new QLabel("Email / Name:"); l1->setStyleSheet("font-weight:bold;"); fl->addWidget(l1); upgEmail = new QLineEdit(); fl->addWidget(upgEmail);
        QLabel* l2 = new QLabel("Sender Number:"); l2->setStyleSheet("font-weight:bold;"); fl->addWidget(l2); upgPhone = new QLineEdit(); fl->addWidget(upgPhone);
        QLabel* l3 = new QLabel("TrxID:"); l3->setStyleSheet("font-weight:bold;"); fl->addWidget(l3); upgTrx = new QLineEdit(); fl->addWidget(upgTrx);
        QLabel* l4 = new QLabel("Package:"); l4->setStyleSheet("font-weight:bold;"); fl->addWidget(l4); upgPkg = new QComboBox(); upgPkg->addItems({"7 Days Trial", "6 Months (50 BDT)", "1 Year (100 BDT)"}); fl->addWidget(upgPkg);
        QPushButton* bSub = new QPushButton("SUBMIT"); bSub->setStyleSheet("background: #10B981; color: white;"); bSub->setFixedHeight(45); fl->addWidget(bSub);
        l->addWidget(formCard);
        connect(bSub, &QPushButton::clicked, [=](){ if(upgEmail->text().isEmpty() || upgPhone->text().isEmpty() || upgTrx->text().isEmpty()) { new ToastNotification("⚠️ Fill all fields!", this); return; } QString dId = GetDeviceID(); QString url = "https://firestore.googleapis.com/v1/projects/mywebtools-f8d53/databases/(default)/documents/subscription_requests/" + dId + "?key=AIzaSyDGd3KAo45UuqmeGFALziz_oKm3htEASHY"; runPowerShell("$body = @{ fields = @{ deviceID = @{ stringValue = '" + dId + "' }; status = @{ stringValue = 'PENDING' }; package = @{ stringValue = '" + upgPkg->currentText() + "' }; userEmail = @{ stringValue = '" + upgEmail->text() + "' }; senderPhone = @{ stringValue = '" + upgPhone->text() + "' }; trxId = @{ stringValue = '" + upgTrx->text() + "' }; adminMessage = @{ stringValue = '' } } } | ConvertTo-Json -Depth 5; Invoke-RestMethod -Uri '" + url + "' -Method Patch -Body $body -ContentType 'application/json'"); new ToastNotification("✅ Request Sent!", this); });
        l->addStretch(); stack->addWidget(page);
    }

    void setupTray() {
        QString iconPath = QCoreApplication::applicationDirPath() + "/icon.ico"; QIcon icon;
        if(QFile::exists(iconPath)) { icon = QIcon(iconPath); } else { QPixmap pix(32, 32); pix.fill(Qt::transparent); QPainter p(&pix); p.setRenderHint(QPainter::Antialiasing); p.setBrush(QColor("#1CB8C9")); p.drawEllipse(2, 2, 28, 28); p.setPen(Qt::white); p.setFont(QFont("Segoe UI", 12, QFont::Bold)); p.drawText(pix.rect(), Qt::AlignCenter, "RF"); icon = QIcon(pix); }
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
        if(chkFocusSound->isChecked()) ManageFocusSound(true); 
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

    void SyncListsFromUI() { auto getList = [](QListWidget* lw) { QStringList l; for(int i=0; i<lw->count(); ++i) l << lw->item(i)->text(); return l; }; blockedApps = getList(listBlockApp); blockedWebs = getList(listBlockWeb); allowedApps = getList(listAllowApp); allowedWebs = getList(listAllowWeb); SaveAllData(); }

    void LoadAllData() {
        auto lF = [](QString fn, QStringList& l, QListWidget* lw) { QFile f(GetSecretDir() + fn); if(f.open(QIODevice::ReadOnly|QIODevice::Text)) { QTextStream in(&f); while(!in.atEnd()) { QString v=in.readLine().trimmed(); if(!v.isEmpty()){ l<<v; lw->addItem(v); } } f.close(); } };
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
        dashProgress->updateProgress(0, "Ready"); lblPomoTime->setText("00:00:00"); lblStatus->setText(""); lblPomoStatus->setText("Status: Ready"); SaveAllData(); updateUIStates();
        ManageFocusSound(false); 
    }

    void updateUIStates() {
        btnStart->setEnabled(!isSessionActive); btnStop->setEnabled(isSessionActive);
        editPass->setEnabled(!isSessionActive); spinHr->setEnabled(!isSessionActive); spinMin->setEnabled(!isSessionActive);
        pomoMin->setEnabled(!isSessionActive); pomoSes->setEnabled(!isSessionActive); bPStart->setEnabled(!isSessionActive); bPStop->setEnabled(isSessionActive);
        rbBlock->setEnabled(!isSessionActive); rbAllow->setEnabled(!isSessionActive); chkFocusSound->setEnabled(!isSessionActive);
        if(cbBlockApp) cbBlockApp->setEnabled(!isSessionActive); if(inBlockWeb) inBlockWeb->setEnabled(!isSessionActive);
        if(cbAllowApp) cbAllowApp->setEnabled(!isSessionActive); if(inAllowWeb) inAllowWeb->setEnabled(!isSessionActive); 
        listBlockApp->setEnabled(!isSessionActive); listBlockWeb->setEnabled(!isSessionActive);
        listAllowApp->setEnabled(!isSessionActive); listAllowWeb->setEnabled(!isSessionActive);
        if(isSessionActive) lblStatus->setText("🔒 Focus is Active. Settings Locked."); else lblStatus->setText("");
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
        QString usageStr = ""; for(auto i = usageStats.constBegin(); i != usageStats.constEnd(); ++i) { if(i.value() > 60) { usageStr += QString("%1: %2m | ").arg(i.key()).arg(i.value()/60); } } if(usageStr.isEmpty()) usageStr = "No significant app usage yet.";
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
                if(!isPomodoroBreak && pomoTicks >= pomoLengthMin*60) { isPomodoroBreak=true; pomoTicks=0; QString p = GetSecretDir() + "pomodoro_break.html"; QFile f(p); if(f.open(QIODevice::WriteOnly)){ QTextStream out(&f); out<<"<html><body style='background:#1CB8C9; color:white; text-align:center; padding-top:100px; font-family:sans-serif;'><h1>Time to Relax & Drink Water!</h1><p>Break Started.</p></body></html>"; f.close(); } QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }
                else if(isPomodoroBreak && pomoTicks >= 2*60) { isPomodoroBreak=false; pomoTicks=0; pomoCurrentSession++; if(pomoCurrentSession > pomoTotalSessions) { ClearSessionData(); new ToastNotification("✅ Pomodoro Complete!", this); } }
                
                int totalMins = isPomodoroBreak ? 2 : pomoLengthMin;
                int l = (totalMins*60)-pomoTicks; if(l<0) l=0;
                int prog = 100 - ((l * 100) / (totalMins * 60));
                QString st = isPomodoroBreak ? "Break" : QString("Focus %1/%2").arg(pomoCurrentSession).arg(pomoTotalSessions);
                QString tt = QString("%1:%2:%3").arg(l/3600, 2, 10, QChar('0')).arg((l%3600)/60, 2, 10, QChar('0')).arg(l%60, 2, 10, QChar('0'));
                
                dashProgress->updateProgress(prog, tt);
                lblPomoTime->setText(tt);
                lblPomoStatus->setText(st); trayIcon->setToolTip(st + " - " + tt);
            }
            else if(isTimeMode) {
                timerTicks++; int left = focusTimeTotalSeconds - timerTicks;
                if(left <= 0) { ClearSessionData(); new ToastNotification("✅ Focus Time Over!", this); return; }
                int prog = 100 - ((left * 100) / focusTimeTotalSeconds);
                QString tt = QString("%1:%2:%3").arg(left/3600, 2, 10, QChar('0')).arg((left%3600)/60, 2, 10, QChar('0')).arg(left%60, 2, 10, QChar('0'));
                dashProgress->updateProgress(prog, tt);
                trayIcon->setToolTip("Time Left: " + tt);
            } else { dashProgress->updateProgress(100, "Locked"); trayIcon->setToolTip("Focus Active (Password)"); }
            
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
    CreateDesktopShortcut();

    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME.toStdString().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SendMessageA(HWND_BROADCAST, WM_WAKEUP, 0, 0); 
        return 0; 
    }

    SetupAutoStart(); 

    QString cmdArgs = ""; for(int i=1; i<argc; i++) cmdArgs += QString(argv[i]) + " ";
    
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false); 
    app.setFont(QFont("Segoe UI", 12)); 
    
    RasFocusApp window;
    
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
